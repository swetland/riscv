// Copyright 2019, Brian Swetland <swetland@frotz.net>
// Licensed under the Apache License, Version 2.0.

#pragma once

#include <stdint.h>

// extract instruction fields
static inline uint32_t get_oc(uint32_t ins) {
	return ins & 0x7f;
}
static inline uint32_t get_rd(uint32_t ins) {
	return (ins >> 7) & 0x1f;
}
static inline uint32_t get_r1(uint32_t ins) {
	return (ins >> 15) & 0x1f;
}
static inline uint32_t get_r2(uint32_t ins) {
	return (ins >> 20) & 0x1f;
}
static inline uint32_t get_ii(uint32_t ins) {
	return ((int32_t)ins) >> 20;
}
static inline uint32_t get_is(uint32_t ins) {
	return ((((int32_t)ins) >> 20) & 0xffffffe0) | ((ins >> 7) & 0x1f);
}
static inline uint32_t get_ib(uint32_t ins) {
	return ((ins >> 7) & 0x1e) |
		((ins >> 20) & 0x7e0) |
		((ins << 4) & 0x800) |
		(((int32_t)(ins & 0x80000000)) >> 20);
}
static inline uint32_t get_iu(uint32_t ins) {
	return ins & 0xFFFFF000;
}
static inline uint32_t get_ij(uint32_t ins) {
	return (((int32_t)(ins & 0x80000000)) >> 11) |
		(ins & 0xFF000) |
		((ins >> 9) & 0x800) |
		((ins >> 20) & 0x7fe);
}
static inline uint32_t get_fn3(uint32_t ins) {
	return (ins >> 12) & 7;
}
static inline uint32_t get_fn7(uint32_t ins) {
	return ins >> 25;
}
static inline uint32_t get_ic(uint32_t ins) {
	return (ins >> 15) & 0x1F;
}
static inline uint32_t get_iC(uint32_t ins) {
	return ins >> 20;
}

// opcode constants (6:0)
#define OC_LOAD     0b0000011
#define OC_CUSTOM_0 0b0001011
#define OC_MISC_MEM 0b0001111
#define OC_OP_IMM   0b0010011
#define OC_AUIPC    0b0010111
#define OC_STORE    0b0100011
#define OC_OP       0b0110011
#define OC_LUI      0b0110111
#define OC_BRANCH   0b1100011
#define OC_JALR     0b1100111
#define OC_JAL      0b1101111
#define OC_SYSTEM   0b1110011

// further discrimination of OC_OP_IMM (14:12)
#define F3_ADDI  0b000
#define F3_SLLI  0b001 // 0b0000000
#define F3_SLTI  0b010
#define F3_SLTIU 0b011
#define F3_XORI  0b100
#define F3_SRLI  0b101 // 0b0000000
#define F3_SRAI  0b101 // 0b0100000
#define F3_ORI   0b110
#define F3_ANDI  0b111

// further discrimination of OC_OP_LOAD (14:12)
#define F3_LB  0b000
#define F3_LH  0b001
#define F3_LW  0b010
#define F3_LBU 0b100
#define F3_LHU 0b101

// further discrimination of OC_OP_STORE (14:12)
#define F3_SB 0b000
#define F3_SH 0b001
#define F3_SW 0b010

// further discrimination of OC_OP (14:12) (fn7==0)
#define F3_ADD  0b0000
#define F3_SLL  0b0001
#define F3_SLT  0b0010
#define F3_SLTU 0b0011
#define F3_XOR  0b0100
#define F3_SRL  0b0101
#define F3_OR   0b0110
#define F3_AND  0b0111
// OC_OP (14:12) (fn7==0b0100000)
#define F3_SUB  0b1000
#define F3_SRA  0b1101

// further discrimination of OC_BRANCH
#define F3_BEQ  0b000
#define F3_BNE  0b001
#define F3_BLT  0b100
#define F3_BGE  0b101
#define F3_BLTU 0b110
#define F3_BGEU 0b111

// further discrimination of OC_SYSTEM (13:12)
#define F3_CSRRW 0b01
#define F3_CSRRS 0b10
#define F3_CSRRC 0b11

// further discrimination of OC_MISC_MEM (14:12)
#define F3_FENCE   0b000
#define F3_FENCE_I 0b001

// CSR values
#define CSR_MVENDORID   0xF11
#define CSR_MARCHID     0xF12
#define CSR_MIMPID      0xF13
#define CSR_MHARTID     0xF14

#define CSR_MSTATUS     0x300
#define CSR_MISA        0x301
#define CSR_MEDELEG     0x302
#define CSR_MIDELEG     0x303
#define CSR_MIE         0x304
#define CSR_MTVEC       0x305
#define CSR_MCOUNTEREN  0x306

#define CSR_MSCRATCH    0x340
#define CSR_MEPC        0x341
#define CSR_MCAUSE      0x342
#define CSR_MTVAL       0x343
#define CSR_MIP         0x344

// exception codes
#define EC_I_ALIGN       0
#define EC_I_ACCESS      1
#define EC_I_ILLEGAL     2
#define EC_BREAKPOINT    3
#define EC_L_ALIGN       4
#define EC_L_ACCESS      5
#define EC_S_ALIGN       6
#define EC_S_ACCESS      7
#define EC_ECALL_FROM_U  8
#define EC_ECALL_FROM_S  9
#define EC_ECALL_FROM_M  11
#define EC_I_PAGEFAULT   12
#define EC_L_PAGEFAULT   13
#define EC_S_PAGEFAULT   15

void rvdis(uint32_t pc, uint32_t ins, char *out);
const char* rvregname(uint32_t n);

