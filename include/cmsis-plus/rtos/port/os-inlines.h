/*
 * This file is part of the µOS++ project (https://micro-os-plus.github.io/).
 * Copyright (c) 2016-2025 Liviu Ionescu. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose is hereby granted, under the terms of the MIT license.
 *
 * If a copy of the license was not distributed with this file, it can
 * be obtained from https://opensource.org/licenses/mit/.
 */

/*
 * This file is part of the CMSIS++ proposal, intended as a CMSIS
 * replacement for C++ applications.
 *
 * It contains the Cortex-M inline implementations of the
 * `os::rtos::port` functions (critical sections, interrupts,
 * scheduler and clock helpers).
 *
 * It is included at the end of `cmsis-plus/rtos/os.h`.
 */

#ifndef CMSIS_PLUS_RTOS_PORT_OS_INLINES_H_
#define CMSIS_PLUS_RTOS_PORT_OS_INLINES_H_

// ----------------------------------------------------------------------------

#if defined(OS_USE_OS_APP_CONFIG_H)
#include <cmsis-plus/os-app-config.h>
#endif

#include <cmsis-plus/rtos/os-c-decls.h>

// ----------------------------------------------------------------------------

#ifdef __cplusplus

#include <cmsis_device.h>

#include <cmsis-plus/diag/trace.h>

namespace os
{
  namespace rtos
  {
    namespace port
    {
      // ----------------------------------------------------------------------

      namespace scheduler
      {

        inline void __attribute__ ((always_inline))
        greeting (void)
        {
          trace::printf ("Scheduler: µOS++ Cortex-M");
// Mainline with DSP: v7E-M (M4, M7).
#if (__ARM_ARCH_ISA_THUMB == 2) && defined(__ARM_FEATURE_DSP)
          trace::putchar ('0' + __CORTEX_M); // M4/M7
#if defined(__ARM_FP)
          trace::printf (" FP");
#endif
// Mainline without DSP: v7-M (M3).
#elif (__ARM_ARCH_ISA_THUMB == 2)
          trace::printf ("3");
// Baseline: v6-M (M0, M0+).
#elif (__ARM_ARCH_ISA_THUMB == 1)
          trace::printf ("0/0+");
#else
#error "Not yet implemented for this architecture."
#endif
          trace::printf (", preemptive");
// Mainline: v7-M, v7E-M, v8-M Main, v8.1-M (M3, M4, M7, M33, ...).
#if (__ARM_ARCH_ISA_THUMB == 2) \
    && defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)
          trace::printf (", BASEPRI(%u)",
                         OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY);
#else
          trace::printf (", DI/EI");
#endif
#if defined(OS_EXCLUDE_RTOS_IDLE_SLEEP)
          trace::printf (", no WFI");
#else
          trace::printf (", WFI");
#endif
          trace::puts ("");

          // At this stage the system clock should have already been configured
          // at high speed by __initialise_hardware().
          trace::printf ("System clock: %u Hz\n", SystemCoreClock);
        }

        inline port::scheduler::state_t __attribute__ ((always_inline))
        lock (void)
        {
          return locked (state::locked);
        }

        inline port::scheduler::state_t __attribute__ ((always_inline))
        unlock (void)
        {
          return locked (state::unlocked);
        }

        inline bool __attribute__ ((always_inline))
        locked (void)
        {
          return lock_state != state::unlocked;
        }

        inline void __attribute__ ((always_inline))
        wait_for_interrupt (void)
        {
#if !defined(OS_EXCLUDE_RTOS_IDLE_SLEEP)
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
          trace::printf ("%s() \n", __func__);
#endif
          // The DSB is recommended by ARM before WFI.
          __DSB ();
          __WFI ();
#endif /* !defined(OS_EXCLUDE_RTOS_IDLE_SLEEP) */
        }

      } /* namespace scheduler */

