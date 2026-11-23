# Code review 1 - µOS++ IIIe Cortex-M port

Scope: the whole port, branch `xpack-development` at 687e975 (identical
to `xpack`, version 1.1.0): `CMakeLists.txt`,
`include/cmsis-plus/rtos/port/os-c-decls.h`, `os-decls.h`,
`os-inlines.h`, `src/rtos/os-core.cpp`, and the user documentation
(`README.md`). `maintenance/` and the reorganisation of the repository
(maintainer files moved under `maintenance/`, `test/` renamed to
`tests/`, uncommitted at the time) were not reviewed.

Reviewed together with the portable core (`micro-os-plus-iii`,
`xpack-development` at fa7327c5, version 7.2.0 in preparation), with
which the port is compiled, and compared with the POSIX port
(`micro-os-plus-iii-posix-arch`, `xpack-development`).

Method: reading the code against the port contract (`os::rtos::port`
in the core `os-decls.h`), the CMSIS core headers and the Arm
documentation, plus four experiments on QEMU Cortex-M7 (debug build of
the core `rtos-apis` test, temporary code, removed afterwards) and a
compiler check with GCC 15 and clang 21. The experiments are quoted in
the findings that use them.

Outcome (2026-11-23): all findings are addressed; fixed in issues #1 to
#12 (commits listed in each section), with tests in the core where the
problem can be reproduced; the Armv8-M support part of #4 is deferred to
the enhancement issue #13.

## Summary

| # | Severity | File | Issue |
| --- | --- | --- | --- |
| #1 | important | `src/rtos/os-core.cpp:564` | A reschedule requested while the scheduler is locked is dropped; a higher priority thread woken by `mutex::unlock()` waits for the next tick (**fixed**) |
| #2 | important | `include/cmsis-plus/rtos/port/os-inlines.h:394` | `cycles_since_tick()` tests the pending tick in the wrong register; `hrclock` goes backwards by up to one tick (**fixed**) |
| #3 | narrow | `src/rtos/os-core.cpp:251` | Stack size assertion mixes words and bytes; debug builds reject stacks below 548 bytes (documented minimum 256) (**fixed**) |
| #4 | decision | `src/rtos/os-core.cpp:33` | No Armv8-M support; legacy architecture lists and an `__ARM_EABI__` guard (guards **fixed**; Armv8-M deferred, issue #13) |
| #5 | narrow | `include/cmsis-plus/rtos/port/os-inlines.h:209` | Cortex-M7 r0p1 erratum 837070: raising BASEPRI does not take effect immediately (**fixed**) |
| #6 | pre-existing | `src/rtos/os-core.cpp:975` | `PendSV_Handler` is `naked` but contains C code and extended asm; unsupported by GCC, rejected by clang (**fixed**) |
| #7 | narrow | `src/rtos/os-core.cpp:463` | Armv6-M reads the initial MSP from address 0 (wrong when the vector table is elsewhere); `__vectors_start` declared and unused (**fixed**) |
| #8 | minor | `src/rtos/os-core.cpp:490` | The fake first thread lives on the stack reused as the interrupt stack; interrupts pending at start can overwrite it (**fixed**) |
| #9 | narrow | `src/rtos/os-core.cpp:345` | `SysTick_Config()` failure ignored in release builds (**fixed**) |
| #10 | nit | `include/cmsis-plus/rtos/port/os-inlines.h:166` | `is_priority_valid()` reads out of `SCB->SHP` in NMI and HardFault; on Armv6-M it returns `true` there (**fixed**) |
| #11 | nit | `include/cmsis-plus/rtos/port/os-inlines.h:420` | Unmatched `#pragma GCC diagnostic pop` (**fixed**) |
| #12 | docs | `README.md`, headers | Outdated or wrong statements (limitations, dependencies, compilers, header comments) (**fixed**) |
| #13 | important | `src/rtos/os-core.cpp:562` | `reschedule()` before the scheduler is started takes PendSV with no scheduler (HardFault) (**fixed**) |

Severity values:

- **important**: a real bug with visible effects in normal use;
- **pre-existing**: a defect or risk that does not show today;
- **narrow**: a real bug, but only in uncommon configurations;
- **minor**: a real defect with a small window or impact;
- **decision**: a design choice or missing feature, needs a decision
  before fixing;
- **nit**, **docs**: style, comments, documentation.

All findings are already present in 1.1.0; none was introduced by a
recent change.

## Suggested fix order

Phase 1, small and contained:

1. #2 - one register name; add a monotonicity test (**fixed**, ff9f21b6, issue #1).
2. #3 - one assertion (debug only) (**fixed**, d5c30e4, issue #4).
3. #11, #10, #12 - pragma, comment and documentation fixes (#11
   **fixed**, a572be7, issue #5; #10 **fixed**, e47269d, issue #6;
   #12 **fixed**, b86a78b).
4. #9 - check the `SysTick_Config()` result (**fixed**, 7c346b2, issue #7).

Phase 2, scheduler behaviour, with tests:

