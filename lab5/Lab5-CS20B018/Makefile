#
# Internetworking with TCP/IP, Volume III example code Makefile
#
#	David L Stevens, Internetworking Research Group at Purdue
#	Tue Sep 17 19:40:42 EST 1991
#

INCLUDE =

CLNTS = ospf
SERVS = 
OTHER = 

DEFS =
CFLAGS = -W -pedantic -g ${DEFS} ${INCLUDE}
LINKFLAG = -lm

HDR =
CSRC = TCPecho.c TCPdaytime.c TCPtecho.c UDPecho.c UDPtime.c
CXSRC = connectsock.c errexit.c

SSRC = TCPdaytimed.c TCPechod.c TCPmechod.c TCPmtechod.c UDPtimed.c daytimed.c superd.c
SXSRC = passivesock.c sv_funcs.c

CXOBJ = connectsock.o passivesock.o errexit.o
SXOBJ = connectsock.o passivesock.o errexit.o

PROGS = ${CLNTS} ${SERVS} ${OTHER}

all: ${PROGS}

${CLNTS}: ${CXOBJ}
	${CC} -o $@ ${CFLAGS} $@.o ${CXOBJ} ${LINKFLAG}

${SERVS}: ${SXOBJ}
	${CC} -o $@ ${CFLAGS} $@.o ${SXOBJ} ${LINKFLAG}

superd: ${SXOBJ} superd.o sv_funcs.o
	${CC} -o $@ ${CFLAGS} $@.o ${SXOBJ} sv_funcs.o

TCPmtechod: TCPmtechod.o ${SXOBJ}
	${CC} -o $@ ${CFLAGS} $@.o ${SXOBJ} -lpthread

clients: ${CLNTS}
servers: ${SERVS}

clean: FRC
	rm -f Makefile.bak a.out core errs lint.errs ${PROGS} *.o
	rm -f NR* RDS* TDS* ADS*
	rm -f out*

depend: ${HDR} ${CSRC} ${SSRC} ${TNSRC} FRC
	maketd -a ${DEFS} ${INCLUDE} ${CSRC} ${SSRC} ${TNSRC}

install: all FRC
	@echo "Your installation instructions here."

lint: ${HDR} ${XSRC} ${CSRC} ${SSRC} FRC
	lint ${DEFS} ${INCLUDE} ${CSRC} ${SSRC} ${CXSRC} ${SXSRC}

print: Makefile ${SRC} FRC
	lpr Makefile ${CSRC} ${SSRC} ${CXSRC} ${SXSRC}

spotless: clean FRC
	rcsclean Makefile ${HDR} ${SRC}

tags: ${CSRC} ${SSRC} ${CXSRC} ${SXSRC}
	ctags ${CSRC} ${SSRC} ${CXSRC} ${SXSRC}

${HDR} ${CSRC} ${CXSRC} ${SSRC} ${SXSRC}:
	co $@

ospf: ospf.o

FRC:
	
# DO NOT DELETE THIS LINE - maketd DEPENDS ON IT