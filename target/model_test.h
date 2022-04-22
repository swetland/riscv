#ifndef _MODEL_TEST_H
#define _MODEL_TEST_H

// #include "riscv_test.h"

#define RVMODEL_RV32M

#define RVMODEL_BOOT .globl _start; _start:

#define RVMODEL_DATA_BEGIN \
	.align 4; .globl begin_signature; begin_signature:
#define RVMODEL_DATA_END \
	.align 4; .globl end_signature; end_signature:

#define RVMODEL_HALT .long 0x0000000b;
#define RVMODEL_CODE_BEGIN
#define RVMODEL_CODE_END
// .globl _start; \
//_start: \
//begin_testcode: \

#define RVMODEL_IO_INIT
#define RVMODEL_IO_WRITE_STR(_SP, _STR)
#define RVMODEL_IO_CHECK()
#define RVMODEL_IO_ASSERT_GPR_EQ(_SP, _R, _I)
#define RVMODEL_IO_ASSERT_SFPR_EQ(_F, _R, _I)
#define RVMODEL_IO_ASSERT_DFPR_EQ(_D, _R, _I)

#define RVMODEL_SET_MSW_INT
#define RVMODEL_CLEAR_MSW_INT
#define RVMODEL_CLEAR_MTIMER_INT
#define RVMODEL_CLEAR_MEXT_INT

#endif