1. #1 - deliver the deferred reschedule at unlock; decide whether in
   the core (all ports) or in each port (**fixed**, bcd0100, issue #2;
   core d7a16bb2, issue micro-os-plus-iii#154; POSIX port 32ed2fa).
2. #8 - enter the first thread directly, no fake thread (**fixed**,
   e6062f2, issue #8).
3. #7 - take the initial MSP from `__stack` (**fixed**, 8905fd7, issue #9).

Found while fixing:

- #13 - `reschedule()` before the scheduler start (**fixed**, 32662b5,
  issue #3).

Phase 3, decide first:

1. #6 - rewrite `PendSV_Handler` as basic asm (needed for clang, and
   for #4) (**fixed**, 3a57a88, issue #10).
2. #4 - Armv8-M support, together with the ACLE tests and the
   portability rule (ACLE guards **fixed**, 3b8a92f, issue #11; Armv8-M
   support deferred to enhancement issue #13).
3. #5 - the Cortex-M7 r0p1 workaround (can be done with #4, since it
   touches the same BASEPRI code) (**fixed**, 29a7ce7, issue #12).

---

## #1 - A reschedule requested while the scheduler is locked is dropped

**File:** `src/rtos/os-core.cpp:564` (`port::scheduler::reschedule()`),
`os-core.cpp:525` (`port::scheduler::locked()`)

**Status:** fixed in bcd0100 (issue #2) and, for the portable part, in
the core d7a16bb2 (micro-os-plus/micro-os-plus-iii#154), and for the POSIX port in
32ed2fa. The ports record the request, the core
performs it at unlock; see "Fix applied" below.

### Problem

`reschedule()` returns without doing anything when the scheduler is
locked:

```cpp
if (rtos::scheduler::locked ()
    || (rtos::interrupts::in_handler_mode () && !rtos::scheduler::preemptive ()))
  {
    return;
  }
```

and `locked (state)`, which ends every `scheduler::critical_section`,
only stores the new state. So a context switch requested inside a
scheduler critical section is not performed when the section ends; it
waits for the next reschedule from somewhere else, usually the next
SysTick.

`os::rtos::scheduler::is_reschedule_pending` exists for this purpose,
but nothing reads it: the port sets it in `reschedule()` (only when
not locked) and clears it in `switch_stacks()`.

### Scenario

The core wakes threads under the scheduler lock in several places, for
example `mutex::unlock()` (`internal_unlock_()` runs under a
`scheduler::critical_section` and calls `list_.resume_one()`), the
priority inheritance paths (`priority_inherited()` ends in `yield()`)
and `mutex::internal_leave_wait_()`. (The idle thread also destroys
threads under the lock, but it yields right after, which delivers the
switch.)

Experiment (QEMU Cortex-M7, debug): the main thread (normal priority)
locks a `protocol::none` mutex; a `high` priority thread blocks on it;
main calls `unlock()` and then busy-waits until the waiter has run.

```
EXPERIMENT mutex unlock: waiter ran at once 0, waited 2 ticks
```

The higher priority thread, ready since `unlock()`, did not run until
the next tick; the lower priority main thread kept the CPU meanwhile.
In a system with a long tick period, or with a busy main thread, this
is a priority inversion of up to one tick on every mutex hand-off.

### Documentation and tests

- The reference describes `scheduler::critical_section` as deferring
  context switches, not dropping them.
- No test checks that a woken higher priority thread runs at once; the
  existing tests block (join, sleep) right after waking, which hides
  it.

### Port behaviour

The POSIX port (`micro-os-plus-iii-posix-arch`, `xpack-development`)
has the same structure: `reschedule()` returns when locked and
`locked()` only stores the state.

### Suggested fix

Remember the request and deliver it at unlock:

- in `reschedule()`, when the scheduler is locked, set
  `is_reschedule_pending = true` and return;
- when the scheduler becomes unlocked and `is_reschedule_pending` is
  set (and not in handler mode), call `reschedule()`.

The second part is best done once in the core, in
`scheduler::locked (state_t)` / `unlock()`, after calling the port, so
both ports benefit; the port only needs the first part. Test: the
experiment above as a regression test (`ran at once` must be 1).

### Analysis before fixing

- Confirmed in both ports: `reschedule()` returns when the scheduler is
  locked, and `locked()` only stores the state.
- Every unlock goes through three core functions: `scheduler::lock()`,
  `unlock()` and `locked (state_t)`, the last one also ending every
  `scheduler::critical_section`; so the catch-up is best done once,
  there.
- Requests from handlers when not preemptive (Cortex-M in cooperative
  mode, POSIX always) are left as they are, by design; only the
  "locked" case is recorded.

### Fix applied

- Core (`micro-os-plus-iii`, `src/rtos/os-core.cpp`): a private
  `internal_reschedule_if_pending_()`, called at the end of `unlock()`
  and `locked (state_t)`, calls `port::scheduler::reschedule()` when the
  scheduler is started, unlocked and `is_reschedule_pending` is set
  (the flag is read after the new state is stored, so a request made
  just before is seen, one made just after is performed at once by the
  port). `start()` clears the flag before the port start. Documented
  in `os-sched.h`; `CHANGELOG.md` entry.
- Cortex-M port (`src/rtos/os-core.cpp`, `reschedule()`): when the
  scheduler is locked, set `is_reschedule_pending` and return; the
  cooperative handler case is unchanged.
- POSIX port (`src/rtos/os-core.cpp`, `reschedule()`): the same for
  the locked case in thread mode; the flag is cleared when it switches.
- No C API or data layout changes.

A first version did not check `is_started_`: natively, a request
recorded while the scheduler was locked before the start was then
performed at the next unlock, still before the start, and the POSIX
port aborted in `scheduler::start()` (`Assertion failed: (!empty ())`,
`unlink_head`). Hence the `is_started_` test and the reset in
`start()`.

### Verification

New tests in the core `tests/sources/rtos-apis/src/test-cpp-api.cpp`,
"Scheduler lock" section, deterministic by priorities:

1. a `high` waiter on a `protocol::none` mutex owned by main; after
   `mx.unlock ()` returns, the waiter must already have run;
2. a `high` thread waiting on a semaphore; main posts it inside a
   `scheduler::critical_section`; right after the section the thread
   must have run.

Negative check (core and both ports at their previous commits, failures
made non-fatal): on QEMU Cortex-M7 and natively both cases report "did
not run at once". With the fix, `test-native-cmake-clang`,
`test-native-cmake-gcc`, and the QEMU Cortex-M0, M3, M4F and M7F suites
pass (debug and release), including `mutex-stress` and the CMSIS
validator.

---

## #2 - `cycles_since_tick()` tests the pending tick in the wrong register

**File:** `include/cmsis-plus/rtos/port/os-inlines.h:394`

**Status:** fixed in ff9f21b6 (issue #1; test in the core d3001418): the
pending tick is read from `SCB->ICSR`; see "Fix applied" below.

### Problem

```cpp
if (SysTick->CTRL & SCB_ICSR_PENDSTSET_Msk)
```

`SCB_ICSR_PENDSTSET_Msk` is bit 26 of `SCB->ICSR` (SysTick exception
pending). Bit 26 of `SysTick->CTRL` is reserved and reads 0 (CMSIS
`core_cm7.h`: `SCB_ICSR_PENDSTSET_Pos 26U`, `SysTick_CTRL_COUNTFLAG_Pos
16U`). So the correction for a tick that wrapped while its interrupt is
held pending never applies: `steady_count_` has not been incremented
yet, `VAL` has been reloaded, and `clock_highres::now()` returns a value
up to one tick smaller than a previous reading.

### Scenario

Experiment (QEMU Cortex-M7, debug, 16000 cycles per tick): reading
`hrclock.now()` repeatedly inside an `interrupts::critical_section`
(5 rounds of 400000 reads):

```
EXPERIMENT hrclock backwards 4040 times, max 15599 cycles (cycles per tick 16000)
```

Effects:

- any application measuring intervals with `hrclock.now()` can get a
  negative interval (a huge unsigned one);
- with `OS_INCLUDE_RTOS_STATISTICS_THREAD_CPU_CYCLES`,
  `internal_switch_threads()` computes
  `delta = now - switch_timestamp_` (unsigned), so a backward step adds
  almost 2^64 cycles to a thread's statistics.

### Documentation and tests

`hrclock` is documented as a monotonic high resolution clock. No test
checks monotonicity.

### Suggested fix

Test `SCB->ICSR & SCB_ICSR_PENDSTSET_Msk`. Keep the existing double
read of `VAL` (it decides whether the wrap happened before or after the
first read). Add a test like the experiment, with no backward step
allowed.

### Analysis before fixing

- The register is the bug. Once it is fixed, a second, smaller flaw
  shows: the double read of `VAL` took the "wrap before the first read"
  branch only for `val > val_subsequent`; two reads returning the same
  value (the counter did not advance, possible on QEMU, whose SysTick
  follows virtual time) also mean the wrap happened before the first
  read, so the test must be `>=`.
- The hardware records only one wrap (one pending bit), so a critical
  section held for more than one tick still loses ticks; this cannot be
  fixed and is now documented in a comment.
- The core uses `cycles_since_tick()` only in `clock_highres::now()`,
  under an interrupts critical section; the fix is local to the port.

### Fix applied

- `include/cmsis-plus/rtos/port/os-inlines.h`,
  `clock_highres::cycles_since_tick()`: `SCB->ICSR &
  SCB_ICSR_PENDSTSET_Msk` instead of `SysTick->CTRL & ...`; the
  "no reload between the two reads" test is `val >= val_subsequent`;
  comment on the one-wrap limit.
- No core, C API or data layout changes.

### Verification

New test in the core `tests/sources/rtos-apis/src/test-cpp-api.cpp`,
"Clocks" section: 20 rounds, each reading `hrclock.now()` inside an
`interrupts::critical_section` for 0.75 tick (so at most one wrap can
happen while the tick is masked, also when a tick was already pending
at the start because of the interrupt latency); no reading may be
smaller than the previous one; it reports how many rounds actually
crossed a tick.

- A first version kept the critical section for 1.5 ticks and failed
  also with the fix: the diagnostic showed rounds starting with a tick
  already pending, then crossing a second wrap, which no implementation
  can recover. Hence the sub-tick window.
- Negative check (QEMU Cortex-M7, debug): with the old header the test
  reports `hrclock went backwards 18 times`; with the fix, three runs
  passed (19-20 rounds over a tick each).
- The `>=` change alone was not demonstrated: with only the register
  fixed (`>` kept) the test also passed three runs; equal reads do occur
  on QEMU (`VAL 1/1` in the diagnostic), so the change is kept on
  analysis.
- `test-qemu-cortex-m7f-cmake`, `-m0-`, `-m3-`, `-m4f-` and
  `test-native-cmake-clang` pass (debug and release), the core tests
  using this repository through the `xpm link`. On native the POSIX
  port's `hrclock` also passes the test.
- `test-nucleo-f411re-cmake` passes on the board (debug and release,
  15 rounds over a tick).

---

## #3 - Stack size assertion mixes words and bytes

**File:** `src/rtos/os-core.cpp:251` (`context::create()`)

**Status:** fixed in d5c30e4 (issue #4; test in the core 6f8b7c26): the
assertion compares bytes with bytes; see "Fix applied" below.

### Problem

```cpp
assert ((p - stack.bottom ())
        > static_cast<int> (2 * sizeof (stack::frame_t)));
```

`p - stack.bottom ()` is a number of 32-bit words; `2 * sizeof
(frame_t)` is a number of bytes (2 * 68 = 136). The check intended
"room for two exception frames" (34 words) but requires more than 136
words (544 bytes), so in debug builds every thread with a stack of 548
bytes or less fails, although `OS_INTEGER_RTOS_MIN_STACK_SIZE_BYTES`
(and `thread::stack::min_size ()`) is 256.

Experiment (QEMU Cortex-M7, debug): a thread with
`th_stack_size_bytes = 512`:

```
EXPERIMENT min stack 256
assertion "(p - stack.bottom ()) > static_cast<int> (2 * sizeof (stack::frame_t))" failed
```

Also in `context::create()`: the `& 3` alignment block is dead code
(`p` is an `element_t*`, always 4-byte aligned) and casts a pointer to
`int`.

### Suggested fix

Compare like with like, for example
`(p - stack.bottom ()) * sizeof (element_t) > 2 * sizeof (frame_t)`,
and drop the dead `& 3` block. Test: a thread with the minimum stack
size, in debug.

### Analysis before fixing

- Confirmed: `p - stack.bottom ()` counts words, `2 * sizeof (frame_t)`
  is 136 bytes, so the intended 34 words became more than 136 words.
- The core already guarantees the minimum (`thread::stack::initialize()`
  aligns the bottom to 8 bytes and fails with `ENOMEM` below
  `min_size ()`; a requested size not above `min_size ()` gets the
  default size), so the port assertion is only a sanity check and
  correcting it cannot let a too small stack through.
- Release builds are not affected (no assertions), except for the dead
  `& 3` block.

### Fix applied

- `src/rtos/os-core.cpp`, `context::create()`: the assertion compares
  `(p - stack.bottom ()) * sizeof (element_t)` with
  `2 * sizeof (frame_t)` (bytes); the dead `& 3` block, with its
  pointer to `int` cast, is removed, with a comment explaining why the
  frame starts at 4 modulo 8; the final alignment assertion uses
  `uintptr_t` instead of `int`.
- No core, C API or data layout changes; the POSIX port has no such
  code.

### Verification

New test in the core `tests/sources/rtos-apis/src/test-cpp-api.cpp`
("Threads"): a thread with a stack of `thread::stack::min_size () * 2`
bytes (512 on Cortex-M, 65536 natively) runs and is joined.

Negative check (QEMU Cortex-M7, debug): with the old port the assertion
fails. With the fix, the QEMU Cortex-M0, M3, M4F and M7F suites and
`test-native-cmake-clang` pass (debug and release).

---

## #4 - No Armv8-M support; legacy architecture lists; `__ARM_EABI__` guard

**File:** `src/rtos/os-core.cpp:17` and `:33`;
`include/cmsis-plus/rtos/port/os-inlines.h` (all architecture tests)

**Status:** guards fixed in 3b8a92f (issue #11; core `CLAUDE.md` in
a130e1fb): ACLE tests and one central architecture check; Armv8-M support
deferred (enhancement issue #13).

### Problem

- `os-core.cpp` is compiled only under `__ARM_EABI__` (a toolchain
  macro; see the portability rule in the core `CLAUDE.md`) and stops
  with `#error "Architecture not supported."` unless the core is
  v6-M, v7-M or v7E-M. Armv8-M Baseline (M23) and Mainline (M33, M35P,
  M52, M55, M85) are not supported, although the core already handles
  v8-M in `exception-handlers.c`.
- `os-inlines.h` tests the same legacy lists; on any other
  architecture `is_priority_valid()` has no return statement and the
  critical sections return an uninitialised `pri` (hidden today only by
  the `#error` in `os-core.cpp`).
- clang names Armv8.1-M `__ARM_ARCH_8_1M_MAIN__`, which no list knows.

### Suggested fix

Replace the lists with the ACLE tests the core now uses
(`__ARM_ARCH_PROFILE == 'M'`; `__ARM_ARCH_ISA_THUMB == 2` for Mainline,
`== 1` for Baseline, with a comment listing the matched architectures),
and decide on Armv8-M:

- Baseline (M23) can follow the v6-M code (but it has VTOR, see #7);
- Mainline can follow the v7-M code (BASEPRI, FPU), but needs a
  decision on the Security Extension: the fixed initial `EXC_RETURN`
  `0xFFFFFFFD` and the stack limit registers (`PSPLIM`, `MSPLIM`) are
  correct only for builds without TrustZone or with the limits unused.

Until then, keep the `#error` but on the ACLE test, so the message is
right for every unsupported core. Verification is limited: QEMU
provides Cortex-M33 (`mps2-an505`), but the tests have no such
platform yet.

### Analysis before fixing

- the legacy lists were in `os-core.cpp` (the file guard, the
  `#error`, the `__MAX_PRIO` static asserts, `start()`, the critical
  section in `switch_stacks()`) and in `os-inlines.h` (the greeting,
  `is_priority_valid()`, the four critical/uncritical section
  functions, which have no `#else`; the `#error` in `os-core.cpp` did
  not protect them, since `os-inlines.h` is compiled in every file that
  includes `os.h`);
- two earlier fixes in this review (#8 in `start()`, #10 in
  `is_priority_valid()`) had kept the legacy lists;
- the ACLE macros (arm-none-eabi-gcc 14): `__ARM_ARCH` is 6 (M0/M0+),
  7 (M3/M4/M7) or 8 (M23/M33/M55); `__ARM_ARCH_ISA_THUMB` 1 (Baseline)
  or 2 (Mainline); `__ARM_FEATURE_DSP` tells v7E-M from v7-M;
- Armv8-M support cannot be tested here yet: the tests use CMSIS 4.5.0
  (no `core_cm23.h`/`core_cm33.h`/`core_cm55.h`); QEMU provides
  `mps2-an505` (M33) and `mps3-an547` (M55); it also needs the
  TrustZone decisions (initial `EXC_RETURN`, `PSPLIM`/`MSPLIM`). So it
  is a separate enhancement, not part of this fix.

### Fix applied

- one central check in `include/cmsis-plus/rtos/port/os-decls.h` (the
  first port header the core includes): `#error` unless M-profile
  (written as the core's profile test), and `#error` for Armv8-M
  (`__ARM_ARCH` not 6 or 7) until it is supported;
- `os-core.cpp`: the `__ARM_EABI__` file guard and the old `#error`
  removed (a misconfigured build now fails with a clear message instead
  of an empty object file);
- all other tests use `__ARM_ARCH_ISA_THUMB`, with a comment listing the
  matched architectures; architecture-specific code uses `== 2` /
  `#elif == 1` / `#else #error`; the critical sections use
  `Mainline && BASEPRI` / `#else` PRIMASK (valid on every M core), which
  also merges the duplicated PRIMASK branches;
- the greeting keeps exactly the same text (`__ARM_FEATURE_DSP` for
  v7E-M);
- core `CLAUDE.md`: the portability rule mentions `__ARM_ARCH`, for one
  central check excluding unsupported versions.

No behaviour change; no change to the core code, the C API, the data
layout or the POSIX port.

### Verification

- release binaries (QEMU M0, M3, M4F, M7F and native) hashed with
  `hash-builds.sh` before and after: all identical, including the CMSIS
  validator images (0 differing bytes);
- the port `os-core.cpp` compiled with `-mcpu=cortex-m33` and
  `-mcpu=cortex-m23` stops with "Armv8-M is not yet implemented in the
  Cortex-M port", with `-mcpu=cortex-a7` with "The Cortex-M port
  requires an M-profile architecture";
- QEMU M0, M3, M4F and M7F suites (debug and release) pass, with no
  warnings.

---

## #5 - Cortex-M7 r0p1 erratum 837070

**File:** `include/cmsis-plus/rtos/port/os-inlines.h:209`
(`critical_section::enter()`), `src/rtos/os-core.cpp:793`
(`switch_stacks()`)

**Status:** fixed in 29a7ce7 (issue #12; documentation in the core
9a7c6e2e): BASEPRI is raised with interrupts
disabled (PRIMASK saved and restored) on Cortex-M7 builds, unless
`OS_DISABLE_CORTEXM7_ERRATUM_837070_WORKAROUND` is defined.

### Problem

On Cortex-M7 revision r0p1 (used in many STM32F7 devices), Arm erratum
837070 says that increasing the priority level with a write to
`BASEPRI` may not take effect immediately, so an interrupt that should
be masked can still be taken right after the write. With
`OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY` defined, the port
raises BASEPRI with `__set_BASEPRI_MAX()` in `critical_section::enter()`
and `switch_stacks()`, so on these parts a kernel-aware interrupt can
enter a critical section.

### Suggested fix

The workaround recommended by Arm (and used by the FreeRTOS
`ARM_CM7/r0p1` port) is to disable interrupts around the BASEPRI raise:
`cpsid i; msr basepri, rX; cpsie i`. Enable it for Cortex-M7 builds
(`__CORTEX_M == 7`), or behind an option, since the cost is small. Not
testable on QEMU, which does not model the erratum.

### Analysis before fixing

- the erratum affects only writes that raise BASEPRI; besides
  `critical_section::enter()` and `switch_stacks()`, the review missed
  `uncritical_section::exit()`, which raises BASEPRI back to the
  critical level; the other writes lower it;
- the suggested `cpsid i; msr basepri; cpsie i` is unsafe here: critical
  sections can run with PRIMASK already set (for example on the start
  path, or in application code that disables interrupts around RTOS
  calls), and `cpsie i` would enable the interrupts; PRIMASK must be
  saved and restored;
- r0p1 parts are early STM32F74x/F75x; STM32F76x/F77x (the Nucleo-F767ZI
  in the tests) are r1p0, STM32H7 r1p1; the revision cannot be known at
  compile time, and a run-time `SCB->CPUID` test costs about as much as
  the workaround.

### Fix applied

- `os-inlines.h`: `interrupts::internal::raise_basepri_max()` and
  `raise_basepri()`; with BASEPRI critical sections on Cortex-M7
  (`__CORTEX_M == 7`, the CMSIS core macro), they save PRIMASK, disable
  interrupts, write BASEPRI (or BASEPRI_MAX) and restore PRIMASK;
  otherwise a plain write;
- used in `critical_section::enter()`, `uncritical_section::exit()` and
  `switch_stacks()`; the writes that lower BASEPRI are unchanged;
- enabled by default on all Cortex-M7 builds (about 3 instructions per
  critical section entry); `OS_DISABLE_CORTEXM7_ERRATUM_837070_WORKAROUND`
  disables it for r1p0 and later parts; documented in the core
  `doxygen/pages/os-app-config.h`.

No change to PRIMASK-only (DI/EI) builds, other cores, the C API, the
data layout or the POSIX port.

### Verification

- no deterministic test: QEMU does not model the erratum, and no test
  board is r0p1;
- release binaries hashed before and after (`hash-builds.sh`): M0, M3,
  M4F and native identical; on M7F only `rtos-apis` and `mutex-stress`
  changed (they use BASEPRI), the CMSIS validator (DI/EI) is identical;
- M7F release disassembly: `switch_stacks()` and the inlined critical
  sections show `mrs PRIMASK; cpsid i; msr BASEPRI_MAX; msr PRIMASK`;
  with `-DOS_DISABLE_CORTEXM7_ERRATUM_837070_WORKAROUND` the `cpsid`
  is gone;
- QEMU M0, M3, M4F and M7F suites (debug and release) pass, with no
  warnings.

---

## #6 - `PendSV_Handler` is `naked` but contains C code and extended asm

**File:** `src/rtos/os-core.cpp:975`

**Status:** fixed in 3a57a88 (issue #10): `PendSV_Handler` contains only
basic asm; the generated instructions are unchanged.

### Problem

`PendSV_Handler` is `naked` and calls the inline functions
`save_on_stack()`, `switch_stacks()` and `restore_from_stack()`, which
use extended asm and a C call. The GCC documentation says only basic
asm can be used safely in naked functions; the rest "may appear to work"
but is not supported. The code relies on `optimize ("1")` and on the
compiler keeping `sp` in `r0` (the base register of `ldmia` must not be
in the register list).

Compiler check with the same pattern:

- GCC 15 (`-O0` with the `optimize ("1")` attribute) generates the
  expected sequence (`mrs r0, PSP; stmdb r0!, {...}; bl switch_stacks;
  ldmia.w r0!, {...}; msr PSP, r0; bx lr`);
- clang 21: `error: non-ASM statement in naked function is not
  supported` (and `optimize` is ignored).

So the port cannot be built with clang, although the README says the
code was compiled with clang.

### Suggested fix

Write the handler as one basic asm block per architecture (save, `bl`
to an `extern "C"` wrapper of `switch_stacks()`, restore, `bx lr`), as
most RTOS ports do. Check the generated code against the current one
(it should be the same instructions).

### Analysis before fixing

- besides the naked rule, the extended asm depended on the register
  allocation: on Armv6-M the save sequence used `r3`-`r7` as scratch
  with no clobbers declared, and the `ldmia` base had to be `r0` (not
  `r4`, which is in the register list); both held only by chance, with
  the help of `optimize ("1")` (not meant for production, ignored by
  clang);
- the code GCC generates today is correct (checked on M0, M3, M4F and
  M7F, debug and release), so the rewrite can be compared instruction
  by instruction;
- basic asm can only call a fixed symbol name, while `switch_stacks()`
  is declared in the portable part with C++ linkage (and is a friend
  of `thread`); a C linkage wrapper would cost a tail branch per
  context switch (on Armv6-M a `push`/`bl`/`pop`, measured).

### Fix applied

- `PendSV_Handler()` is `naked` and contains a single basic asm block,
  one sequence for Mainline (`__ARM_ARCH_ISA_THUMB == 2`, with the FPU
  part under `__ARM_FP`) and one for Baseline (`== 1`), `#error`
  otherwise; the same instructions as before, written explicitly;
- the block starts with `.syntax unified` (GCC assembles Thumb-1 inline
  asm in the divided syntax, which rejects `subs r0, #36`);
- `switch_stacks()` gets the assembler name `os_rtos_port_switch_stacks`
  by a redeclaration in the port, before its definition (accepted by
  GCC and clang; `-Wredundant-decls` silenced locally, for GCC), so the
  handler calls it directly, with no wrapper;
- removed: `save_on_stack()`, `restore_from_stack()`, the
  `optimize ("1")` attribute and the outdated disassembly comment;
- README: clang is no longer said to be rejected, but not tested.

No behaviour change; no change to the core, the C API, the data layout
or the POSIX port.

### Verification

- the `PendSV_Handler` disassembly of the `rtos-apis` test before and
  after, on QEMU M0, M3, M4F and M7F, debug and release: identical
  instructions (only the `bl` target symbol name differs; in debug
  builds a padding `nop` follows `bx lr`, not executed);
- clang (Apple clang 17, the xPack clang builds have no Arm target):
  the port `os-core.cpp` compiles for `thumbv6m-none-eabi` with no
  errors or warnings (previously `non-ASM statement in naked
  function`); for `thumbv7em-none-eabihf` the only error is in the
  CMSIS 4 `cmsis_gcc.h` used by the tests (`vfpcc` clobber), not in the
  port; the handler alone assembles for Armv6-M, Armv7-M, Armv7E-M with
  FPU and Armv8-M Main into the same instructions as GCC; not linked or
  run with clang;
- QEMU M0, M3, M4F and M7F suites (debug and release) pass, with no
  warnings.

---

## #7 - Armv6-M reads the initial MSP from address 0

**File:** `src/rtos/os-core.cpp:463` (`scheduler::start()`), `:67`

**Status:** fixed in 8905fd7 (issue #9; test in the core 7569c54c): MSP is
reset to `__stack` on all architectures.

### Problem

On Armv6-M `start()` resets MSP from `*(uint32_t*) 0x00000000`, which
needs `-fno-delete-null-pointer-checks` (undefined behaviour otherwise)
and is wrong when the vector table is not at address 0: Cortex-M0+ has
an optional VTOR, and with a bootloader the word at 0 is the
bootloader's stack. `__vectors_start` (the application vector table,
from the linker script) is declared for v6-M but never used.

### Suggested fix

Use `__vectors_start` (its first word is the initial stack pointer)
for Baseline cores, and drop the attribute. The core's
`initialize-hardware.c` already relies on `__vectors_start` for VTOR.

### Analysis before fixing

- the port already describes the interrupt stack as
  `[_Heap_Limit, __stack)` (for `interrupts::stack()` and its usage
  statistics), and on all test platforms the first word of the vector
  table is `&__stack`; so the initial MSP is simply `&__stack`, and
  reading it back from a vector table (VTOR on Mainline, address 0 on
  Baseline) is an indirection that may disagree with that range (boot
  ROM at 0 on the RP2040, a bootloader table, or a VTOR controlled by
  other firmware, the reason for `OS_DISABLE_CORTEXM_SET_MSP_VIA_VTOR`);
- `__vectors_start` would add a link-time dependency in the port on
  Mainline too, while the core references it only from a weak
  function that applications may override;
- the Armv6-M read from address 0 is a null pointer dereference
  (undefined behaviour), made to work only with a GCC-only
  `optimize ("no-delete-null-pointer-checks")` attribute (see the
  note in #8).

### Fix applied

- `scheduler::start()` sets the initial MSP to `&__stack` on all
  architectures (no memory read, no undefined behaviour), or keeps the
  current MSP when `OS_DISABLE_CORTEXM_SET_MSP_VIA_VTOR` is defined (the
  name kept for compatibility);
- removed: the VTOR read, the read from address 0, the `optimize`
  attribute, the unused `__vectors_start` declaration and the
  `__ARM_ARCH_7M__`/`__ARM_ARCH_6M__` list with its `#error` (this part
  now works unchanged on Armv8-M, a step towards #4);
- core: the documentation of `OS_DISABLE_CORTEXM_SET_MSP_VIA_VTOR`
  (`doxygen/pages/os-app-config.h`) updated.

Behaviour change only where the vector table is not the one at address
0 on Armv6-M (for example the RP2040): the interrupt stack now starts
at the application stack top. No change to the C API, the data layout
or the POSIX port.

### Verification

- new test in the core "Interrupt priorities" (Cortex-M only, unless
  `OS_DISABLE_CORTEXM_SET_MSP_VIA_VTOR`): in Thread mode, `__get_MSP()`
  (the base of the interrupt stack) equals `&__stack`;
- negative check, with the previous port `os-core.cpp`: QEMU M0 and
  M7F debug pass too (on QEMU the vector table is at address 0 and VTOR
  points to it), so on QEMU the test is a regression test only; the
  previous code would fail it on the RP2040 (not run on hardware: the
  Pico test build currently fails in the test platform, which calls
  `clocks_init()` without the Pico SDK 2.x header, with the build
  linked to a local SDK 2.3.0 instead of the pinned 1.5.1; skipped for
  now);
- QEMU M0, M3, M4F and M7F suites (debug and release) pass, with no
  warnings (native tests not needed).

---

## #8 - The fake first thread lives on the stack reused as the interrupt stack

**File:** `src/rtos/os-core.cpp:490` (`scheduler::start()`)

**Status:** fixed in e6062f2 (issue #8): `scheduler::start()` selects the
first thread and enters it directly, as the POSIX port does; there is
no fake thread and no first context switch.

### Problem

`start()` switches the thread stack to PSP (at the current MSP value),
resets MSP to the top of the stack, then builds `os_thread_t
fake_thread` as a local variable and enables interrupts. Interrupt
handlers now run on MSP, from the top down over the frames of the
callers of `start()`; the comment notes the overlap. Any interrupt
pending at `__enable_irq()` runs before PendSV (which has the lowest
priority), and if its stack use goes deeper than those caller frames it
overwrites `fake_thread`. `switch_stacks()` then writes into it and
`internal_relink_running_()` reads its `state_`; a garbage `running`
state would link a fake thread into the ready list.

### Suggested fix

Give `fake_thread` static storage (it is used once; cost: one
`os_thread_t` in `.bss`). Not reproducible deterministically; a review
fix.

### Analysis before fixing

The review understated it:

- measured on QEMU (temporary instrumentation), the fake thread ends
  only 112-144 bytes below the top of MSP (the callers of `start()` use
  little stack): M0 debug `[0x207FFEC0,0x207FFF48)`, M7F release
  `[0x207FFF08,0x207FFF90)`, MSP top `0x20800000`;
- not only other interrupts: the first PendSV itself runs
  `switch_stacks()` and the core `internal_switch_threads()` on MSP,
  from the top, while writing `stack_ptr` into the fake thread and
  reading its `state_`; MSP in the first `switch_stacks()` was
  `0x207FFFD0` (debug) / `0x207FFFF0` (release), so a margin of only
  96-136 bytes for the rest of that call chain (with
  `OS_TRACE_RTOS_THREAD_CONTEXT`, a `trace::printf()` runs there);
- a run with `OS_TRACE_RTOS_THREAD_CONTEXT` did not fail on M7F (on M0
  the 512 bytes test thread overflowed, an unrelated effect of the extra
  tracing), so not reproduced;
- the comment "this routine does not use stack variables" was wrong;
- the static fake thread costs 136 bytes for the whole run, only to be
  used once; rejected. A null current thread for the first switch
  would add a test to every context switch (port and core); placing the
  fake thread at the bottom of the interrupt stack would break the
  interrupt stack usage statistics. Rejected too.

The POSIX port has no fake thread: it selects the first thread with
`unlink_head()` and loads its context with `setcontext()`.

### Fix applied

- `scheduler::start()` selects the first thread
  (`ready_threads_list_.unlink_head()`, which marks it as running),
  emits the instrumentation `active` event and, with CPU cycles
  statistics, sets the switch timestamp (the time before the start is
  not charged to the first thread); entering the first thread is not
  counted as a context switch (the per-thread counter is private to the
  core; the POSIX port does not count it either);
- it then enters the initial context built by `context::create()`
  directly from Thread mode, with a short inline asm sequence: PSP set
  to the thread stack after the frame, CONTROL (SPSEL set, FPCA
  cleared when present), ISB, MSP reset to the top of the interrupt
  stack, `r0` (argument) and `lr` loaded, `cpsie i`, `bx` to the entry;
  interrupts are enabled only when no stack variable is used any more;
- the initial MSP value is computed first (VTOR, or address 0 on
  Armv6-M, unchanged, see #7) and set in the asm sequence;
- removed: the fake thread, its `memset()`, the first `reschedule()`
  and the wrong comment.

0 bytes of RAM; `PendSV_Handler()` and `switch_stacks()` (the context
switch path) are unchanged. No change to the core, the C API, the data
layout or the POSIX port.

Noticed while fixing: the Armv6-M read of the initial MSP from address
0 is a null pointer dereference (undefined behaviour, made to work with
a GCC-only `optimize` attribute); fixed with #7 (MSP set from
`__stack`).

### Verification

- the start path runs in every test: QEMU M0, M3, M4F and M7F suites
  (debug and release) pass, with no warnings (native tests not needed,
  the change is in the Cortex-M port only);
- the generated code (M0 debug, M7F release) loads all the asm inputs
  into registers before the stack switch.


### Follow-up: redundant switch timestamp

Found while fixing the POSIX port review
(`micro-os-plus-iii-posix-arch/maintenance/docs/CODE-REVIEW-1.md`, #7):
the core's `scheduler::start()` already resets the statistics and sets
`switch_timestamp_ = hrclock.now()` right before calling
`port::scheduler::start()`, so the assignment added to `start()` here
(with the comment "Do not charge the time before the start to the first
thread") was redundant. Removed in c244fa1 (no issue), for these reasons:

- one place: the core owns the statistics initialisation, and the POSIX
  port relies on it, so both ports now do the same bookkeeping;
- the comment was misleading: it suggested the port corrected something
  the core did wrong;
- the only effect of the second assignment was to exclude the few
  microseconds of the port start (and any interrupt taken before the
  port disables them) from the first thread's CPU cycles, negligible
  in a statistic that includes the switch and interrupt time at every
  later switch anyway.

The comment in `start()` now gives this reason. The instrumentation
`active` event is kept, and was added to the POSIX port.

QEMU M0, M3, M4F and M7F suites (debug and release) pass, with no
warnings.

---

## #9 - `SysTick_Config()` failure ignored in release builds

**File:** `src/rtos/os-core.cpp:345` (`clock_systick::start()`)

**Status:** fixed in 7c346b2 (issue #7): the result is checked in all
builds; on failure the port stops, like the default fault handlers.

### Problem

`SysTick_Config()` fails when the reload value exceeds 24 bits
(`SystemCoreClock / OS_INTEGER_SYSTICK_FREQUENCY_HZ > 2^24`, for example
480 MHz with a 10 Hz tick). Debug builds assert; release builds ignore
the result and run without a tick, so the first `sleep_for()` never
returns. The following `NVIC_SetPriority (SysTick_IRQn, ...)` repeats
what `SysTick_Config()` already does.

### Suggested fix

Check the result in all builds (`os_assert_throw()` or a trace message
and `abort()`), and drop the redundant priority setting.

### Analysis before fixing

- the review is right, but the more likely cause is not a too large
  reload value: `SystemCoreClock` equal to 0 (`SystemCoreClockUpdate()`
  not called) or lower than the tick frequency gives a reload value of
  0, and `ticks - 1` wraps;
- the suggested `abort()` (also what `os_assert_throw()` ends in with
  `-fno-exceptions`) is not effective on Cortex-M release builds: it
  calls `_Exit()` and `os_terminate()`, which resets the core; after the
  reset `SysTick_Config()` fails again, so the board loops through
  resets, with no message, since the trace is usually disabled in
  release. (The POSIX port uses `abort()`, which is correct there.)
  The debug `assert()` ends the same way on boards without
  semihosting;
- the extra `NVIC_SetPriority()` is redundant with all CMSIS versions in
  use, but harmless and explicit; kept, with a comment.

### Fix applied

- `clock_systick::start()` checks the `SysTick_Config()` result in all
  builds; on failure it prints a trace message (reload value and
  `SystemCoreClock`), disables interrupts, in debug builds breaks into
  the debugger (on Mainline only when one is connected, as the default
  fault handlers do), and stops in a loop: no reset, the state is kept
  for a debugger;
- the `NVIC_SetPriority()` call is kept, with a comment.

Behaviour change: a release build with an invalid SysTick configuration
now stops at startup instead of hanging on the first timeout. No change
to the POSIX port, the core, the C API or the data layout.

### Verification

- no automated test (the failure stops the system); manual check on
  QEMU M7F, with a temporary `SystemCoreClock = 0` and SysTick stopped
  at the end of `os_startup_initialize_hardware()` (after the platform's
  early tick check), then restored:
  - without the fix, release: `main()` starts and hangs at the first
    wait; debug: the `assert()` fails and aborts;
  - with the fix, release: stops before `main()`; debug: prints
    "SysTick_Config(0) failed, SystemCoreClock 0" and stops (QEMU logs
    a "Bad read" for DHCSR, which it does not model, as for the fault
    handlers);
- the debug build first failed with `-Wold-style-cast` on the CMSIS
  `CoreDebug` macro; the access is wrapped in a diagnostic push/pop, as
  for the other SCB accesses in the port;
- QEMU M0, M3, M4F and M7F suites (debug and release) pass, with no
  warnings.

---

## #10 - `is_priority_valid()` reads out of `SCB->SHP` in NMI and HardFault

**File:** `include/cmsis-plus/rtos/port/os-inlines.h:166`

**Status:** fixed in e47269d (issue #6; test in the core 5903ab8a):
`is_priority_valid()` returns `false` for Reset, NMI and HardFault on
all architectures.

### Problem

For exceptions with a fixed priority (NMI, HardFault), CMSIS
`NVIC_GetPriority()` computes the `SHP` index as `(IRQn & 0xF) - 4`,
which is negative, so it reads outside the array and the result is
meaningless. RTOS calls are not allowed from these handlers anyway, so
the impact is only a wrong assertion result.

### Suggested fix

Return `false` for exception numbers below 4 (Reset, NMI, HardFault)
before calling `NVIC_GetPriority()`.

### Analysis before fixing

The review understated it:

- on Armv7-M and Armv7E-M with BASEPRI, the out-of-bounds read is as
  described; the result depends on the bytes before `SHPR` in the SCB
  (the CPUID/ICSR/VTOR/AIRCR/SCR/CCR area), so it is not reliably wrong;
- on Armv6-M, and when the critical sections use PRIMASK, the function
  returned `true` unconditionally, so the assertions in the RTOS calls
  accepted calls from NMI and HardFault. Neither is masked by PRIMASK
  or BASEPRI, so a system call from them can interrupt a critical
  section and corrupt the scheduler lists;
- on other architectures (for example Armv8-M) the function had no
  `return` statement.

### Fix applied

- `is_priority_valid()` reads IPSR once and returns `false` for
  exception numbers 1 to 3 (Reset, NMI, HardFault), before the
  architecture-specific branch, so the check applies to all Cortex-M
  variants and `NVIC_GetPriority()` is never called with them;
- the BASEPRI branch is otherwise unchanged; the doc comment now says
  that NMI and HardFault have fixed priorities, are not masked by the
  critical sections, and system calls are never valid from them.

No change to the POSIX port (it has no such exceptions), the C API or
the data layout.

### Verification

- new test in the core "Interrupt priorities" (Cortex-M only, guarded
  by `__ARM_ARCH_PROFILE == 'M'`): a strong `NMI_Handler` overrides the
  weak default and records `is_priority_valid()`; the test pends NMI
  via `ICSR.NMIPENDSET` and checks that the handler ran and returned
  `false`. No test platform defines a strong `NMI_Handler` (the CubeMX
  `startup_*.s` aliases are weak and are not built);
- negative check, with the previous `os-inlines.h`: QEMU M0 debug
  fails ("NMI taken, is_priority_valid() true"); QEMU M7F debug passes,
  because the out-of-bounds bytes happen to read as a priority below
  the threshold, so on Armv7-M the test is a regression test only;
- with the fix, QEMU M0, M3, M4F and M7F (debug and release) pass, with
  no warnings; native clang passes (the test is compiled out there).

---

## #11 - Unmatched `#pragma GCC diagnostic pop`

**File:** `include/cmsis-plus/rtos/port/os-inlines.h:420`

**Status:** fixed in a572be7 (issue #5): the extra `pop` is removed; see
"Fix applied" below.

### Problem

The file has two `pop` and one `push`. The extra `pop` at the end
removes a diagnostic state pushed by whatever included the core
`os.h` before (the core `os.h` itself does not push before including
the port inlines), so it can re-enable warnings an includer had
disabled, or make the includer's own `pop` unmatched. The POSIX port's
file is balanced.

### Suggested fix

Remove the extra `pop`.

### Fix applied

`include/cmsis-plus/rtos/port/os-inlines.h`: the `#pragma GCC diagnostic
pop` before `#endif /* __cplusplus */` is removed; the file now has one
`push` and one `pop`. Includers' diagnostic suppressions now last until
their own `pop` (so at most fewer warnings, never new ones).

### Verification

No behaviour to test. Release builds (native clang, QEMU Cortex-M0, M3,
M4F, M7F) before and after, loadable contents compared with
`hash-builds.sh`: all 15 test binaries identical. The QEMU Cortex-M0
and M7F suites pass (debug and release; all test builds use
`-Werror`).

---

## #12 - Documentation

**Status:** fixed in b86a78b (no issue): header comments and `README.md`;
the `<signal.h>`/`<sys/time.h>` includes are kept (see below).

- `os-inlines.h` header comment: "If contains the implementation for
  all objects, using the FreeRTOS API" - copied from another port; it
  contains the Cortex-M inline port functions.
- `os-decls.h` includes `<signal.h>` and `<sys/time.h>` ("Platform
  definitions"), which the Cortex-M port does not use.
- `README.md`:
  - "Dependencies: none" - the port needs the portable core and the
    CMSIS device header (`<cmsis_device.h>`);
  - "Limitations: none" - only Armv6-M and Armv7-M are supported (#4),
    and only GCC (#6);
  - "compiled with GCC 11/12, clang 12/13/14/15" - not true for
    `os-core.cpp` (#6);
  - the CMake example ends with a stray backtick
    (`add_subdirectory(...)\``);
  - "Status: ... end-of-life ... not recommended for new designs" -
    check whether this is still intended, given the current IIIe 7.2.0
    work.
- `is_reschedule_pending` is written but never read (see #1) - addressed
  by #1, it is now read by the core at unlock.

### Analysis before fixing

- the review missed more wrong header comments: `os-inlines.h` also
  said it was "included in all src/os-*.cpp files" (it is included at
  the end of `rtos/os.h`); `os-decls.h` said it was included from
  `rtos/os.h` (it is included from `rtos/os-decls.h` and
  `rtos/os-c-decls.h`); `os-c-decls.h` said it was included from
  `rtos/os-c-decls.h` (it is included from the port `os-decls.h` and
  from `startup/exception-handlers.c`);
- removing `<signal.h>` and `<sys/time.h>` is not recommended: the port
  does not use them, but on Cortex-M it is the only path through which
  `<cmsis-plus/rtos/os.h>` provides them (the core headers do not
  include them, the POSIX port does), so removing them may break the
  build of applications for no functional gain;
- the Cortex-M tests build only with arm-none-eabi-gcc 14 (clang is
  used only for native builds, which do not compile this port), so the
  README compiler line was wrong for both compilers.

### Fix applied

- `os-inlines.h`, `os-decls.h`, `os-c-decls.h`: header comments describe
  the content and where each file is included;
- `os-decls.h`: the "Platform definitions" comment replaced by one
  saying why the two includes are kept;
- `README.md`:
  - Status: aligned with the core README (fully functional, IVe work in
    progress), no longer end-of-life;
  - compilers: arm-none-eabi-gcc 14, clang not supported yet;
  - Dependencies: the portable part (`micro-os-plus::iii`) and the
    CMSIS core and device headers;
  - Limitations: Armv8-M/Armv8.1-M (#4), GCC only (#6), Cortex-M7
    r0p1 erratum (#5); to be updated when these are decided;
  - Tests: the core tests (QEMU and physical boards);
  - the stray backtick in the CMake example removed.

The core README has the same outdated compiler line; not changed here.

### Verification

- no code change; the QEMU M0 and M7F suites (debug and release) pass
  with no warnings.

---

## #13 - `reschedule()` before the scheduler is started

**File:** `src/rtos/os-core.cpp:562` (`port::scheduler::reschedule()`);
the same in the POSIX port

**Status:** fixed in 32662b5 (issue #3), POSIX port 9220598, test in the
core 6a11d02; found while fixing #1. A regression of the core startup
change micro-os-plus/micro-os-plus-iii#142.

### Problem

Since micro-os-plus-iii#142, `_start()` enables the interrupts after the
DATA and BSS initialisation, so the static constructors run with
interrupts enabled. A constructor that requests a context switch outside
a scheduler critical section (for example `flags_raise()` or `resume()`
on a thread, which end in `port::scheduler::reschedule()`) pends PendSV,
which is then taken at once, before the scheduler exists:
`switch_stacks()` saves a context from an uninitialised PSP into
`current_thread_` (the last thread constructed, or `nullptr`).

Thread construction itself is safe (it runs `resume()` under the
scheduler lock, and `yield()` does nothing before the start), which is
why the first experiment, a thread in a static constructor, did not show
it.

### Scenario

Test: a global `thread_inclusive` object, plus a second global object
whose constructor calls `flags_raise (0x1)` on it.

- Cortex-M (QEMU Cortex-M7, debug), old port: `[HardFault]` before
  `main()`, test timeout.
- POSIX (native), old port: the run continues but later hangs.

### Fix applied

- Both ports, `reschedule()`: return at once when
  `!rtos::scheduler::started ()`. Threads made ready before the start run
  after `scheduler::start()`; the core sets the started flag before
  calling `port::scheduler::start()`, so the first switch still works.
  Requests before the start are neither performed nor recorded (the
  core `start()` also clears `is_reschedule_pending`, see #1).

### Verification

New test in the core `tests/sources/rtos-apis/src/test-cpp-api.cpp`
(start of the "Threads" section): the thread created by the static
constructor must have run, and `scheduler::started ()` must have been
true when it ran. Its stack is inside the object (`thread_inclusive`): a
first version used a heap allocated stack, kept for the whole program,
which exhausted the small `sys-heap` of the QEMU platform and made a
later ISO test fail (`attempt to destruct a running thread` after a
failed thread construction), unrelated to the scheduler.

Negative check: with the old ports, `[HardFault]` on QEMU Cortex-M7 and
a hang natively; with the fix, `test-native-cmake-clang`,
`test-native-cmake-gcc` and the QEMU Cortex-M0, M3, M4F and M7F suites
pass (debug and release).

