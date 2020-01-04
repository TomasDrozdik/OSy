# SPDX-License-Identifier: Apache-2.0
# Copyright 2019 Charles University

GDB_PORT = 10001
DIFF = diff

-include config.mk

### Phony targets

.PHONY: all clean distclean kernel run-msim-gdb run-gdb cstyle fix-cstyle test-all

SUITE_ALL=suite_all.txt


### Default target

all: kernel

kernel:
	$(MAKE) -C kernel

clean:
	$(MAKE) -C kernel clean

distclean:
	$(MAKE) -C kernel distclean
	rm -f config.mk
	rm -rf _build_kernel__*

run-msim-gdb:
	msim -g $(GDB_PORT)

run-gdb:
	$(GDB) -ix gdbinit -iex "target remote :$(GDB_PORT)"

cstyle:
	find kernel/ -name '*.[ch]' | while read fname; do clang-format -style=file "$$fname" | $(DIFF) -ud "$$fname" -; done

fix-cstyle:
	find kernel/ -name '*.[ch]' -exec clang-format -style=file -i {} \;

$(SUITE_ALL): suite_as1.txt suite_as2.txt suite_as3.txt suite_as4.txt suite_as5.txt
		cat $^ > $@

test-all: $(SUITE_ALL)
	./tools/tester.py suite $<

