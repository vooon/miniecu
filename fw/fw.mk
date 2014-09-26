# List of all the board related files.
FWSRC = ${MINIECU}/fw/main.c \
	${MINIECU}/fw/alert_led.c \
	${MINIECU}/fw/lib/lib_crc16.c \
	${MINIECU}/fw/lib/ntc.c \
	${MINIECU}/fw/hw/usb_vcom.c \
	${MINIECU}/fw/hw/serial1.c \
	${MINIECU}/fw/comm/pbstx.c \
	${MINIECU}/fw/comm/th_comm_pbstx.c \
	${MINIECU}/fw/adc/th_adc.c \
	${MINIECU}/fw/param/param.c \
	${MINIECU}/fw/rtc_time.c \
	${MINIECU}/fw/memdump.c \
	${MINIECU}/fw/th_rpm.c \
	${MINIECU}/fw/th_flash_log.c \
	${MINIECU}/fw/th_command.c

# Required include directories
FWINC = ${MINIECU}/fw \
	${MINIECU}/fw/adc \
	${MINIECU}/fw/comm \
	${MINIECU}/fw/hw \
	${MINIECU}/fw/lib \
	${MINIECU}/fw/param
