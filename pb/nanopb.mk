# Path to the nanopb root directory
NANOPBDIR := $(MINIECU)/ext/nanopb

PROTODIR = ${MINIECU}/build/pb

# Files for the nanopb core
NANOPBSRC = ${NANOPBDIR}/pb_encode.c \
	    ${NANOPBDIR}/pb_decode.c \
	    ${PROTODIR}/miniecu.pb.c \
	    ${PROTODIR}/flash.pb.c

NANOPBINC = ${NANOPBDIR} \
	    ${PROTODIR}
