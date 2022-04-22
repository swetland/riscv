// Copyright 2019, Brian Swetland <swetland@frotz.net>
// Licensed under the Apache License, Version 2.0.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "rvsim.h"
#include "iocall.h"

uint32_t ior32(uint32_t addr) {
        return 0xffffffff;
}

void iow32(uint32_t addr, uint32_t val) {
}

uint32_t iocall(void* ctx, uint32_t n, const uint32_t args[8]) {
	rvstate_t* s = ctx;
	switch (n) {
	case IOCALL_DPUTC: {
		uint8_t x = args[0];
		if (write(1, &x, 1)) { return -1; }
		return 0;
	}
	case IOCALL_OPEN: { // (path, flags, mode) -> fd/error
		void* ptr = rvsim_dma(s, args[0], 1024);
		if (ptr == NULL) return -1;
		if (memchr(ptr, 0, 1024) == NULL) return -1;
		return open(ptr, args[1], args[2]);
	}
	case IOCALL_CLOSE: { // (fd) -> 0/error
		return close(args[0]);
	}
	case IOCALL_READ: { // (fd, ptr, len) -> len/error
		void* ptr = rvsim_dma(s, args[1], args[2]);
		if (ptr == NULL) return -1;
		return read(args[0], ptr, args[2]);
	}
	case IOCALL_WRITE: { // (fd, ptr, len) -> len/error
		void* ptr = rvsim_dma(s, args[1], args[2]);
		if (ptr == NULL) return -1;
		return write(args[0], ptr, args[2]);
	}
	default:
		return -1;
	}
}

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
	void* memory;
	uint32_t membase = 0x80000000;
	uint32_t memsize = 0x01000000;
	rvstate_t* s;

	if (rvsim_init(&s, NULL)) {
		fprintf(stderr, "error: cannot initialize simulator\n");
		return -1;
	}
	if ((memory = rvsim_dma(s, membase, memsize)) == NULL) {
		fprintf(stderr, "error: cannot access sim memory\n");
		return -1;
	}
	if (load_image(fn, memory, memsize) < 0) {
		fprintf(stderr, "error: failed to load '%s'\n", fn);
		return -1;
	}
	rvsim_exec(s, membase);

	if (dumpfn && (dumpto > dumpfrom)) {
		FILE* fp;
		if ((fp = fopen(dumpfn, "w")) == NULL) {
			fprintf(stderr, "error: failed to open '%s' to write\n", dumpfn);
			return -1;
		}
		for (uint32_t n = dumpfrom; n < dumpto; n += 4) {
			uint32_t v = rvsim_rd32(s, n);
			fprintf(fp, "%08x\n", v);
		}
		fclose(fp);
	}
	return 0;
}

