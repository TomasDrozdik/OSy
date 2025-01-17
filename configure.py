#!/usr/bin/python3

# SPDX-License-Identifier: Apache-2.0
# Copyright 2019 Charles University

import sys
import os
import argparse
import logging

KERNEL_TEST_EXTRAS = {
    'basic/probe_memory': {
        'CFLAGS': [ '-DKERNEL_TEST_PROBE_MEMORY_MAINMEM_SIZE_KB={mainmem_size}']
    }
}

CONFIG_MK_FMT = '''

TARGET = {toolchain_target}
TOOLCHAIN_DIR = {toolchain_dir}

CC = $(TOOLCHAIN_DIR)/bin/$(TARGET)-gcc
LD = $(TOOLCHAIN_DIR)/bin/$(TARGET)-ld
OBJCOPY = $(TOOLCHAIN_DIR)/bin/$(TARGET)-objcopy
OBJDUMP = $(TOOLCHAIN_DIR)/bin/$(TARGET)-objdump
GDB = $(TOOLCHAIN_DIR)/bin/$(TARGET)-gdb

KERNEL_TEST_SOURCES = {kernel_test_sources}
KERNEL_EXTRA_CFLAGS = {kernel_extra_cflags}

'''

BUILD_DIR_MAKEFILE_FMT = '''

SRC_BASE = {src_base}/

vpath %.S $(SRC_BASE)
vpath %.c $(SRC_BASE)
vpath %.h $(SRC_BASE)

include $(SRC_BASE)/Makefile

'''

MSIM_CONF_FMT = '''

# Generated by configure.py

# Single processor machine
add dcpu cpu0

# Setup main memory for kernel
add rwm mainmem 0
mainmem generic {ram_size}K
mainmem load "kernel/kernel.bin"

# Setup memory for the loader
# (R4000 starts at hardwired address 0x1FC00000).
add rom loadermem 0x1FC00000
loadermem generic 4K
loadermem load "kernel/loader.bin"

# Console printer
add dprinter printer 0x10000000

'''

MIPSEL_TARGETS = ['mipsel-linux-gnu', 'mipsel-unknown-linux-gnu']

def copy_to_file(inp_fd, out_name, out_mode):
    with open(out_name, out_mode) as out:
        while True:
            buffer = inp_fd.read(512)
            if buffer == '':
                break
            out.write(buffer)

def has_gcc_installed(prefix, target):
    gcc_path = os.path.join(prefix, 'bin', '{}-gcc'.format(target))
    return os.path.exists(gcc_path)

