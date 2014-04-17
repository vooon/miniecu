# List of all the board related files.
FWSRC = ${MINIECU}/fw/main.c \
	${MINIECU}/fw/pios_crc.c \
	${MINIECU}/fw/pbstx.c \
	${MINIECU}/fw/alert_led.c \
	${MINIECU}/fw/th_comm.c \
	${MINIECU}/fw/th_adc.c

# Required include directories
FWINC = ${MINIECU}/fw

DDEFS += -DPB_BUFFER_ONLY -DPB_NO_ERRMSG
