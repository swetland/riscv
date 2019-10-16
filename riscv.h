#pragma once

#include <stdint.h>

// extract instruction fields
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
		(((int32_t)ins) >> 19);
}
static inline uint32_t get_iu(uint32_t ins) {
	return ins & 0xFFFFF000;
}
static inline uint32_t get_ij(uint32_t ins) {
	return (((int32_t)ins) >> 11) |
		(ins & 0xFF000) |
		((ins >> 9) & 0x800) |
		((ins >> 20) & 0x7fe);
}

#define OP_LUI 0b0110111
#define OP_AUIPC 0b0010111
#define OP_JAL 0b1101111
#define OP_JALR 0b1100111
#define OP_B

void rvdis(uint32_t ins, char *out);
