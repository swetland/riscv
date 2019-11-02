// Copyright 2019, Brian Swetland <swetland@frotz.net>
// Licensed under the Apache License, Version 2.0.

#pragma once

typedef struct rvstate rvstate_t;

// initialize simulator
int rvsim_init(rvstate_t** s, void* ctx);

// start simulator running at pc
int rvsim_exec(rvstate_t* s, uint32_t pc);

// obtain a pointer for direct memory access
void* rvsim_dma(rvstate_t* s, uint32_t va, uint32_t len);

// read a word from memory
uint32_t rvsim_rd32(rvstate_t* s, uint32_t addr);


// hook for "syscalls"
uint32_t iocall(void* ctx, uint32_t n, const uint32_t args[8]);

// hooks to implement io read/write access
uint32_t ior32(uint32_t addr);
void iow32(uint32_t addr, uint32_t val);

