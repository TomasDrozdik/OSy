// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <mm/tlb.h>
#include <debug.h>
#include <proc/thread.h>
#include <utils.h>

#define PAGE_MASK 0xFFFFF000

static bool is_mapped(as_t* as, uintptr_t vpn, uintptr_t *pfn) {
    errno_t err = as_get_mapping(as, vpn << 12, pfn);
    switch (err) {
    case EINVAL:
        panic("Tlb refill: requested virtual address not aligned to PAGE_SIZE");
        break;
    case ENOENT:
        return false;
    case EOK:
        assert(*pfn == ((*pfn) & PAGE_MASK));
        *pfn >>= 12;
        return true;
    default:
        panic("as_get_mapping: unknown errno");
    }
    panic("Invalid code path.");
    return false;
}

void handle_tlb_refill(context_t* context) {
    thread_t* thread = thread_get_current();
    dprintk("\n\tThread %s"
            "\tASID: %u\n"
            "\tentryhi: %p\n"
            "\tbadva: %p\n",
            thread->name, thread->as->asid,
            context->entryhi, context->badva);

    // Addresses for TLB numbering corrensponds to 2 PFNs.
    // virt1 corresponds to VPN2 with virt2 following.
    uintptr_t vpn2;
    uintptr_t pfn1, pfn2;
    bool valid1, valid2;

    vpn2 = ((context->badva & PAGE_MASK) >> 12) >> 1; 
    dprintk("Address : VPN : VPN2 - %p : %p : %p\n",
            context->badva, (context->badva & PAGE_MASK) >> 12, vpn2);
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

    dprintk("Complete!\n");
}
