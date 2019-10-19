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

typedef struct {
	uint32_t x[32];
	uint32_t pc;
} rvstate;

void rvsim(rvstate* s) {
	char dis[128];
	uint32_t pc, ins;
	pc = s->pc;
	while (pc < 0x80000100) {
		ins = rd32(pc);
		rvdis(pc, ins, dis);
		printf("%08x: %08x %s\n", pc, ins, dis);
		pc += 4;
	}
}


int main(int argc, char** argv) {
	rvstate s;
	if (load_image("out/hello.bin", memory, sizeof(memory)) < 0) return -1;
	memset(&s, 0, sizeof(s));
	s.pc = 0x80000000;
	rvsim(&s);
	return 0;
}

