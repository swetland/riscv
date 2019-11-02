#!/bin/sh -e

git clone git@github.com:riscv/riscv-compliance.git rvc

mkdir -p rvc/riscv-target/rvsim/device/rv32i
mkdir -p rvc/riscv-target/rvsim/device/rv32im

ln -s ../../../compliance_io.h rvc/riscv-target/rvsim/compliance_io.h
ln -s ../../../compliance_test.h rvc/riscv-target/rvsim/compliance_test.h
ln -s ../../../../../makefile.inc rvc/riscv-target/rvsim/device/rv32i/Makefile.include
ln -s ../../../../../makefile.inc rvc/riscv-target/rvsim/device/rv32im/Makefile.include

