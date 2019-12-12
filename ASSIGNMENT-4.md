# Assignment 04: Synchronization and Preemptive Scheduler

The goal of this assignment is to extend your kernel with basic synchronization
primitives and make your scheduler preemptive. With this assignment implemented,
you will not need to yield the threads to achieve multitasking.

## Basic Functions

The synchronization primitives are described in two headers: `proc/mutex.h`
and `proc/sem.h`. Both mutexes and semaphores should use passive waiting,
however, you might want to try active waiting as a simpler initial
alternative.

Preemptive scheduling will require changes to your scheduler. We do not
mandate a generic timer device support, it is okay to directly set the timer
device from the scheduler functions because no other part of your kernel will
touch the timer.

## Protecting Critical Sections

To implement blocking mutexes and semaphores, you will need a more low level
primitive to protect access to shared data. Typically, a busy test-and-set loop
would be used, however, because your kernel is expected to run on a single processor
only, you can simplify disable interrupts as needed. To do that, surround your critical
sections with calls to `interrupts_disable()` and `interrupts_restore()`.

Note that you will need to add locking to many of your existing functions,
such as those implementing the heap, after you switch to preemptive scheduling.

## Provided Code

The first big change is in the exception handling. Instead of stopping the simulation,
the code now jumps into `handle_exception_general`, the `context_t` parameter
contains the register values saved at the time of the interrupt. Functions
inside `drivers/cp0.h` should help you read the cause of the interrupt.
Note that TLB exceptions still stop the simulation.

To set the timer interrupt, you will need to update the `compare` register.
We have prepared this code for you in `timer_interrupt_after`.

We have extended the tester script to also check that code that should panic the kernel
actually panics. This is needed for example in the tests for `mutex_destroy` that
should panic the kernel when the mutex is still locked.

## Tests

New tests can be split into two groups: tests for the synchronization primitives
and extensions to the existing tests to check that preemptive scheduling
does not break your implementation.

The synchronization tests check different aspects of the primitives and
should be easy to understand. The extended tests (for heap and threads) check
that the scheduler is really preemptive and that the internal data structures
are not corrupted by concurrent execution.

And as usual: the tests should help you check that you have not forgotten any
major part of the implementation. It is possible to pass some tests even with
a solution that does not really work.

While passing the tests is required for this assignment, your implementation
will also be checked manually, and must include working scheduler and thread
management functions to actually pass.

## Hints and Questions to Think About

 * Interrupt from timer is number 7.
 * What happens when `handle_exception_general` returns?
 * Why do we need `interrupts_disable()` and `interrupts_restore()` instead of `interrupts_enable()`?

## Assignment Grading

A fully working assignment will receive a baseline of 10 points.
This refers to the state of the assignment at the submission deadline.

Working implementation with active waiting will receive a baseline of 6 points.

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
