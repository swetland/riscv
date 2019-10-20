// Copyright 2019, Brian Swetland <swetland@frotz.net>
// Licensed under the Apache License, Version 2.0.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "riscv.h"

static const char* regname[32] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
};

uint8_t memory[32768];

int load_image(const char* fn, uint8_t* ptr, size_t sz) {
	struct stat s;
	int fd = open(fn, O_RDONLY);
	if (fd < 0) return -1;
	if (fstat(fd, &s) < 0) return -1;
	if (s.st_size > sz) return -1;
	sz = s.st_size;
	while (sz > 0) {
		ssize_t r = read(fd, ptr, sz);
		if (r <= 0) {
			close(fd);
			return -1;
		}
		ptr += r;
		sz -= r;
	}
	close(fd);
	fprintf(stderr, "image: %ld bytes\n", s.st_size);
	return 0;
}

uint32_t ior32(uint32_t addr) {
	return 0xffffffff;
}

void iow32(uint32_t addr, uint32_t val) {
}

uint32_t rd32(uint32_t addr) {
	if (addr < 0x80000000) {
		return ior32(addr);
	} else {
		addr &= (sizeof(memory) - 1);
		return ((uint32_t*) memory)[addr >> 2];
	}
}

void wr32(uint32_t addr, uint32_t val) {
	if (addr < 0x80000000) {
		iow32(addr, val);
	} else {
		addr &= (sizeof(memory) - 1);
		((uint32_t*) memory)[addr >> 2] = val;
	}
}

typedef struct {
	uint32_t x[32];
	uint32_t pc;
} rvstate_t;

static inline uint32_t rreg(rvstate_t* s, uint32_t n) {
	return n ? s->x[n] : 0;
}
static inline void wreg(rvstate_t* s, uint32_t n, uint32_t v) {
	s->x[n] = v;
}


#define RdR1() rreg(s, get_r1(ins))
#define RdR2() rreg(s, get_r2(ins))
#define WrRd(v) wreg(s, get_rd(ins), v)

#define DO_DISASM 1
#define DO_TRACK  1

void rvsim(rvstate_t* s) {
#if DO_TRACK
	uint32_t last[32];
#endif
	uint32_t pc = s->pc;
	uint32_t next = pc;
	uint32_t ccount = 0;
	uint32_t ins;
	for (;;) {
		pc = next;
#if DO_TRACK
		memcpy(last, &s->x, sizeof(last));
#endif
		ins = rd32(pc);
#if DO_DISASM
		char dis[128];
		rvdis(pc, ins, dis);
		printf("%08x: %08x %s\n", pc, ins, dis);
#endif
		next = pc + 4;
		ccount++;
		switch (get_oc(ins)) {
		case OC_LOAD:
			switch (get_fn3(ins)) {
			case F3_LW:
				WrRd(rd32(RdR1() + get_ii(ins)));
				break;
			default:
				goto inval;
			}
			break;
		case OC_CUSTOM_0:
			goto inval;
		case OC_MISC_MEM:
			if (get_fn3(ins) != 0) goto inval;
			// fence -- do nothing
			break;
		case OC_OP_IMM: {
			uint32_t a = RdR1();
			uint32_t b = get_ii(ins);
			uint32_t n;
			switch (get_fn3(ins)) {
			case F3_ADDI: n = a + b; break;
			case F3_SLLI:
				if (b & 0b111111100000) goto inval;
				n = a << b; break;
			case F3_SLTI: n = ((int32_t)a) < ((int32_t)b); break;
			case F3_SLTIU: n = a < b; break;
			case F3_XORI: n = a ^ b; break;
			case F3_SRLI:
				if (b & 0b101111100000) goto inval;
				if (b & 0b010000000000) {
					n = ((int32_t)a) >> b;
				} else {
					n = a >> b;
				}
				break;
			case F3_ORI: n = a | b; break;
			case F3_ANDI: n = a & b; break;
			}
			WrRd(n);
			break;
			}
		case OC_AUIPC:
			WrRd(pc + get_iu(ins));
			break;
		case OC_STORE:
			switch (get_fn3(ins)) {
			case F3_SW:
				wr32(RdR2() + get_is(ins), RdR1());
				break;
			default:
				goto inval;
			}
		case OC_OP: {
			uint32_t a = RdR1();
			uint32_t b = RdR1();
			uint32_t n;
			if (ins & 0xDE000000) goto inval;
			switch (get_fn3(ins) | (ins >> 27)) {
			case F3_ADD: n = a + b; break;
			case F3_SLL: n = a << (b & 31); break;
			case F3_SLT: n = ((int32_t)a) < ((int32_t)b); break;
			case F3_SLTU: n = a < b; break;
			case F3_XOR: n = a ^ b; break;
			case F3_SRL: n = a >> (b & 31); break;
			case F3_OR: n = a | b; break;
			case F3_AND: n = a & b; break;
			case F3_SUB: n = a - b; break;
			case F3_SRA: n = ((int32_t)a) >> (b & 31); break;
			default: goto inval;
			}
			WrRd(n);
			break;
			}
		case OC_LUI:
			WrRd(get_iu(ins));
			break;
		case OC_BRANCH: {
			uint32_t a = RdR1();
			uint32_t b = RdR2();
			int32_t p;
			switch (get_fn3(ins)) {
			case F3_BEQ: p = (a == b); break;
			case F3_BNE: p = (a != b); break;
			case F3_BLT: p = (((int32_t)a) == ((int32_t)b)); break;
			case F3_BGE: p = (((int32_t)a) == ((int32_t)b)); break;
			case F3_BLTU: p = (a <= b); break;
			case F3_BGEU: p = (a >= b); break;
			default:
				goto inval;
			}
			if (p) next = pc + (get_ib(ins) << 1);
			break;
			}
		case OC_JALR:
			if (get_fn3(ins) != 0) goto inval;
			WrRd(next);
			next = RdR1() + (get_ii(ins) << 1);
			break;
		case OC_JAL:
			WrRd(next);
			next = pc + get_ij(ins);
			break;
		case OC_SYSTEM:
			goto inval;
		default:
		inval:
			return;
		}
#if DO_TRACK
		for (unsigned n = 1; n < 32; n++) {
			if (s->x[n] != last[n]) {
				printf("          (%s = 0x%08x)\n",
					regname[n], s->x[n]);
			}
		}
#endif
	}
}


int main(int argc, char** argv) {
	rvstate_t s;
	if (load_image("out/hello.bin", memory, sizeof(memory)) < 0) return -1;
	memset(&s, 0, sizeof(s));
	s.pc = 0x80000000;
	rvsim(&s);
	return 0;
}

