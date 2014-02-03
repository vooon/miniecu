#
#       !!!! Do NOT edit this makefile with an editor which replace tabs by spaces !!!!
#
##############################################################################################
#
# On command line:
#
# make all = Create project
#
# make clean = Clean project files.
#
# To rebuild project do "make clean" and "make all".
#

##############################################################################################
# Start of default section
#

TRGT = 
CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
AS   = $(TRGT)gcc -x assembler-with-cpp
LD   = $(TRGT)gcc

# List all default C defines here, like -D_DEBUG=1
DDEFS = -DSIMULATOR -DSHELL_USE_IPRINTF=FALSE

# List all default ASM defines here, like -D_DEBUG=1
DADEFS =

# List all default directories to look for include files here
DINCDIR =

# List the default directory to look for the libraries here
DLIBDIR =

# List all default libraries here
DLIBS =

#
# End of default section
##############################################################################################

##############################################################################################
# Start of user section
#

# Define project name here
PROJECT = miniecu_sim

# Define linker script file here
LDSCRIPT =

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# Imported source files
MINIECU ?= ../..
CHIBIOS = $(MINIECU)/ext/chibios
BUILDDIR = $(MINIECU)/build
include $(MINIECU)/boards/miniecu_sim/board.mk
include $(CHIBIOS)/os/hal/hal.mk
include ${CHIBIOS}/os/hal/platforms/Posix/platform.mk
include ${CHIBIOS}/os/ports/GCC/SIMIA32/port.mk
include $(CHIBIOS)/os/kernel/kernel.mk
include $(MINIECU)/pb/nanopb.mk
include $(MINIECU)/fw/fw.mk

# List C source files here
CSRC  = ${PORTSRC} \
        ${KERNSRC} \
        ${HALSRC} \
        ${PLATFORMSRC} \
        $(BOARDSRC) \
        $(NANOPBSRC) \
        $(FWSRC)

# List ASM source files here
ASRC =

# List all user directories here
INCDIR = $(PORTINC) $(KERNINC) \
         $(HALINC) $(PLATFORMINC) $(BOARDINC) \
         $(NANOPBINC) $(FWINC) \
         ${CHIBIOS}/os/various

# List the user directory to look for the libraries here
LIBDIR =

# List all user libraries here
LIBS =

# Define optimisation level here
OPT = -ggdb -O2 -fomit-frame-pointer

#
# End of user defines
##############################################################################################

include $(MINIECU)/boards/miniecu_sim/rules.mk
