TARGET_SIM ?= /work/riscv/riscv/bin/rvsim
ifeq ($(shell command -v $(TARGET_SIM) 2> /dev/null),)
    $(error Target simulator executable '$(TARGET_SIM)` not found)
endif

RUN_TARGET=\
     $(TARGET_SIM) \
    $(work_dir_isa)/$<.bin \
    -dump=$(work_dir_isa)/$(*).signature.output \
    -from=$$(grep begin_signature $(work_dir_isa)/$<.map | awk '{print $$1}') \
    -to=$$(grep end_signature $(work_dir_isa)/$<.map | awk '{print $$1}') \
    2> $(work_dir_isa)/$@ 1>&2

RISCV_PREFIX   ?= riscv32-unknown-elf-
RISCV_GCC      ?= $(RISCV_PREFIX)gcc
RISCV_OBJDUMP  ?= $(RISCV_PREFIX)objdump
RISCV_OBJCOPY  ?= $(RISCV_PREFIX)objcopy
RISCV_NM       ?= $(RISCV_PREFIX)nm
RISCV_GCC_OPTS ?= -static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles

COMPILE_TARGET=\
	$$(RISCV_GCC) $(2) $$(RISCV_GCC_OPTS) \
		-I$(ROOTDIR)/riscv-test-env/ \
		-I$(ROOTDIR)/riscv-test-env/p/ \
		-I$(TARGETDIR)/$(RISCV_TARGET)/ \
		-T$(ROOTDIR)/riscv-test-env/p/link.ld $$< \
		-o $(work_dir_isa)/$$@; \
	$$(RISCV_OBJDUMP) -D $(work_dir_isa)/$$@ > $(work_dir_isa)/$$@.objdump; \
	$$(RISCV_OBJCOPY) -O binary $(work_dir_isa)/$$@ $(work_dir_isa)/$$@.bin; \
	$$(RISCV_NM) $(work_dir_isa)/$$@ > $(work_dir_isa)/$$@.map