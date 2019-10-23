#!/bin/sh

export RVSIM=`pwd`/../bin/rvsim
export TOOLCHAIN=`pwd`/../../toolchains/riscv32-elf-7.3.0-Linux-x86_64/bin/riscv32-elf-

make -C rvc RISCV_TARGET=rvsim RISCV_PREFIX=${TOOLCHAIN} TARGET_SIM=${RVSIM}
