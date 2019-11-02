// Copyright 2019, Brian Swetland <swetland@frotz.net>
// Licensed under the Apache License, Version 2.0.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "riscv.h"
#include "rvsim.h"

#define DO_TRACE_INS     0
#define DO_TRACE_TRAPS   0
#define DO_TRACE_MEM_WR  0
#define DO_TRACE_REG_WR  0

#define RVMEMBASE 0x80000000
#define RVMEMSIZE 32768
#define RVMEMMASK (RVMEMSIZE - 1)

typedef struct rvstate {
	uint32_t x[32];
	void* memory;
	uint32_t mscratch;
	uint32_t mtvec;
	uint32_t mtval;
	uint32_t mepc;
	uint32_t mcause;
	void* ctx;
} rvstate_t;

void* rvsim_dma(rvstate_t* s, uint32_t va, uint32_t len) {
	if (va < RVMEMBASE) return NULL;
	va -= RVMEMBASE;
	if (va >= RVMEMSIZE) return NULL;
	if (len > (RVMEMSIZE - va)) return NULL;
	return s->memory + va;
}

static uint32_t rd32(uint8_t* memory, uint32_t addr) {
	if (addr < RVMEMBASE) {
		return ior32(addr);
	} else {
		addr &= RVMEMMASK;
		return ((uint32_t*) memory)[addr >> 2];
	}
}
static void wr32(uint8_t* memory, uint32_t addr, uint32_t val) {
	if (addr < RVMEMBASE) {
		iow32(addr, val);
	} else {
		addr &= RVMEMMASK;
		((uint32_t*) memory)[addr >> 2] = val;
	}
}
static uint32_t rd16(uint8_t* memory, uint32_t addr) {
	if (addr < RVMEMBASE) {
		return 0xffff;
	} else {
		addr &= RVMEMMASK;
		return ((uint16_t*) memory)[addr >> 1];
	}
}
static void wr16(uint8_t* memory, uint32_t addr, uint32_t val) {
	if (addr >= RVMEMBASE) {
		addr &= RVMEMMASK;
		((uint16_t*) memory)[addr >> 1] = val;
	}
}
static uint32_t rd8(uint8_t* memory, uint32_t addr) {
	if (addr < RVMEMBASE) {
		return 0xff;
	} else {
		addr &= RVMEMMASK;
		return memory[addr];
	}
}
static void wr8(uint8_t* memory, uint32_t addr, uint32_t val) {
	if (addr >= RVMEMBASE) {
		addr &= RVMEMMASK;
		memory[addr] = val;
	}
}

uint32_t rvsim_rd32(rvstate_t* s, uint32_t addr) {
	return rd32(s->memory, addr);
}

int rvsim_init(rvstate_t** _s, void* ctx) {
	rvstate_t *s;
	if ((s = malloc(sizeof(rvstate_t))) == NULL) {
		return -1;
	}
	memset(s, 0, sizeof(rvstate_t));
	if ((s->memory = malloc(RVMEMSIZE)) == NULL) {
		free(s);
		return -1;
	}
	memset(s->memory, 0, RVMEMSIZE);
	s->mtvec = 0x80000000;
	s->ctx = ctx ? ctx : s;
	*_s = s;
	return 0;
}

static inline uint32_t rreg(rvstate_t* s, uint32_t n) {
	return n ? s->x[n] : 0;
}
static inline void wreg(rvstate_t* s, uint32_t n, uint32_t v) {
	s->x[n] = v;
}

static void put_csr(rvstate_t* s, uint32_t csr, uint32_t v) {
	switch (csr) {
	case CSR_MSCRATCH: s->mscratch = v; break;
	case CSR_MTVEC:    s->mtvec = v & 0xFFFFFFFC; break;
	case CSR_MTVAL:    s->mtval = v; break;
	case CSR_MEPC:     s->mepc = v; break;
	case CSR_MCAUSE:   s->mcause = v; break;
	}
}
static uint32_t get_csr(rvstate_t* s, uint32_t csr) {
	switch (csr) {
	case CSR_MISA:      return 0x40000100; // RV32I
	case CSR_MVENDORID: return 0; // NONE
	case CSR_MARCHID:   return 0; // NONE
	case CSR_MIMPID:    return 0; // NONE
	case CSR_MHARTID:   return 0; // Thread Zero
	case CSR_MSCRATCH:  return s->mscratch;
	case CSR_MTVEC:	    return s->mtvec;
	case CSR_MTVAL:	    return s->mtval;
	case CSR_MEPC:	    return s->mepc;
	case CSR_MCAUSE:    return s->mcause;
	default:
		return 0;
	}
}

