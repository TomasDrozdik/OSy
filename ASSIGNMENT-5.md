# Assignment 05: TLB and Virtual Memory

The goal of this assignment is to extend your kernel with basic support for
virtual memory, including the management of the address translation hardware.
You will also need to implement a frame allocator.

## Overall Notes

The virtual memory support will rely on the concept of virtual address spaces.
In general purpose operating systems, an address space describes the mapping
of multiple virtual memory areas to corresponding physical pages. Your
implementation will be more basic, only mapping a single continuous
virtual memory area (thus somewhat resembling the `sbrk` UNIX system call).
The area will always start at virtual address 0, and extend for a given number
of 4KiB pages (the page starting at virtual address 0 should not be mapped,
so that accessing a NULL pointer will cause an exception).

Your threads will be associated with address spaces. At thread creation time,
your threads can either choose to share the address space of their parent thread
or create a new address space of given size. That size will remain constant
for the lifetime of the address space. The address space should be destroyed
once the thread that has created it terminates.

You can also assume that all physical memory will fit into the 512MiB segment
directly accessible from the kernel. Neither your frame allocator nor your
memory probing code needs to consider physical memory beyond this.

## Basic Functions

One essential feature of the memory management implementation is allocating
physical frames used to back the virtual pages. In your kernel, the allocator
interface is described in `mm/frame.h`. The functions are expected to work in
a manner similar to `kmalloc()` and `kfree()`, but note that frames can
be returned in different chunks than they were allocated.

The frame allocator will work with the same physical memory as your existing heap allocator.
A general solution would update the heap allocator to use the frame allocator for obtaining
pages backing the kernel heap, however, we will also accept solutions where the
physical memory is split between the frame allocator and the kernel heap
allocator at boot, without dynamic resizing.

The virtual memory functions in `mm/as.h` represent the interface of your address spaces.
We do not prescribe any specific data structures for your translation mechanisms.

The actual address translation on your platform is done via TLB.
When software accesses an address whose translation is not in TLB,
a TLB exception is raised and your kernel must fill the proper entry
(or kill the offending thread for invalid memory access). Please consult
the manual on how TLB refill should be implemented.

For effective translation, your processor supports single-byte address space
identifiers (ASID). Until now, your code used ASID 1 (see the third argument of
`cpu_switch_context`). It is now necessary to mark the TLB entries with the proper ASID.
As a basic solution, we will accept implementations where TLB is flushed on each address
space switch, but ideal solutions should allocate ASID identifiers to address spaces.
You can safely assume that there will be never more than 254 active address
spaces, that is, you do not have to implement ASID recycling.

## Provided Code

The first big change is in the exception handling. We have added the assembly language
portion of the TLB refill exception handler, your implementation should add C code
to the `handle_tlb_refill` function called inside this handler. We have also
added several useful macros to `drivers/cp0.h` to simplify TLB management.
Note that in MSIM, you can use the `cpu0 tlbd` command to dump the current
contents of TLB.

As an example of how to use the macros, the following code adds a mapping from
virtual addresses 0x2000 and 0x3000 to physical frames 0x17000 and 0x1B000
for ASID 0x15.

```c
cp0_write_pagemask_4k();
cp0_write_entrylo0(0x17, true, true, false);
cp0_write_entrylo1(0x1B, true, true, false);
cp0_write_entryhi(0x1, 0x15);
cp0_tlb_write_random();
```

We have also included a bitmap implementation in `adt/bitmap.h`. You may want
to use this in your frame allocator code, but it is not mandated.

Finally, we have added one more function to the thread API, namely `thread_kill`.
The function should forcefully terminate a thread, and can be used for example
when the thread accesses memory outside its virtual address space. The
`thread_join` function should return `EKILLED` in that case.

## Tests

There are three groups of new tests -
tests for `thread_kill`,
frame allocator tests and
tests for the virtual memory mapping.

The `thread_kill` tests are extremely simple and are more of a reminder to not forget this function.

The frame allocator tests are similar to the heap allocator tests, do not forget to
run those too to ensure that your frame allocator and heap allocator happilly coexist.

The virtual memory mapping tests check that your threads can access virtual addresses
in the first few kilobytes of virtual memory and that separate address spaces do not
influence each other.

As usual, bear in mind that the tests mostly help you check for forgotten implementation components.
It is possible to pass some tests even with a solution that does not really work, such solutions
will obviously be penalized later.

While passing the tests is required for this assignment, your implementation
will also be checked manually, and must include a working frame allocator and
basic address space management functions to actually pass.

## Hints and Questions to Think About

 * Allocate thread stacks from frame allocator directly.
   You will need much smaller heap then.
 * TLB entries are actually pair of entries.
 * What is PFN and VPN2?
 * How is `thread_kill` different from `thread_finish`?
 * Why `TLBWR` and `TLBWI` instructions do not taky any arguments?
 * What happens when (timer) interrupt occurs in the middle of TLB exception handling?
 * What happens when `handle_tlb_refill` returns?
 * What happens when entry in TLB is marked as invalid?


## Assignment Grading

A fully working assignment will receive a baseline of 10 points.
This refers to the state of the assignment at the submission deadline.

A working implementation where the heap allocator does not request
memory from the frame allocator will receive a 2 point penalty.

A working implementation where each context switch empties the entire
TLB will receive a 2 point penalty. An implementation which does not
suffer from this issue and also supports more address spaces than
254 (valid ASID range) will receive 2 points bonus.

An implementation where the physical memory usage of the heap allocator
can grow and shrink (frames allocated and returned on demand) will
receive 2 points bonus.

The baseline will be further adjusted based on additional criteria:

- Bonus for relevant features that were not part of the assignment
  (such as more complex tests).

- Penalty for bugs that were not discovered by the tests but are still important
  (in extreme cases where the code passess the tests more or less by chance
  the penalty can be severe).

- Penalty for technically poor solution even if it works somehow.

- Penalty for poor coding style (such as inconsistent or cryptic variable names,
  lack of comments, poor code structure, and so on).

Any single bonus or penalty will typically be restricted to 1-2 points,
however, exceptions in extreme cases are possible.
