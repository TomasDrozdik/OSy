# Assignment 06: Userspace Applications

The goal of this assignment is to allow execution of userspace applications
on top of your kernel. You will need to add support for threads that execute
in user mode and build a rudimentary standard library (libc) for the applications to use.


## Overall Notes

This assignment builds on top of the previous one as userspace applications
on your CPU can only access memory mapped via TLB. The interface for creating
userspace processes is again rather simplified but shall be sufficient to verify
that your kernel can support userspace applications.

To create a userspace process, you shall implement a `process_create` function
where you specify the `process_t` structure handle and the address of the application
image and its size. The binary application image will be loaded by MSIM to
a given (fixed) physical address. You are expected to copy the image from
that location to the virtual memory of the new process.

Note that we use a trivial memory layout, where the first virtual page (addresses 0 to 4095)
is not mapped (to enforce that dereferencing NULL pointer fails), the next three pages are
reserved for the userspace stack (we will support only single-threaded applications),
followed by the application code (and space for global variables etc.).
Any memory beyond this can be used for the heap of the application.

For the user application to do something useful, you need to create a base library
with system call support. For this assignment, this `libc` will need to provide
functions for printing to console, heap management and application termination.

Note that when a userspace application tries to do something it is not supposed
to do (i.e. access kernel memory), it should be killed. On the other hand, providing
wrong arguments to a syscall etc. should not result in termination, an error code should
be returned instead.


## Basic Functions

A new userspace process is created with `process_create`, waiting for the process
to finish is done via `process_join`. We expect that you will call `thread_create`
inside `process_create` to actually create the main process thread. Feel
free to use the currently unused `flags` to pass information that the
thread will run in userspace.

There are principially two ways to switch to userspace. One is to
directly jump to userspace when creating the new thread (i.e. set `$ra` to
userspace address and set `$status` accordingly), the other is to start
a kernel thread that will later switch to userspace via call to
`cpu_jump_to_userspace`.

On the userspace side, it is necessary to add an implementation of `printf`
and the related functions. We expect that `printf` will be a copy (without
the `%p` extensions) of your kernel `printk`.

Heap shall be managed via `malloc` and `free`. Note that the linker script
provides `_app_end` symbol similar to `_kernel_end` and you are expected to
add a syscall to determine the virtual memory size. With this change, it will
be possible to port your kernel allocator to userspace. Note that heap
implementation is not required for passing the basic tests.

`np_proc_info_get` is a function that is expected to retrieve information
about the currently running process. The virtual memory size is needed for heap
allocator, the remaning items are there for testing purposes only. We
expect that you will use a single system call to fill this structure.
This function has to be implemented even if you do not implement heap
management.


## Provided Code

There is again one bigger change in kernel initialization. When no kernel
test is launched, we start a userspace application.

We highly recommend to start with your kernel configured without any test
(run `configure.py` without any further settings, except perhaps `--debug`)
and place a breakpoint (`__asm__ volatile(".insn\n\n.word 0x29\n\n");`)
inside the userspace `__main`. With this change, after your kernel
successfully calls the userspace code, MSIM will enter an interactive
mode.

We have provided an almost empty `handle_syscall`. It is not called from
anywhere, but you may find it useful when you work on system calls.


## Tests

All tests covering the userspace functionality are inside `userspace/tests`.
Note that many of them intentionally try whether the application is forcefully
killed.

The `printf` related tests may appear to pass with empty `printf`
implementation, so do not forget about `printf`.


## Hints and Questions to Think About

 * Where will a syscall be intercepted?
 * What syscalls would be needed for printing to console? Should `printf`
   be a syscall? Or should everything be routed through some kind
   of `putchar` syscall?
 * Who will be responsible for populating the virtual memory with the application image?
 * Will the interrupts be disabled during syscall handling?


## Assignment Grading

A fully working assignment will receive a baseline of 10 points.
This refers to the state of the assignment at the submission deadline.

A working implementation where the userspace heap is not implemented
will receive a 2 point penalty.

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
