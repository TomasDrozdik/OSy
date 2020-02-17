// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <debug.h>
#include <mm/tlb.h>
#include <proc/thread.h>
#include <utils.h>

#define PAGE_MASK 0xFFFFF000

/* Check whether given vpn is mapped in given address space, potentionally
 * returning corresponding pfn.
 *
 * @param as Address space to look in.
 * @param vpn Virtual page number of looked for page.
 * @param pfn Output parameter, pointer to resulting pfn.
 * @returns True if address is mapped withing given AS and this mapping is in
 *          corresponding pfn, false otherwise with pfn undefined.
 */
static bool is_mapped(as_t* as, uintptr_t vpn, uintptr_t* pfn) {
    errno_t err = as_get_mapping(as, vpn << 12, pfn);
    switch (err) {
    case EOK:
        // pfn now contains physical address, shift-right by 12 discards page
        // addressing bits resulting in pfn.
        assert(*pfn == ((*pfn) & PAGE_MASK));
        *pfn >>= 12;
        return true;
    case EINVAL:
        panic("TLB refill: requested virtual address not aligned to PAGE_SIZE");
        break;
    case ENOENT:
        return false;
    default:
        assert(0 && "as_get_mapping: unknown errno");
    }

    assert(0 && "Invalid code path.");
    return false;
}

void handle_tlb_refill(context_t* context) {
    thread_t* thread = thread_get_current();

    // Addresses for TLB numbering corrensponds to 2 PFNs.
    // virt1 corresponds to VPN2 with virt2 following.
    uintptr_t vpn2;
    uintptr_t pfn1, pfn2;
    bool valid1, valid2;

    vpn2 = context->badva >> 13;
    valid1 = is_mapped(thread->as, vpn2 << 1, &pfn1);
    valid2 = is_mapped(thread->as, (vpn2 << 1) + 1, &pfn2);

    if (!valid1 && !valid2) {
        thread_kill(thread);
    }

    const bool dirty = true;
    const bool global = false;
    cp0_write_pagemask_4k();
    cp0_write_entrylo0(pfn1, dirty, valid1, global);
    cp0_write_entrylo1(pfn2, dirty, valid2, global);
    cp0_write_entryhi(vpn2, thread->as->asid);
    cp0_tlb_write_random();
}

void invalidate_tlb(uint8_t asid) {
    const bool global = false;
    const bool valid = false;
    const bool dirty = true;
    const uintptr_t vpn2 = 0;
    const uintptr_t pfn = 0;

    for (size_t i = 0; i < TLB_ENTRY_COUNT; ++i) {
        if (cp0_tlb_read_entry_asid(i) == asid) {
            cp0_write_pagemask_4k();
            cp0_write_entrylo0(pfn, dirty, valid, global);
            cp0_write_entrylo1(pfn, dirty, valid, global);
            cp0_write_entryhi(vpn2, INVALID_ASID);
            cp0_write_index(i);
            cp0_tlb_write_indexed();
        }
    }
}