#define RdR1() rreg(s, get_r1(ins))
#define RdR2() rreg(s, get_r2(ins))
#define RdRd() rreg(s, get_rd(ins))
#define WrRd(v) wreg(s, get_rd(ins), v)

#if DO_TRACE_REG_WR
#define trace_reg_wr(v) do {\
	uint32_t r = get_rd(ins); \
	if (r) { \
	fprintf(stderr, "          (%s = %08x)\n", \
		rvregname(get_rd(ins)), v); \
	}} while (0)
#else
#define trace_reg_wr(v) do {} while (0)
#endif

#if DO_TRACE_MEM_WR
#define trace_mem_wr(a, v) do {\
	fprintf(stderr, "          ([%08x] = %08x)\n", a, v);\
	} while (0)
#else
#define trace_mem_wr(a, v) do {} while (0)
#endif

int rvsim_exec(rvstate_t* s, uint32_t _pc) {
	uint32_t pc = _pc;
	uint32_t next = _pc;
	uint32_t ins;
	uint64_t ccount = 0;
	for (;;) {
		ccount++;
		pc = next;
		ins = rd32(s->memory, pc);
#if DO_TRACE_INS
		char dis[128];
		rvdis(pc, ins, dis);
		fprintf(stderr, "%08x: %08x %s\n", pc, ins, dis);
#endif
		next = pc + 4;
		switch (get_oc(ins)) {
		case OC_LOAD: {
			uint32_t a = RdR1() + get_ii(ins);
			uint32_t v;
			switch (get_fn3(ins)) {
			case F3_LW:
				if (a & 3) goto trap_load_align;
				v = rd32(s->memory, a); break;
			case F3_LHU:
				if (a & 1) goto trap_load_align;
				v = rd16(s->memory, a); break;
			case F3_LBU:
				v = rd8(s->memory, a); break;
			case F3_LH:
				if (a & 1) goto trap_load_align;
				v = rd16(s->memory, a);
				if (v & 0x8000) { v |= 0xFFFF0000; } break;
			case F3_LB:
				v = rd8(s->memory, a);
				if (v & 0x80) { v |= 0xFFFFFF00; } break;
			default:
				goto inval;
			}
			WrRd(v);
			trace_reg_wr(v);
			break;
		trap_load_align:
			s->mcause = EC_L_ALIGN;
			s->mtval = a;
			goto trap_common;
		}
		case OC_CUSTOM_0:
			switch (get_fn3(ins)) {
			case 0b000: // _exiti
				fprintf(stderr, "CCOUNT %lu\n", ccount);
				return get_ii(ins);
			case 0b100: // _exit
				fprintf(stderr, "CCOUNT %lu\n", ccount);
				return RdR1();
			case 0b001: // _iocall
				s->x[10] = iocall(s->ctx, get_ii(ins), s->x + 10);
				break;
			default:
				goto inval;
			}
			break;
		case OC_MISC_MEM:
			switch (get_fn3(ins)) {
			case F3_FENCE:
			case F3_FENCE_I:
				break;
			default:
				goto inval;
			}
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
			case F3_SW:
				if (a & 3) goto trap_store_align;
				wr32(s->memory, a, v); break;
			case F3_SH:
				if (a & 1) goto trap_store_align;
				wr16(s->memory, a, v); break;
			case F3_SB:
				wr8(s->memory, a, v); break;
			default:
				goto inval;
			}
			trace_mem_wr(a, v);
			break;
		trap_store_align:
			s->mcause = EC_S_ALIGN;
			s->mtval = a;
			goto trap_common;
			}
		case OC_OP: {
			uint32_t a = RdR1();
			uint32_t b = RdR2();
			uint32_t n = get_fn3(ins);
			switch (ins >> 25) {
			case 0b0000000:
				switch (n) {
				case F3_ADD: n = a + b; break;
				case F3_SLL: n = a << (b & 31); break;
				case F3_SLT: n = ((int32_t)a) < ((int32_t)b); break;
				case F3_SLTU: n = a < b; break;
				case F3_XOR: n = a ^ b; break;
				case F3_SRL: n = a >> (b & 31); break;
				case F3_OR: n = a | b; break;
				case F3_AND: n = a & b; break;
				}
				break;
			case 0b0000001:
				switch (n) {
				case F3_MUL: n = a * b; break;
				case F3_MULH: n = ((int64_t)(int32_t)a * (int64_t)(int32_t)b) >> 32; break;
				case F3_MULHSU: n = ((int64_t)(int32_t)a * (uint64_t)b) >> 32; break;
				case F3_MULHU: n = ((uint64_t)a * (uint64_t)b) >> 32; break;
				case F3_DIV:
					if (b == 0) { n = 0xffffffff; }
					else if ((a == 0x80000000) && (b == 0xffffffff)) { n = a; }
					else { n = ((int32_t)a / (int32_t)b); }
					break;
				case F3_DIVU:
					if (b == 0) { n = 0xffffffff; }
					else { n = a / b; }
					break;
				case F3_REM:
					if (b == 0) { n = a; }
					else if ((a == 0x80000000) && (b == 0xffffffff)) { n = 0; }
					else { n = ((int32_t)a % (int32_t)b); }
					break;
				case F3_REMU:
					if (b == 0) { n = a; }
					else { n = a % b; }
					break;
				}
				break;
			case 0b0100000:
				switch (n) {
				case F3_SUB: n = a - b; break;
				case F3_SRA: n = ((int32_t)a) >> (b & 31); break;
				default: goto inval;
				}
				break;
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
			if (p) {
				next = pc + get_ib(ins);
				if (next & 3) goto trap_pc_align;
			}
			break;
			}
		case OC_JALR: {
			uint32_t a = (RdR1() + get_ii(ins)) & 0xFFFFFFFE;
			if (get_fn3(ins) != 0) goto inval;
			WrRd(next);
			trace_reg_wr(next);
			next = a;
			if (next & 3) goto trap_pc_align;
			break;
			}
		case OC_JAL:
			WrRd(next);
			trace_reg_wr(next);
			next = pc + get_ij(ins);
			if (next & 3) goto trap_pc_align;
			break;
		case OC_SYSTEM: {
			uint32_t fn = get_fn3(ins);
			if (fn == 0) {
				switch(ins >> 7) {
				case 0b0000000000000000000000000: // ecall
					s->mcause = EC_ECALL_FROM_M;
					s->mtval = 0;
					goto trap_common;
				case 0b0000000000010000000000000: // ebreak
					s->mcause = EC_BREAKPOINT;
					s->mtval = 0;
					goto trap_common;
				case 0b0011000000100000000000000: // mret
					next = s->mepc;
					break;
				default:
					goto inval;
				}
				break;
			}
			uint32_t c = get_iC(ins);
			uint32_t nv = (fn & 4) ? get_ic(ins) : RdR1();
			uint32_t ov = 0;
			switch (fn & 3) {
			case F3_CSRRW:
				// only reads of Rd != x0
				if (get_rd(ins)) ov = get_csr(s, c);
				put_csr(s, c, nv);
				break;
			case F3_CSRRS:
				// only writes if nv != 0
				ov = get_csr(s, c);
				if (nv) put_csr(s, c, ov | nv);
				WrRd(ov);
				break;
			case F3_CSRRC:
				// only writes if nv != 0
				ov = get_csr(s, c);
				if (nv) put_csr(s, c, ov & (~nv));
				break;
			}
			WrRd(ov);
			break;
			}
trap_pc_align:
			s->mcause = EC_I_ALIGN;
			s->mtval = next;
			goto trap_common;
		default:
		inval:
			s->mcause = EC_I_ILLEGAL;
			s->mtval = ins;
#if DO_ABORT_INVAL
			fprintf(stderr,"          (TRAP ILLEGAL %08x)\n", ins);
			return -1;
#endif
trap_common:
			s->mepc = pc;
			next = s->mtvec & 0xFFFFFFFD;
#if DO_TRACE_TRAPS
			fprintf(stderr, "          (TRAP C=%08x V=%08x)\n", s->mcause, s->mtval);
#endif
			break;
		}
	}
}

