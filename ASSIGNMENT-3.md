# Assignment 03: Kernel Threads and Cooperative Scheduler

The goal of this assignment is to extend your kernel with basic scheduling mechanisms.
Once you implement this assignment, you will be able to create and manage threads
in your kernel. However, the threads will have to yield the processor voluntarily
in this version (you will be adding exception handling and timer in the next
assignment).

## Basic Functions

The required functions are described in two headers: `proc/thread.h` and `proc/scheduler.h`.
This reflects the logical separation between thread management and thread scheduling.
The functions from `proc/scheduler.h` are not tested directly but must work too.

Note that the `kernel_main` function was changed significantly from the last assignment.
Now it creates a thread for running the system (or the tests).

## Provided Code

Several new functions are available for you: `panic` and `panic_if` for error handling,
`errno_t` for error signalling, and `cpu_switch_context` for performing a thread
context switch.

`panic` and `panic_if` can be used to halt the execution if your code encounters
an unrecoverable error. A typical example is in `kernel_main`, where the kernel
panics if creating the main kernel thread fails. Unlike assertions, kernel
panics are not removed from production builds.

`errno_t` is a new type for passing around error conditions. Feel free to add your
own values, do not forget to extend `errno_as_str`. A new macro for kernel tests,
`ktest_assert_errno`, was added to simplify testing errnos for `EOK`.

`cpu_switch_context` performs actual switching of CPU context. It receives three arguments:
a pointer to a variable where the existing top of stack reference will be stored, a pointer
to a similar variable where the future top of stack reference is stored, and the ASID
(address space identifier) of the new thread. The state of the calling thread is
saved on the stack and the reference to top of that stack is stored in the first
pointer, the state of the returning thread is then loaded from the stack
referenced by the second pointer. The structure of the context is described
by `context_t`.

(The address space identifier is not needed yet, just set it to `1`.)

## Thread Tests

The thread tests at this stage assume cooperative scheduling and are therefore rather simple.
While they should help check that you have not forgotten any major part of the implementation,
it is possible to pass some tests even with a solution that does not really work.

While passing the tests is required for this assignment, your implementation will also be checked
manually, and must include working scheduler and thread management functions to actually pass.

## Hints and Questions to Think About

 * Who calls `thread_finish` if the `thread_entry_func_t` returns?
 * Who will be responsible for thread destruction?
 * Consider adding `T` modifier to `%p` (similar to `L` for lists)
   for printing information about given thread.
 * The first argument is passed in the `a0` register.
 * The CPU status register in kernel mode on initialization should be `0xff01`.

## Assignment Grading

A fully working assignment will receive a baseline of 10 points.
This refers to the state of the assignment at the submission deadline.

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
