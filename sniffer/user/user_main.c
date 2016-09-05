#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"

#include "wifi.h"
#include "mem.h"
#include "driver/uart.h"
#include "driver/uart_codec.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

#define printmac(buf, i) os_printf("\t%02X:%02X:%02X:%02X:%02X:%02X", buf[i+0], buf[i+1], buf[i+2], \
				    buf[i+3], buf[i+4], buf[i+5])

static unsigned int packet_count = 0;
static volatile os_timer_t channelHop_timer;

static void loop(os_event_t *events);
static void promisc_cb(uint8 *buf, uint16 len);

static int cs[MAX_CHANNELS];
static struct cache_entry cache[MAX_CACHE_ENTRIES];

// dummy

void ICACHE_FLASH_ATTR uart_rx_task(os_event_t *events) {}
// Initialize channel stats

#ifdef DEBUG
                                          
static int iters;
void init_stats (void)
{
    int i;

    for (i = 0; i < MAX_CHANNELS; i++)
    {
        cs[i] = 0;
    }
    iters = 0;
}
#endif // DEBUG

void cache_init (void)
{
    bzero (cache, sizeof (cache));
}

void send_packet (struct cache_entry *entry, unsigned long now)
{
    entry->data.age  = now - entry->insert_time;
    entry->data.rssi = entry->accumulated_rssi / entry->data.count;

    os_printf ("Sending packet %d\n", ++packet_count);
    uart_codec_send_packet ((const void *)&entry->data, sizeof (struct metadata), uart1_tx_buffer);
    bzero (entry, sizeof (struct cache_entry));
}

long millis()
{
    return system_get_time()/1000;
}

void send_cached (uint8 *addr, unsigned rssi)
{
    unsigned long oldest_time = ~0UL;
    long now = millis();
    int i, oldest_index;

#ifdef XM_PREFIX
    if (memcmp (addr, XM_PREFIX, XM_PREFIX_LEN) == 0)
    {
        // A packet from our sending device, ignore.
        return;
    }
#endif // XM_PREFIX

    for (i = 0; i < MAX_CACHE_ENTRIES; i++)
    {
        // Send and remove old cache entries
        if (cache[i].insert_time + MAX_CACHE_AGE_MS < now)
        {
            send_packet (&cache[i], now);
        }

        // Find oldest entry, if packet was sent above, then insert_time is 0
        // ensuring this slot is used for the next packet to be stored
        if (cache[i].insert_time < oldest_time)
        {
            oldest_time = cache[i].insert_time;
            oldest_index = i;
        }

        // Compare cache entry
        if (memcmp (addr, cache[i].data.addr, 6) == 0)
        {
            cache[i].accumulated_rssi += rssi;
            cache[i].data.count++;
            return;
        }

    }

    // Not found, send oldest entry
    if (oldest_time > 0)
    {
        send_packet (&cache[oldest_index], now);
    }

    // Add to cache
    cache[oldest_index].valid = 1;
    memcpy (&cache[oldest_index].data.addr, addr, 6);
    cache[oldest_index].insert_time = now;
    cache[oldest_index].accumulated_rssi = rssi;
    cache[oldest_index].data.count = 1;

    return;
}

int next_channel_index_from_index (int index)
{
#ifdef MAIN_CHANNELS
    switch (index)
    {
        case 0:  return  5;
        case 5:  return 10;
        case 10: return  0;
        default: return  0;
    }
#else
    return (index + 1) % MAX_CHANNELS;
#endif // !Main_CHANNELS
}

void hop_channel(void *arg)
{
    int ChannelIndex = (wifi_get_channel() - 1);

#ifdef DEBUG
    int i, sum = 0, sum1611 = 0;
    if (++iters > CHANNEL_HOPS_TILL_DEBUG_OUTPUT)
    {
        for (i = 0; i < MAX_CHANNELS; i++)
        {
            sum += cs[i];
            os_printf(" %d, %4.4d", i + 1, cs[i]);
        }
        sum1611 = cs[0] + cs[5] + cs[10];
        os_printf("\n (main=%d, total=%d)\n", sum1611, sum);
        init_stats();
    }
#endif // DEBUG

    wifi_set_channel(next_channel_index_from_index (ChannelIndex) + 1);
}

#ifdef DEBUG
void hexdump (uint8 *buf, uint16 len)
{
    int i;

    os_printf ("000000 ");
    for (i = 0; i < len; i++)
    {
       os_printf("%02X ", buf[i]);
    }
    os_printf ("\n");
}
#endif // DEBUG

#define ADDR1(buf) (buf +  4)
#define ADDR2(buf) (buf + 10)
#define ADDR3(buf) (buf + 16)

enum { DIR_UNKNOWN, DIR_UP, DIR_DOWN };

