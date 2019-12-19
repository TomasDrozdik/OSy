// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/proc.h>
#include <np/utest.h>
#include <stddef.h>

/*
 * Checks that accessing invalid memory causes forced process termination.
 */

int main(void) {
    utest_start("procinfo/bad2");

    bool ok = np_proc_info_get((np_proc_info_t*)0x70000000);
    utest_assert(!ok, "bad address shall not abort the program");

    np_proc_info_t info;
    ok = np_proc_info_get(&info);

    utest_assert(ok, "failed to retrieve process information");
    ok = np_proc_info_get((np_proc_info_t*)(info.virt_mem_size - 4));
    utest_assert(!ok, "bad address shall not abort the program");

    utest_passed();

    return 0;
}
