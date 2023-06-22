/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2016-2023 Liviu Ionescu. All rights reserved.
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
 * If contains the implementation for all objects, using the
 * FreeRTOS API.
 *
 * This file is included in all src/os-*.cpp files.
 */

#ifndef CMSIS_PLUS_RTOS_PORT_OS_INLINES_H_
#define CMSIS_PLUS_RTOS_PORT_OS_INLINES_H_

// ----------------------------------------------------------------------------

#include <cmsis-plus/os-app-config.h>
#include <cmsis-plus/rtos/os-c-decls.h>

// ----------------------------------------------------------------------------

#ifdef  __cplusplus

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

        inline void
        __attribute__((always_inline))
        greeting (void)
        {
          trace::printf ("Scheduler: µOS++ Cortex-M");
#if defined(__ARM_ARCH_7EM__)
          trace::putchar ('0'+__CORTEX_M); // M4/M7
#if defined (__ARM_FP)
          trace::printf (" FP");
#endif
#elif defined(__ARM_ARCH_7M__)
          trace::printf ("3");
#elif defined(__ARM_ARCH_6M__)
          trace::printf ("0/0+");
#endif
          trace::printf (", preemptive");
#if (defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)) && defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)
          trace::printf (", BASEPRI(%u)", OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY);
#else
          trace::printf(", DI/EI");
#endif
#if defined(OS_EXCLUDE_RTOS_IDLE_SLEEP)
          trace::printf(", no WFI");
#else
          trace::printf(", WFI");
#endif
          trace::puts ("");

          // At this stage the system clock should have already been configured
          // at high speed by __initialise_hardware().
          trace::printf ("System clock: %u Hz\n", SystemCoreClock);
        }

        inline port::scheduler::state_t
        __attribute__((always_inline))
        lock (void)
        {
          return locked (state::locked);
        }

        inline port::scheduler::state_t
        __attribute__((always_inline))
        unlock (void)
        {
          return locked (state::unlocked);
        }

        inline bool
        __attribute__((always_inline))
        locked (void)
        {
          return lock_state != state::unlocked;
        }

        inline void
        __attribute__((always_inline))
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
        inline bool
        __attribute__((always_inline))
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
         * interrupt priorities are valid.
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
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

#if defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)

          // The trick here is to properly account for differences
          // between Cortex-M exception number stored in IPSR (an
          // unsigned byte), and the CMSIS interrupt number (IRQn),
          // which is [-15 ... -1, 0 ... 239 ], in other words
          // it is negative for the system exceptions.
          uint32_t exception_number = __get_IPSR ();
          if (exception_number > 0)
            {
              uint32_t prio = NVIC_GetPriority (
                  static_cast<IRQn_Type> (exception_number - 16));
              return (prio
                  >= OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY);
            }
          else
            {
              return true;
            }

#else

          // When using DI/EI, all priority levels are valid.
          return true;

#endif /* defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY) */

#elif defined(__ARM_ARCH_6M__)

          // When using DI/EI, all priority levels are valid.
           return true;

#endif /* architecture */
        }

        // Enter an IRQ critical section
        inline rtos::interrupts::state_t
        __attribute__((always_inline))
        critical_section::enter (void)
        {
          uint32_t pri;

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

#if defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)

          // Read the current BASEPRI, to be returned and later restored.
          pri = __get_BASEPRI ();

          // According to ARM "MSR{cond} spec_reg, Rn":
          // When you write to BASEPRI_MAX, the instruction writes to
          // BASEPRI only if either:
          // Rn is non-zero and the current BASEPRI value is 0
          // Rn is non-zero and less than the current BASEPRI value.
          __set_BASEPRI_MAX (
              OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY
              << ((8 - __NVIC_PRIO_BITS)));

#else

          // Read the current PRIMASK, to be returned and later restored.
          pri = __get_PRIMASK ();

          // Disable all interrupts.
          __disable_irq ();

#endif /* defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY) */

#elif defined(__ARM_ARCH_6M__)

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
        inline void
        __attribute__((always_inline))
        critical_section::exit (rtos::interrupts::state_t state)
        {
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

#if defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)

          // Restore BASEPRI to the value saved by enter().
          __set_BASEPRI (state);

#else

          // Restore PRIMASK to the value saved by enter().
          __set_PRIMASK (state);

#endif /* defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY) */

#elif defined(__ARM_ARCH_6M__)

          // Restore PRIMASK to the value saved by enter().
          __set_PRIMASK (state);

#endif

          // __DSB ();
          // __ISB ();
        }

        // ====================================================================

        // Enter an IRQ uncritical section
        inline rtos::interrupts::state_t
        __attribute__((always_inline))
        uncritical_section::enter (void)
        {
          uint32_t pri;

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

#if defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)

          pri = __get_BASEPRI ();

          // Setting BASEPRI to 0 makes it ineffective,
          // practically enabling all interrupts.
          __set_BASEPRI (0);

#else

          pri = __get_PRIMASK ();

          // Enable all interrupts.
          __enable_irq ();

#endif /* defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY) */

#elif defined(__ARM_ARCH_6M__)

          pri = __get_PRIMASK ();

          // Enable all interrupts.
          __enable_irq ();

#endif

          // __DSB ();
          // __ISB ();

          return pri;
        }

        // Exit an IRQ critical section
        inline void
        __attribute__((always_inline))
        uncritical_section::exit (rtos::interrupts::state_t state)
        {
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

#if defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY)

          // Restore BASEPRI to the value saved by enter().
          __set_BASEPRI (state);

#else

          // Restore PRIMASK to the value saved by enter().
          __set_PRIMASK (state);

#endif /* defined(OS_INTEGER_RTOS_CRITICAL_SECTION_INTERRUPT_PRIORITY) */

#elif defined(__ARM_ARCH_6M__)

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
        inline void
        __attribute__((always_inline))
        prepare_suspend (void)
        {
          ;
        }

      } /* namespace this_thread */

      // ======================================================================

      inline void
      __attribute__((always_inline))
      clock_highres::start (void)
      {
        ;
      }

      inline uint32_t
      __attribute__((always_inline))
      clock_highres::input_clock_frequency_hz (void)
      {
        // The SysTick is clocked with the CPU clock.
        return SystemCoreClock;
      }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
      inline uint32_t
      __attribute__((always_inline))
      clock_highres::cycles_per_tick (void)
      {
        return SysTick->LOAD + 1;
      }

      inline uint32_t
      __attribute__((always_inline))
      clock_highres::cycles_since_tick (void)
      {
        uint32_t load_value = SysTick->LOAD;

        // Initial sample of the decrementing counter.
        // If this happens before the event, will be used as such.
        uint32_t val = SysTick->VAL;

        // Check overflow. If the exception is pending, it means that the
        // interrupt occurred during this critical section and was not
        // yet processed, so the total cycles count in steady_count_
        // does not yet reflect the correct value and needs to be
        // adjusted by one full cycle length.
        if (SysTick->CTRL & SCB_ICSR_PENDSTSET_Msk)
          {
            // Sample the decrementing counter again to validate the
            // initial sample.
            uint32_t val_subsequent = SysTick->VAL;

            // If the value did decrease, the timer did not recycle
            // between the two reads; in other words, the interrupt
            // occurred before the first read.
            if (val > val_subsequent)
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

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------

#endif /* __cplusplus */

#endif /* CMSIS_PLUS_RTOS_PORT_OS_INLINES_H_ */