void update_and_send (uint8 *buf, uint16 len, uint8 *addr3, signed rssi)
{
    int dir = DIR_UNKNOWN;
    uint8 *sa, *da;
    int type;
    int subtype;
    int result;

    uint8 frame_control0 = buf[0];
    subtype = (frame_control0 >> 4) & 0xf;
    type    = (frame_control0 >> 2) & 0x3;

    uint8 frame_control1 = buf[1];
    int to_ds   = frame_control1 & 0x1;
    int from_ds = (frame_control1 >> 1) & 0x1;

    if (to_ds)
    {
        if (from_ds)
        { // to_ds=1, from_ds=1: Inter-DS frames (mesh, repeater, ...), ignore.
            return; // Ignore
        } else
        { // to_ds=1, from_ds=0: Frame from station to DS (upstream)
            sa  = ADDR2 (buf);
            da  = ADDR3 (buf);
            dir = DIR_UP;
        }
    } else // to_ds=0
    {
        da = ADDR1 (buf);

        if (from_ds)
        { // to_ds=0, from_ds=1: Frame forwarded by AP from DS to station
#ifdef DEBUG
            if (addr3 != NULL)
            {
                if (memcmp (addr3, ADDR3 (buf), 6) != 0)
                {
                    os_printf ("addr3 differs (addr3/frame):");
                    printmac (addr3, 0);
                    printmac (ADDR3(buf), 0);
                    os_printf ("\n");
                }
            }
#endif
            sa  = (addr3 == NULL) ? ADDR3 (buf) : addr3;
            dir = DIR_DOWN;
        } else
        { // to_ds=0, from_ds=0: Inter station traffic or management/control frames
            sa = ADDR2 (buf);

            switch (type)
            {
                case FRAME_TYPE_MANAGEMENT:
                    switch (subtype)
                    {
                        case FRAME_SUBTYPE_MGMT_PROBE_REQUEST:  dir = DIR_UP; break;
                        case FRAME_SUBTYPE_MGMT_PROBE_RESPONSE: dir = DIR_DOWN; break;
                        case FRAME_SUBTYPE_MGMT_PROBE_BEACON:   return;
                        default: DIR_UNKNOWN;
                    }
                    break;
                default: dir = DIR_UNKNOWN;
            }
        }
    }

    if (dir == DIR_UNKNOWN)
    {
        return;
    }

    if (dir == DIR_UP)
    {
        send_cached (sa, rssi);
    }

    if (dir == DIR_DOWN)
    {
        send_cached (da, rssi);
    }

#ifdef DEBUG
    if (result)
    {
        os_printf (" %1.1x.%2.2x len:%3.0d fd:%1.1d td:%1.1d [%1.1d]", type, subtype, len, from_ds, to_ds, result);
        printmac (sa, 0);
        os_printf (" => ");
        printmac (da, 0);
        os_printf ("\n");
    }

    if (from_ds == 0 && to_ds == 0 && type == 2)
    {
        hexdump (buf, len);
    }
#endif // DEBUG
}

void dissect_buffer (uint8 *buf, uint16 length)
{
    if (length == 12)
    {
        // This is just an RxControl structure which has no useful information for us. Ignore it.
        return;
    } else if (length == 128)
    {
        // This packet contains a sniffer_buf2 structure
        struct sniffer_buf2 *sb2 = (struct sniffer_buf2 *)buf;
        if (sb2->cnt != 1)
        {
#ifdef DEBUG
            os_printf ("sniffer_buf2 has cnt != 1 (%d)", sb2->cnt);
#endif // DEBUG
            return;
        }
        update_and_send (sb2->buf, sb2->len, NULL, sb2->rx_ctrl.rssi);
        return;
    } else if (length > 0 && length % 10 == 0)
    {
        struct sniffer_buf *sb = (struct sniffer_buf *)buf;
        if (sb->cnt != 1)
        {
#ifdef DEBUG
            os_printf ("Invalid sb of len %d\n", sb->cnt);
#endif // DEBUG
            return;
        }
        update_and_send (sb->buf, sb->lenseq[0].len, sb->lenseq[0].addr3, sb->rx_ctrl.rssi);
        return;
    } else
    {
#ifdef DEBUG
        os_printf ("Invalid packet with %d bytes\n", length);
#endif // DEBUG
        return;
    }
}

static void ICACHE_FLASH_ATTR
promisc_cb(uint8 *buffer, uint16 length)
{
    dissect_buffer (buffer, length);

#ifdef DEBUG
    cs[wifi_get_channel() - 1]++;
#endif // DEBUG
}

//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
    os_delay_us(10);
}

void ICACHE_FLASH_ATTR
sniffer_init_done() {

#ifdef DEBUG
    os_printf("Enter: sniffer_init_done");
#endif // DEBUG

    wifi_station_set_auto_connect(false); // do not connect automatically
    wifi_station_disconnect(); // no idea if this is permanent
    wifi_promiscuous_enable(false);
    wifi_set_promiscuous_rx_cb(promisc_cb);
    wifi_promiscuous_enable(true);

    cache_init();
    wifi_set_channel(1);

#ifdef DEBUG
    init_stats();
    os_printf("done.\n");
#endif // DEBUG
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    wifi_set_opmode(0x1); // 0x1: station mode
    system_init_done_cb(sniffer_init_done);
    system_set_os_print(true);
    
    os_timer_disarm(&channelHop_timer);
    os_timer_setfn(&channelHop_timer, (os_timer_func_t *) hop_channel, NULL);
    os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL_MS, 1);
}
