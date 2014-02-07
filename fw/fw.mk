# List of all the board related files.
FWSRC = ${MINIECU}/fw/main.c \
	${MINIECU}/fw/pios_crc.c \
	${MINIECU}/fw/pbstx.c \
	${CHIBIOS}/os/various/evtimer.c \
	${MINIECU}/fw/th_comm.c

# Required include directories
FWINC = ${MINIECU}/fw

DDEFS += -DPB_BUFFER_ONLY -DPB_NO_ERRMSG