      // The DSB/ISB are recommended by ARM after priority level changes.
      // (Application Note 321: ARM Cortex-M Programming Guide to
      // Memory Barrier Instructions)
      // http://infocenter.arm.com/help/topic/com.arm.doc.dai0321a/DAI0321A_programming_guide_memory_barriers_for_m_profile.pdf

      namespace interrupts
      {
        inline void __attribute__ ((always_inline))
        set_stack_limit (const void* bottom)
        {
// Armv8-M and Armv8.1-M, Baseline and Mainline (M23, M33, M55, M85) have
// the MSPLIM register; the earlier architectures have no stack limit.
// Not the CMSIS `__set_MSPLIM()`, which tests `__ARM_ARCH_8M_MAIN__` and
// does nothing on Armv8.1-M.
#if (__ARM_ARCH >= 8)
          // The low 3 bits of MSPLIM are ignored; round up, to never
          // allow more than the stack area.
          uint32_t limit = (static_cast<uint32_t> (
                                reinterpret_cast<uintptr_t> (bottom))
                            + 7U)
                           & ~7U;
          __asm__ volatile ("msr msplim, %0" : : "r"(limit) :);
// Mainline: v8-M Main, v8.1-M (M33, M55, M85).
#if (__ARM_ARCH_ISA_THUMB == 2)
          // Ignore the limit in HardFault and NMI, otherwise the
          // HardFault that reports the stack overflow cannot stack its
          // exception frame, and the processor locks up.
          // SCB->CCR |= SCB_CCR_STKOFHFNMIGN_Msk; the CMSIS `SCB` macro
          // uses an old-style cast, rejected by the C++ warnings.
          *reinterpret_cast<volatile uint32_t*> (0xE000ED14UL) |= (1UL << 10);
#endif
#else
          (void)bottom;
#endif
        }

// Mainline: v7-M, v7E-M, v8-M Main, v8.1-M (M3, M4, M7, M33, ...).
#if (__ARM_ARCH_ISA_THUMB == 2) \
    && defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)

        namespace internal
        {
          // Cortex-M7 r0p1 erratum 837070: a write that raises BASEPRI
          // may not take effect immediately, so an interrupt that should
          // be masked can still be taken right after it. Arm's workaround
          // is to raise it with interrupts disabled. PRIMASK is saved and
          // restored (not just enabled), since critical sections may also
          // run with interrupts already disabled.
          // Enabled for all Cortex-M7 builds; parts with r1p0 or later
          // (STM32F76x/F77x, STM32H7) may define
          // OS_DISABLE_CORTEXM7_ERRATUM_837070_WORKAROUND.

          // Raise BASEPRI with BASEPRI_MAX (written only if it raises).
          inline void __attribute__ ((always_inline))
          raise_basepri_max (uint32_t value)
          {
#if (__CORTEX_M == 7) \
    && !defined(OS_DISABLE_CORTEXM7_ERRATUM_837070_WORKAROUND)
            uint32_t primask = __get_PRIMASK ();
            __disable_irq ();
            __set_BASEPRI_MAX (value);
            __set_PRIMASK (primask);
#else
            __set_BASEPRI_MAX (value);
#endif
          }

          // Set BASEPRI to a value that may raise it.
          inline void __attribute__ ((always_inline))
          raise_basepri (uint32_t value)
          {
#if (__CORTEX_M == 7) \
    && !defined(OS_DISABLE_CORTEXM7_ERRATUM_837070_WORKAROUND)
            uint32_t primask = __get_PRIMASK ();
            __disable_irq ();
            __set_BASEPRI (value);
            __set_PRIMASK (primask);
#else
            __set_BASEPRI (value);
#endif
          }
        } /* namespace internal */

#endif

        inline bool __attribute__ ((always_inline))
        in_handler_mode (void)
        {
          // In Handler mode, IPSR holds the exception number.
          // If 0, the core is in thread mode.
          return (__get_IPSR () != 0);
        }

