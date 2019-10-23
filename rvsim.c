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
uint32_t rd16(uint32_t addr) {
	if (addr < 0x80000000) {
		return 0xffff;
	} else {
		addr &= (sizeof(memory) - 1);
		return ((uint16_t*) memory)[addr >> 1];
	}
}
void wr16(uint32_t addr, uint32_t val) {
	if (addr >= 0x80000000) {
		addr &= (sizeof(memory) - 1);
		((uint16_t*) memory)[addr >> 1] = val;
	}
}
uint32_t rd8(uint32_t addr) {
	if (addr < 0x80000000) {
		return 0xff;
	} else {
		addr &= (sizeof(memory) - 1);
		return memory[addr];
	}
}
void wr8(uint32_t addr, uint32_t val) {
	if (addr >= 0x80000000) {
		addr &= (sizeof(memory) - 1);
		memory[addr] = val;
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
#define RdRd() rreg(s, get_rd(ins))
#define WrRd(v) wreg(s, get_rd(ins), v)

#define DO_DISASM 1
#define DO_TRACK  1

#define trace_reg(fmt...) printf(fmt...)

#define trace_reg_wr(v) do {\
	uint32_t r = get_rd(ins); \
	if (r) { \
	printf("          (%s = %08x)\n", \
		rvregname(get_rd(ins)), v); \
	}} while (0)

#define trace_mem_wr(a, v) do {\
	printf("          ([%08x] = %08x)\n", a, v);\
	} while (0)

void rvsim(rvstate_t* s) {
	uint32_t pc = s->pc;
	uint32_t next = pc;
	uint32_t ccount = 0;
	uint32_t ins;
	for (;;) {
		pc = next;
		ins = rd32(pc);
#if DO_DISASM
		char dis[128];
		rvdis(pc, ins, dis);
		printf("%08x: %08x %s\n", pc, ins, dis);
#endif
		next = pc + 4;
		ccount++;
		switch (get_oc(ins)) {
		case OC_LOAD: {
			uint32_t a = RdR1() + get_ii(ins);
			uint32_t v;
			switch (get_fn3(ins)) {
			case F3_LW: v = rd32(a); break;
			case F3_LHU: v = rd16(a); break;
			case F3_LBU: v = rd8(a); break;
			case F3_LH: v = rd16(a);
				if (v & 0x8000) { v |= 0xFFFF0000; } break;
			case F3_LB: v = rd8(a);
				if (v & 0x80) { v |= 0xFFFFFF00; } break;
			default:
				goto inval;
			}
			WrRd(v);
			trace_reg_wr(v);
			break;
		}
		case OC_CUSTOM_0:
			goto inval;
		case OC_MISC_MEM:
			if (get_fn3(ins) != 0) goto inval;
			// fence -- do nothing
			break;
		case OC_OP_IMM: {
			uint32_t a = RdR1();
			uint32_t b = get_ii(ins);
			uint32_t n = 0xe1e1e1e1;
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
			trace_reg_wr(n);
			break;
			}
		case OC_AUIPC: {
			uint32_t v = pc + get_iu(ins);
			WrRd(v);
			trace_reg_wr(v);
			break;
			}
		case OC_STORE: {
			uint32_t a = RdR1() + get_is(ins);
			uint32_t v = RdR2();
			switch (get_fn3(ins)) {
			case F3_SW: wr32(a, v); break;
			case F3_SH: wr16(a, v); break;
			case F3_SB: wr8(a, v); break;
			default:
				goto inval;
			}
			trace_mem_wr(a, v);
			break;
			}
		case OC_OP: {
			uint32_t a = RdR1();
			uint32_t b = RdR2();
			uint32_t n;
			if (ins & 0xBE000000) goto inval;
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
			trace_reg_wr(n);
			break;
			}
		case OC_LUI: {
			uint32_t v = get_iu(ins);
			WrRd(v);
			trace_reg_wr(v);
			break;
			}
		case OC_BRANCH: {
			uint32_t a = RdR1();
			uint32_t b = RdR2();
			int32_t p;
			switch (get_fn3(ins)) {
			case F3_BEQ: p = (a == b); break;
			case F3_BNE: p = (a != b); break;
			case F3_BLT: p = (((int32_t)a) < ((int32_t)b)); break;
			case F3_BGE: p = (((int32_t)a) >= ((int32_t)b)); break;
			case F3_BLTU: p = (a < b); break;
			case F3_BGEU: p = (a >= b); break;
			default:
				goto inval;
			}
			if (p) next = pc + get_ib(ins);
			break;
			}
		case OC_JALR: {
			uint32_t a = (RdR1() + get_ii(ins)) & 0xFFFFFFFE;
			if (get_fn3(ins) != 0) goto inval;
			WrRd(next);
			trace_reg_wr(next);
			next = a;
			break;
			}
		case OC_JAL:
			WrRd(next);
			trace_reg_wr(next);
			next = pc + get_ij(ins);
			break;
		case OC_SYSTEM:
			goto inval;
		default:
		inval:
			return;
		}
	}
}


int main(int argc, char** argv) {
	const char* fn = NULL;
	const char* dumpfn = NULL;
	uint32_t dumpfrom = 0, dumpto = 0;
	while (argc > 1) {
		argc--;
		argv++;
		if (argv[0][0] != '-') {
			if (fn != NULL) {
				fprintf(stderr, "error: multiple inputs\n");
				return -1;
			}
			fn = argv[0];
			continue;
		}
		if (!strncmp(argv[0],"-dump=",6)) {
			dumpfn = argv[0] + 6;
			continue;
		}
		if (!strncmp(argv[0],"-from=",6)) {
			dumpfrom = strtoul(argv[0] + 6, NULL, 16);
			continue;
		}
		if (!strncmp(argv[0],"-to=",4)) {
			dumpto = strtoul(argv[0] + 4, NULL, 16);
			continue;
		}
		fprintf(stderr, "error: unknown argument: %s\n", argv[0]);
		return -1;
	}
	rvstate_t s;
	if (load_image(fn, memory, sizeof(memory)) < 0) {
		fprintf(stderr, "error: failed to load '%s'\n", fn);
		return -1;
	}
	memset(&s, 0, sizeof(s));
	s.pc = 0x80000000;
	rvsim(&s);
	if (dumpfn && (dumpto > dumpfrom)) {
		FILE* fp;
		if ((fp = fopen(dumpfn, "w")) == NULL) {
			fprintf(stderr, "error: failed to open '%s' to write\n", dumpfn);
			return -1;
		}
		for (uint32_t n = dumpfrom; n < dumpto; n += 4) {
			uint32_t v = rd32(n);
			fprintf(fp, "%08x\n", v);
		}
		fclose(fp);
	}
	return 0;
}

