# riscv
A sandbox for some riscv experiments.

### building the toolchain

It's assumed there's a riscv32 toolchain in a sibling directory.

This recipe grabs @travisg's scripts for building gcc and uses them

```
$ cd ..
$ git clone git@github.com:travisg/toolchains.git
$ cd toolchain
$ ./doit -a riscv32 -j32 -f
```

### building and running the simulator and test program

```
$ make
$ ./bin/rvsim out/hello.bin
```

### running the riscv compliance tests

Check out https://github.com/riscv-non-isa/riscv-arch-test adjacent to this directory.

```
make -f Makefile.test
```