        /**
         * @details
         * In the simplest form, CMSIS++ can disable all interrupts
         * when entering a critical region, and in this case all
         * configurable interrupt priorities are valid.
         *
         * NMI and HardFault have fixed priorities, higher than any
         * configurable one, and are not masked by the critical
         * sections; system calls are never valid from them.
         *
         * If the application requires to have some very fast
         * interrupts, never affected by the system critical regions,
         * then while running inside the corresponding interrupt
         * handlers, the user must not be allowed to invoke
         * any system calls that may interfere with the scheduler.
         *
         * If the OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY
         * is defined, the numeric value of the current interrupt
         * priority must be higher than or equal to the macro definition
         * (lower priorities).
         */
        inline bool
        is_priority_valid (void)
        {
          uint32_t exception_number = __get_IPSR ();

          // Reset (1), NMI (2) and HardFault (3) have fixed priorities,
          // higher than any configurable one; they are masked neither by
          // BASEPRI nor by PRIMASK, so system calls are never valid from
          // them. (Also, NVIC_GetPriority() would index outside SHPR.)
          if (exception_number > 0 && exception_number < 4)
            {
              return false;
            }

// Mainline: v7-M, v7E-M, v8-M Main, v8.1-M (M3, M4, M7, M33, ...).
#if (__ARM_ARCH_ISA_THUMB == 2) \
    && defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)

          // The trick here is to properly account for differences
          // between Cortex-M exception number stored in IPSR (an
          // unsigned byte), and the CMSIS interrupt number (IRQn),
          // which is [-15 ... -1, 0 ... 239 ], in other words
          // it is negative for the system exceptions.
          if (exception_number > 0)
            {
              uint32_t prio = NVIC_GetPriority (
                  static_cast<IRQn_Type> (exception_number - 16));
              return (prio
                      >= OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY);
            }
          return true;

#else

          // When using DI/EI (always on ARMv6-M), all configurable
          // priority levels are valid.
          return true;

#endif
        }

        // Enter an IRQ critical section
        inline rtos::interrupts::state_t __attribute__ ((always_inline))
        critical_section::enter (void)
        {
          uint32_t pri;

// BASEPRI is available on Mainline (v7-M, v7E-M, v8-M Main, v8.1-M);
// otherwise (DI/EI, and always on Baseline) PRIMASK is used, which
// exists on all M-profile architectures.
#if (__ARM_ARCH_ISA_THUMB == 2) \
    && defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)

          // Read the current BASEPRI, to be returned and later restored.
          pri = __get_BASEPRI ();

          // According to ARM "MSR{cond} spec_reg, Rn":
          // When you write to BASEPRI_MAX, the instruction writes to
          // BASEPRI only if either:
          // Rn is non-zero and the current BASEPRI value is 0
          // Rn is non-zero and less than the current BASEPRI value.
          internal::raise_basepri_max (
              OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY
              << ((8 - __NVIC_PRIO_BITS)));

#else

          // Read the current PRIMASK, to be returned and later restored.
          pri = __get_PRIMASK ();

          // Disable all interrupts.
          __disable_irq ();

#endif

          // Apparently not required by architecture, but used by
          // FreeRTOS, with an unconvincing motivation ("...  ensure
          // the code is completely within the specified behaviour
          // for the architecture").
          // __DSB ();
          // __ISB ();

          return pri;
        }

        // Exit an IRQ critical section
        inline void __attribute__ ((always_inline))
        critical_section::exit (rtos::interrupts::state_t state)
        {
// BASEPRI is available on Mainline (v7-M, v7E-M, v8-M Main, v8.1-M);
// otherwise (DI/EI, and always on Baseline) PRIMASK is used, which
// exists on all M-profile architectures.
#if (__ARM_ARCH_ISA_THUMB == 2) \
    && defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)

          // Restore BASEPRI to the value saved by enter().
          __set_BASEPRI (state);

