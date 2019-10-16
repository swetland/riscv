
.globl _start

_start:
	jal ra, main
	jal main
	nop
	mv x3, x4
	j .
