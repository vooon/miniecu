# List of all the board related files.
FWSRC = ${MINIECU}/fw/main.c \
	${MINIECU}/fw/pios_crc.c \
	${MINIECU}/fw/pbstx.c \
	${MINIECU}/fw/alert_led.c \
	${MINIECU}/fw/th_comm.c \
	${MINIECU}/fw/th_adc.c \
	${MINIECU}/fw/param.c \
	${MINIECU}/fw/rtc_time.c \
	${MINIECU}/fw/ntc.c \
	${MINIECU}/fw/usb_vcom.c

# Required include directories
FWINC = ${MINIECU}/fw

DDEFS += -DPB_BUFFER_ONLY -DPB_NO_ERRMSG