#else

          // Restore PRIMASK to the value saved by enter().
          __set_PRIMASK (state);

#endif

          // __DSB ();
          // __ISB ();
        }

        // ====================================================================

        // Enter an IRQ uncritical section
        inline rtos::interrupts::state_t __attribute__ ((always_inline))
        uncritical_section::enter (void)
        {
          uint32_t pri;

// BASEPRI is available on Mainline (v7-M, v7E-M, v8-M Main, v8.1-M);
// otherwise (DI/EI, and always on Baseline) PRIMASK is used, which
// exists on all M-profile architectures.
#if (__ARM_ARCH_ISA_THUMB == 2) \
    && defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)

          pri = __get_BASEPRI ();

          // Setting BASEPRI to 0 makes it ineffective,
          // practically enabling all interrupts.
          __set_BASEPRI (0);

#else

          pri = __get_PRIMASK ();

          // Enable all interrupts.
          __enable_irq ();

#endif

          // __DSB ();
          // __ISB ();

          return pri;
        }

        // Exit an IRQ critical section
        inline void __attribute__ ((always_inline))
        uncritical_section::exit (rtos::interrupts::state_t state)
        {
// BASEPRI is available on Mainline (v7-M, v7E-M, v8-M Main, v8.1-M);
// otherwise (DI/EI, and always on Baseline) PRIMASK is used, which
// exists on all M-profile architectures.
#if (__ARM_ARCH_ISA_THUMB == 2) \
    && defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)

          // Restore BASEPRI to the value saved by enter(); this raises it
          // back to the critical section level.
          internal::raise_basepri (state);

#else

          // Restore PRIMASK to the value saved by enter().
          __set_PRIMASK (state);

#endif

          // __DSB ();
          // __ISB ();
        }

      } /* namespace interrupts */

      // ======================================================================

      namespace this_thread
      {
        inline void __attribute__ ((always_inline))
        prepare_suspend (void)
        {
          ;
        }

      } /* namespace this_thread */

      // ======================================================================

      inline void __attribute__ ((always_inline))
      clock_highres::start (void)
      {
        ;
      }

      inline uint32_t __attribute__ ((always_inline))
      clock_highres::input_clock_frequency_hz (void)
      {
        // The SysTick is clocked with the CPU clock.
        return SystemCoreClock;
      }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
      inline uint32_t __attribute__ ((always_inline))
      clock_highres::cycles_per_tick (void)
      {
        return SysTick->LOAD + 1;
      }

      inline uint32_t __attribute__ ((always_inline))
      clock_highres::cycles_since_tick (void)
      {
        uint32_t load_value = SysTick->LOAD;

        // Initial sample of the decrementing counter.
        // If this happens before the event, will be used as such.
        uint32_t val = SysTick->VAL;

        // Check overflow. If the SysTick exception is pending (in ICSR),
        // the counter wrapped during this critical section and the
        // interrupt was not yet processed, so the total cycles count in
        // steady_count_ does not yet reflect the correct value and needs
        // to be adjusted by one full cycle length.
        // Only one wrap can be detected; a critical section longer than
        // one tick loses the additional ticks.
        if (SCB->ICSR & SCB_ICSR_PENDSTSET_Msk)
          {
            // Sample the decrementing counter again to validate the
            // initial sample.
            uint32_t val_subsequent = SysTick->VAL;

            // If the value did not increase (equal included), the timer
            // did not recycle between the two reads; in other words, the
            // wrap occurred before the first read.
            if (val >= val_subsequent)
              {
                // The count must be adjusted with a full cycle.
                return load_value + 1 + (load_value - val);
              }
          }

        return load_value - val;
      }
#pragma GCC diagnostic pop

      // ======================================================================

    } /* namespace port */
  } /* namespace rtos */
} /* namespace os */

// ----------------------------------------------------------------------------

#endif /* __cplusplus */

#endif /* CMSIS_PLUS_RTOS_PORT_OS_INLINES_H_ */
