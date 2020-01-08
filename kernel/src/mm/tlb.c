// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Charles University

#include <mm/tlb.h>
#include <debug.h>
#include <proc/thread.h>
#include <utils.h>

static inline bool is_even_page(uintptr_t address) {
    assert(address % PAGE_SIZE == 0);
    // Get the 13th bit of address to determine parity.
    return !((address & (1 << 12)) >> 12);
}

static inline uintptr_t upper_address_part(uintptr_t address) {
    uintptr_t r = address & 0xFFFFF000;
    dprintk("Upper part of address %p is %p\n", address, r);
    return r;
}

static inline uintptr_t convert_to_tlb_format(uintptr_t address) {
    uintptr_t r = address >> 12;
    dprintk("TLB format of address %p is %p\n", address, r);
    return r;
}

static bool is_mapped(as_t* as, uintptr_t virt, uintptr_t *phys) {
    errno_t err = as_get_mapping(as, virt, phys);
    switch (err) {
    case EINVAL:
        panic("Tlb refill: requested virtual address not aligned to PAGE_SIZE");
        break;
    case ENOENT:
        return false;
    case EOK:
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
    uintptr_t virt1, virt2;
    uintptr_t phys1, phys2;

    // Valid bits corresponding to PFN1 and PFN2 
    bool valid1, valid2;

    // Round it to page 
    virt1 = upper_address_part(context->badva);

    if (!is_mapped(thread->as, virt1, &phys1)) {
        thread_kill(thread);
    }

    // Now find mapped neighboring pages to map them to VPN2 if possible
    // based on parity of needed address we can only check following page for
    // even address or preceding page for odd one.
    // Non mapped page is invalidated by valid1/2 bool.
    if (is_even_page(virt1)) {
        valid1 = true;
        virt2 = virt1 + PAGE_SIZE;
        valid2 = is_mapped(thread->as, virt2, &phys2);
    } else {
        virt2 = virt1;
        phys2 = phys1;
        valid2 = true;
        virt1 = virt2 - PAGE_SIZE;
        assert(is_even_page(virt1));
        valid1 = is_mapped(thread->as, virt1, &phys1);
    }

    // Add only upper address i.e. PFNs and VPN2
    phys1 = convert_to_tlb_format(phys1);
    phys2 = convert_to_tlb_format(phys2);
    virt1 = convert_to_tlb_format(virt1);

    const bool dirty = true;
    const bool global = false;
    cp0_write_pagemask_4k();
    cp0_write_entrylo0(phys1, dirty, valid1, global);
    cp0_write_entrylo1(phys2, dirty, valid2, global);
    cp0_write_entryhi(virt1 >> 1, thread->as->asid);
    cp0_tlb_write_random();

    dprintk("Complete!\n");
    //while (1); // Trap in msim
}
