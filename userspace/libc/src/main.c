// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <assert.h>

extern int main(void);
extern void __main(void);

/** First C code running in userspace. */
void __main() {
    __asm__ volatile(".insn\n\n.word 0x29\n\n");
    exit(0);

    assert(0 && "unreachable code");
}