def main(argv):
    args = argparse.ArgumentParser(description='Configure the build')
    args.add_argument('--verbose',
        default=False,
        dest='verbose',
        action='store_true',
        help='Be verbose.'
    )
    args.add_argument('-f', '--force',
        default=False,
        dest='force',
        action='store_true',
        help='Overwrite existing files.'
    )
    args.add_argument('--toolchain',
        default=None,
        dest='toolchain_dir',
        help='Toolchain directory.'
    )
    args.add_argument('--target',
        default=None,
        dest='toolchain_target',
        help='Toolchain target triplet.'
    )
    args.add_argument('--debug',
        default=False,
        dest='debug',
        action='store_true',
        help='Build kernel in debug mode.'
    )
    args.add_argument('--kernel-test',
        default=None,
        dest='kernel_test',
        help='Kernel test (name).'
    )
    args.add_argument('--memory-size',
        default=None,
        dest='memory_size',
        type=int,
        help='Base memory size (msim.conf) in KB'
    )
    config = args.parse_args(argv[1:])
    
    if config.verbose:
        config.logging_level = logging.DEBUG
    else:
        config.logging_level = logging.INFO
    
    logging.basicConfig(format='[%(asctime)s %(name)-6s %(levelname)7s] %(message)s', level=config.logging_level)
    logger = logging.getLogger('main')
    
    # Get basic paths
    src_base = os.path.realpath(os.path.dirname(argv[0]))
    cwd = os.path.realpath(os.getcwd())
    is_out_of_tree_build = src_base != cwd

    # Cannot set memory size in in-tree build
    # (we do not want to modify msim.conf in place).
    if (config.memory_size is not None) and (not is_out_of_tree_build):
        logger.critical('Cannot change memory size when building in source tree.')
        sys.exit(1)
    if config.memory_size is None:
        config.memory_size = 1024

    # Detect toolchain
    if config.toolchain_dir is None:
        for tc_dir in ['/opt/mff-nswi004/', os.path.expanduser("~/.local/"), '/usr/']:
            if config.toolchain_target is None:
                tc_possible_targets = MIPSEL_TARGETS
            else:
                tc_possible_targets = [config.toolchain_target]
            for tc_target in tc_possible_targets:
                if has_gcc_installed(tc_dir, tc_target):
                    config.toolchain_dir = tc_dir
                    if config.toolchain_target is None:
                        config.toolchain_target = tc_target
    else:
        config.toolchain_dir = os.path.realpath(config.toolchain_dir)
        if config.toolchain_target is None:
            for tc_target in MIPSEL_TARGETS:
                if has_gcc_installed(config.toolchain_dir, tc_target):
                    config.toolchain_target = tc_target

    if config.toolchain_dir is None:
        logger.critical("Failed to locate cross-compiler toolchain.")
        logger.critical("Have you installed the toolchain and/or have you specified --toolchain?")
        sys.exit(1)
    if config.toolchain_target is None:
        logger.critical("Failed to determine cross-compiler target.")
        logger.critical("Have you installed the toolchain and/or have you specified --target?")
        sys.exit(1)

    if not has_gcc_installed(config.toolchain_dir, config.toolchain_target):
        logger.critical("GCC for {} not found in {}.".format(config.toolchain_target, config.toolchain_dir))
        logger.critical("Have you installed the toolchain?")
        sys.exit(1)

    # Always create config.mk
    logger.debug('Creating config.mk')
    with open('config.mk', 'w') as f:
        kernel_test_sources = ''
        kernel_extra_cflags = []
        if config.debug:
            kernel_extra_cflags.append('-DKERNEL_DEBUG')
        if not (config.kernel_test is None):
            kernel_test_sources = 'tests/{}/test.c'.format(config.kernel_test)
            kernel_extra_cflags.append('-DKERNEL_TEST')
            if config.kernel_test in KERNEL_TEST_EXTRAS:
                test_extras = KERNEL_TEST_EXTRAS[config.kernel_test]
                if 'CFLAGS' in test_extras:
                    for i in test_extras['CFLAGS']:
                        kernel_extra_cflags.append(i.format(
                            mainmem_size=config.memory_size
                        ))
        f.write(CONFIG_MK_FMT.format(
            toolchain_dir=config.toolchain_dir,
            toolchain_target=config.toolchain_target,
            kernel_test_sources=kernel_test_sources,
            kernel_extra_cflags=' '.join(kernel_extra_cflags),
        ))

    # Create rest of files only when building in different directory
    if is_out_of_tree_build:
        # Create Makefiles
        for sub in ['.', 'kernel', 'user']:
            logger.debug('Creating Makefile inside {}/'.format(sub))
            os.makedirs(sub, exist_ok=True)
            with open(os.path.join(cwd, sub, 'Makefile'), 'w') as f:
                f.write(BUILD_DIR_MAKEFILE_FMT.format(
                    src_base=os.path.join(src_base, sub)
                ))

        # Do not overwrite these
        for f in ['gdbinit']:
            logger.debug('Copying {}'.format(f))
            with open(os.path.join(src_base, f), 'r') as inp:
                try:
                    copy_to_file(inp, f, 'x')
                except FileExistsError:
                    if config.force:
                        logger.warning('Overwriting {}'.format(f))
                        copy_to_file(inp, f, 'w')
                    else:
                        logger.error('Refusing to overwrite {}'.format(f))

        # Write msim.conf
        msim_conf_data = MSIM_CONF_FMT.format(
            ram_size=config.memory_size
        )
        msim_conf_path = os.path.join(cwd, 'msim.conf')
        try:
            with open(msim_conf_path, 'x') as f:
                f.write(msim_conf_data)
        except FileExistsError:
            if config.force:
                logger.warning('Overwriting msim.conf')
                with open(msim_conf_path, 'w') as f:
                    f.write(msim_conf_data)
            else:
                logger.error('Refusing to overwrite msim.conf')

if __name__ == "__main__":
    main(sys.argv)
