// Copyright 2019, Brian Swetland <swetland@frotz.net>
// Licensed under the Apache License, Version 2.0.

#include <string.h>
#include <stdio.h>

#include "riscv.h"

static char *append_str(char *buf, const char *s) {
	while (*s) *buf++ = *s++;
	return buf;
}

static char *append_i32(char *buf, int32_t n) {
	return buf + sprintf(buf, "%d", n);
}

static char *append_u32(char *buf, int32_t n) {
	return buf + sprintf(buf, "0x%x", n);
}

static const char* regname_plain[32] = {
	"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
	"x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
	"x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
	"x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31",
};
static const char* regname_fancy[32] = {
	"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
	"s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
	"a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
	"s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
};

static const char** regname = regname_plain;

const char* rvregname(uint32_t n) {
	if (n < 32) {
		return regname[n];
	} else {
		return "??";
	}
}


typedef struct {
	uint32_t mask;
	uint32_t bits;
	const char* fmt;
} rvins_t;

static rvins_t instab[] = {
#include "gen/instab.h"
};

void rvdis(uint32_t pc, uint32_t ins, char *out) {
	unsigned n = 0;
	while ((ins & instab[n].mask) != instab[n].bits) n++;
	const char* fmt = instab[n].fmt;
	char c;
	while ((c = *fmt++) != 0) {
		if (c != '%') {
			*out++ = c;
			continue;
		}
		switch (*fmt++) {
		case '1': out = append_str(out, regname[get_r1(ins)]); break;
		case '2': out = append_str(out, regname[get_r2(ins)]); break;
		case 'd': out = append_str(out, regname[get_rd(ins)]); break;
		case 'i': out = append_i32(out, get_ii(ins)); break;
		case 'j': out = append_i32(out, get_ij(ins)); break;
		case 'J': out = append_u32(out, pc + get_ij(ins)); break;
		case 'b': out = append_i32(out, get_ib(ins)); break;
		case 'B': out = append_u32(out, pc + get_ib(ins)); break;
		case 's': out = append_i32(out, get_is(ins)); break;
		case 'u': out = append_i32(out, get_iu(ins)); break;
		case 'U': out = append_u32(out, get_iu(ins)); break;
		case 'x': out = append_i32(out, get_r2(ins)); break;
		}
	}
	*out = 0;
}
