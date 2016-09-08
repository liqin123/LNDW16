#ifndef _WIFI_H_
#define _WIFI_H_

struct RxControl
{
    signed rssi:8; // signal intensity of packet
    unsigned rate:4;
    unsigned is_group:1;
    unsigned:1;
    unsigned sig_mode:2; // 0:is 11n packet; 1:is not 11n packet; 
    unsigned legacy_length:12; // if not 11n packet, shows length of packet. 
    unsigned damatch0:1;
    unsigned damatch1:1;
    unsigned bssidmatch0:1;
    unsigned bssidmatch1:1;
    unsigned MCS:7;
    // if is 11n packet, shows the modulation 
    // and code used (range from 0 to 76)
    unsigned CWB:1; // if is 11n packet, shows if is HT40 packet or not 
    unsigned HT_length:16;// if is 11n packet, shows length of packet.
    unsigned Smoothing:1;
    unsigned Not_Sounding:1;
    unsigned:1;
    unsigned Aggregation:1;
    unsigned STBC:2;
    unsigned FEC_CODING:1; // if is 11n packet, shows if is LDPC packet or not. 
    unsigned SGI:1;
    unsigned rxend_state:8;
    unsigned ampdu_cnt:8;
    unsigned channel:4; //which channel this packet in.
    unsigned:12;
};

struct LenSeq
{
	u16 len; // length of packet
	u16 seq; // serial number of packet, the high 12bits are serial number,
	// low 14 bits are Fragment number (usually be 0) 
	u8 addr3[6]; // the third address in packet 
};

struct sniffer_buf
{
	struct RxControl rx_ctrl;
	u8 buf[36 ]; // head of ieee80211 packet
	u16 cnt; // number count of packet 
	struct LenSeq lenseq[1]; //length of packet 
};

struct sniffer_buf2
{
	struct RxControl rx_ctrl;
	u8 buf[112];
	u16 cnt; 
	u16 len; //length of packet 
};

struct metadata
{
    uint8  flags;
    uint8  addr[6];
    signed char rssi;
    uint16 count;
    uint16 age;
};

struct cache_entry
{
    long int accumulated_rssi;
    unsigned long insert_time;
    struct metadata data;
};

#define FRAME_TYPE_MANAGEMENT 0
#define FRAME_TYPE_CONTROL    1
#define FRAME_TYPE_DATA       2

#define FRAME_SUBTYPE_MGMT_ASSOCIATION_REQUEST 0
#define FRAME_SUBTYPE_MGMT_PROBE_REQUEST       4
#define FRAME_SUBTYPE_MGMT_PROBE_RESPONSE      5
#define FRAME_SUBTYPE_MGMT_PROBE_BEACON        8

#define MAX_CHANNELS 13

#endif // _WIFI_H_
