all:
	@echo "To flash device #k run flash_#k (e.g. flash_0a to flash 10th device)"
	@exit 1

flash_%: upload_fw/batch_firmwares/%/0x00000.bin upload_fw/batch_firmwares/%/0x40000.bin sniffer/firmware/0x00000.bin sniffer/firmware/0x40000.bin
	@echo "========================================================================"
	@echo "*** Flasing device $*"
	@echo "========================================================================"
	@read -p "*** Insert UPLOAD side, connect POWER and press [ENTER] " DUMMY
	@echo "========================================================================"
	@esptool.py --port /dev/ttyUSB0 --baud 230400 write_flash 0x00000 upload_fw/batch_firmwares/$*/0x00000.bin 0x40000 upload_fw/batch_firmwares/$*/0x40000.bin
	@echo "========================================================================"
	@read -p "*** Insert SNIFFER side, connect POWER and press [ENTER] " DUMMY
	@echo "========================================================================"
	@esptool.py --port /dev/ttyUSB0 --baud 230400 write_flash 0x00000 sniffer/firmware/0x00000.bin 0x40000 sniffer/firmware/0x40000.bin
	@echo "========================================================================"
	@echo "*** DONE. LABEL DEVICE WITH [$*]"
	@echo "========================================================================"
