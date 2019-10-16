
TOOLCHAIN := ../toolchains/riscv32-elf-7.3.0-Linux-x86_64/bin/riscv32-elf-

CC := $(TOOLCHAIN)gcc
OBJDUMP := $(TOOLCHAIN)objdump
OBJCOPY := $(TOOLCHAIN)objcopy

CFLAGS := -march=rv32i -mabi=ilp32 -O2 
CFLAGS += -ffreestanding -nostdlib
CFLAGS += -Wl,-Bstatic,-T,simple.ld

all: bin/rvsim out/hello.bin out/hello.elf out/hello.lst

out/%.bin: out/%.elf
	@mkdir -p out
	$(OBJCOPY) -O binary $< $@

out/%.lst: out/%.elf
	@mkdir -p out
	$(OBJDUMP) -D $< > $@

HELLO_SRCS := start.S hello.c
out/hello.elf: $(HELLO_SRCS) Makefile
	@mkdir -p out
	$(CC) $(CFLAGS) -o $@ $(HELLO_SRCS)

RVSIM_SRCS := rvsim.c rvdis.c
bin/rvsim: $(RVSIM_SRCS) Makefile gen/instab.h
	@mkdir -p bin
	gcc -O3 -Wall -o $@ $(RVSIM_SRCS)

bin/mkinstab: mkinstab.c
	@mkdir -p bin
	gcc -O3 -Wall -o $@ $<

gen/instab.h: instab.txt bin/mkinstab
	@mkdir -p gen
	bin/mkinstab < instab.txt > gen/instab.h

clean:
	rm -rf out bin gen

