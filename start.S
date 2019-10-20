
#define PASS .long 0x0000000b // _exiti 0
#define FAIL .long 0xFFF0000b // _exiti -1
#define PUTC .long 0x0005500b // _putc a0

.globl _start

_start:
#if 0
	li t1, 0x87654FFF
	PASS
	FAIL
	PUTC
	beqz t1, main0
	bnez t1, main0
	blez t1, main0
	bgez t1, main0
	bltz t1, main0
	bgtz t1, main0
	sltz s1, s2
	sgtz s1, s2
	snez s1, s2
	seqz s1, s2
	not s3, s5
	neg s5, s6
#endif
main0:
	jal main
	PASS
