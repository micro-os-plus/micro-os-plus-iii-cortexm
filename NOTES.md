# Developer notes

## Limitations

The µOS++ port running on synthetic POSIX platforms is fully functional, except
the scheduler runs in cooperative mode only; thread pre-emption might be 
possible, but was considered not worth the effort.

## Context switching

Context switching is implemented with `makecontext()`, `getcontext()`, 
`setcontext()`, available on GNU/Linux and macOS.

## Scheduler tick

The scheduler tick is implemented with `setitimer()`, which triggers a 
periodic `ITIMER_REAL` signal.

## Startup

For portability reasons, execution starts in the `main()` function.

Static constructors are executed before `main()`, as usual, but the order
is not known, so the code must not depend on any specific order.

In particular, registry classes must use the BSS memory initialisation
to clear their data structures, and be sure the registry constructor
does not clear them again, since at that moment it is possible that 
previous other classes to be registered.

## Trace

The default trace output is STDOUT (`OS_USE_TRACE_POSIX_STDOUT`), but 
can be redirected to STDERR by using `OS_USE_TRACE_POSIX_STDERR`.

The trace output is available from the very moment, in the first static 
constructor, since it does not need any initialisation.

## Memory allocation

The default memory allocator uses the system `malloc()`.



