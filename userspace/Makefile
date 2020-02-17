# SPDX-License-Identifier: Apache-2.0
# Copyright 2019 Charles University

### Compiler, assembler and linker options
#
# All the options are described in detail in the GCC
# and binutils documentation. A brief description of
# some of the specific ones:
#
# -O2                         ... basic optimizations
# -march=r4000                ... generate code for MIPS R4000
# -mabi=32                    ... use standard 32 bit ABI
# -mgp32                      ... general purpose registers are 32 bit
# -msoft-float                ... do not generate floating point instructions
# -mlong32                    ... ints, long and pointers are 32 bit
# -G 0                        ... threshold for putting objects into small data/bss sections
# -mno-abicalls               ... do not generate SVR4-style position independent code
# -fno-pic                    ... do not generate position independent code using $gp
# -fno-builtin                ... do not recognize built-in functions without __builtin_ prefix
# -ffreestanding              ... a compilation without standard library and main()
# -nostdlib                   ... do not look for standard library in system directories
# -nostdinc                   ... do not look for standard header files in system directories
#


-include ../config.mk
-include Makefile.depend

CCFLAGS = -I$(SRC_BASE)libc/include -O2 -march=r4000 -mabi=32 -mgp32 -msoft-float -mlong32 -G 0 -mno-abicalls -fno-pic -fno-builtin -ffreestanding -nostdlib -nostdinc -pipe -Wall -Wextra -Werror -Wno-unused-parameter -Wmissing-prototypes -g3 -std=c11
ASFLAGS = -I$(SRC_BASE)libc/include -march=r4000 -mabi=32 -mgp32 -msoft-float -mlong32 -G 0 -mno-abicalls -fno-pic -fno-builtin -ffreestanding -nostdlib -nostdinc -pipe -Wall -Wextra -Werror -Wno-unused-parameter -Wmissing-prototypes -g3 -std=gnu99 -I. -D__ASM__
LDFLAGS = -G 0 -static -g -Llibc -lc

### Userspace linker script

LINKER_SCRIPT = $(SRC_BASE)app.lds

EXAMPLE_APP_SOURCES = example/main.c

ifeq ($(USERSPACE_TEST_SOURCES),)
	APP_SOURCES := $(EXAMPLE_APP_SOURCES)
else
	APP_SOURCES := $(USERSPACE_TEST_SOURCES)
endif
	


### Userspace application objects

APP_OBJECTS := $(addsuffix .o,$(basename $(APP_SOURCES)))
APP_RAW = app.raw
APP_MAP = app.map
APP_BINARY = app.bin
APP_DISASM = app.disasm

LIB_DEPS = libc/libc.a

DEPS := $(addsuffix .dep,$(basename $(APP_SOURCES)))

.PHONY: .FORCE all clean distclean dir-tree

all: $(APP_BINARY) $(APP_DISASM)

dir-tree:
	for i in $(APP_OBJECTS); do mkdir -p `dirname $$i`; done

distclean: clean
	rm -f $(APP_BINARY) $(APP_BINARY)
	$(MAKE) -C libc clean

clean:
	rm -f $(APP_DISASM) $(APP_MAP) $(APP_RAW)
	$(MAKE) -C libc distclean
	find . -\( -name '*.o' -o -name '*.dep' -\) -exec rm -f \{\} \;

-include $(DEPS)

%.disasm: %.raw
	$(OBJDUMP) -d $< > $@
	
### Libraries

libc/libc.a: .FORCE
	$(MAKE) -C libc all

### Binary images
#
# The binary files are made in two stages. First, ELF files are
# created from the object files. Next, the binary files are
# created from the ELF files. The reasons for this are:
#
#  - The ELF files contain debugging information.
#
#  - Linking directly to a binary format does
#    not produce errors on missing symbols.
#

%.bin: %.raw
	$(OBJCOPY) -O binary $< $@

$(APP_RAW): $(LINKER_SCRIPT) $(APP_OBJECTS) $(LIB_DEPS)
	$(LD) -T $(LINKER_SCRIPT) -Map $(APP_MAP) -o $@ $(APP_OBJECTS) $(LDFLAGS)

### Default patterns

%.o: %.c | dir-tree
	$(CC) $(CCFLAGS) $(SHARED_EXTRA_CFLAGS) -MD -MF $*.dep -c -o $@ $<

%.o: %.S | dir-tree
	$(CC) $(ASFLAGS) -c -o $@ $<
