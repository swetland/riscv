
#include "iocall.h"

#define EXITI_0   .long 0x0000000b // _exiti 0
#define IOCALL(n) .long 0x0000100b | ((n) << 20) // iocall n

#define MKIOCALL(a,b) .globl a; a: IOCALL(IOCALL_##b); ret

.globl _start
_start:
	jal main

.globl exit
exit:
	EXITI_0

MKIOCALL(dputc,DPUTC)
MKIOCALL(open,OPEN)
MKIOCALL(close,CLOSE)
MKIOCALL(read,READ)
MKIOCALL(write,WRITE)
