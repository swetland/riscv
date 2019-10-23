// Copyright 2019, Brian Swetland <swetland@frotz.net>
// Licensed under the Apache License, Version 2.0.

#pragma once

typedef struct rvstate rvstate_t;

int rvsim_init(rvstate_t** s, void** memory, uint32_t* memsize);

int rvsim_exec(rvstate_t* s, uint32_t pc);

uint32_t rvsim_rd32(rvstate_t* s, uint32_t addr);

uint32_t ior32(uint32_t addr);
void iow32(uint32_t addr, uint32_t val);


