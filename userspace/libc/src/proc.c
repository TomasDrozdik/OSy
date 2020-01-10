// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <np/proc.h>

/** Get information about current process.
 *
 * Note that id can be simply address of the kernel structure, the only
 * requirement is that it shall not change between calls.
 *
 * virt_mem_size shall be the total amount of virtual memory this process
 * has available (i.e. the first invalid address).
 *
 * total_ticks shall represent accounting for the process, however we will
 * accept a solution that only increments the counter with every call to
 * this function.
 *
 * @param info Where to store the retrieved information.
 * @return Whether the call was successful.
 */
bool np_proc_info_get(np_proc_info_t* info) {
    return false;
}
