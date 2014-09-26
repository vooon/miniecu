# List of all the board related files.
FWSRC = ${MINIECU}/fw/main.c \
	${MINIECU}/fw/lib_crc16.c \
	${MINIECU}/fw/pbstx.c \
	${MINIECU}/fw/alert_led.c \
	${MINIECU}/fw/th_comm.c \
	${MINIECU}/fw/th_adc.c \
	${MINIECU}/fw/param.c \
	${MINIECU}/fw/rtc_time.c \
	${MINIECU}/fw/ntc.c \
	${MINIECU}/fw/usb_vcom.c \
	${MINIECU}/fw/memdump.c \
	${MINIECU}/fw/th_rpm.c \
	${MINIECU}/fw/th_flash_log.c \
	${MINIECU}/fw/th_command.c \
	${MINIECU}/fw/serial1.c

# Required include directories
FWINC = ${MINIECU}/fw
