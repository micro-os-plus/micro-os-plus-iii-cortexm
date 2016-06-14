/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2016 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
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

#include <cmsis-plus/rtos/os-app-config.h>
#include <cmsis-plus/rtos/os-c-decls.h>

// ----------------------------------------------------------------------------

#ifdef  __cplusplus

#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/time.h>

// For Linux
#include <unistd.h>

#include <cmsis-plus/diag/trace.h>
#include <cmsis-plus/iso/malloc.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

extern "C" uint32_t signal_nesting;

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
          struct utsname name;
          if (::uname (&name) != -1)
            {
              trace::printf ("POSIX synthetic, running on %s %s %s",
                             name.machine, name.sysname, name.release);
            }
          else
            {
              trace::printf ("POSIX synthetic");
            }

          trace::puts ("; non-preemptive.");
        }

        inline bool
        __attribute__((always_inline))
        in_handler_mode (void)
        {
          return (signal_nesting > 0);
        }

        inline void
        __attribute__((always_inline))
        lock (rtos::scheduler::status_t status)
        {
          ;
        }

        inline void
        __attribute__((always_inline))
        _wait_for_interrupt (void)
        {
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
          trace::printf ("%s() \n", __func__);
#endif
          pause ();
        }

      } /* namespace scheduler */

      namespace interrupts
      {

        inline bool
        __attribute__((always_inline))
        is_priority_valid (void)
        {
          return true;
        }

        // Enter an IRQ critical section
        inline rtos::interrupts::status_t
        __attribute__((always_inline))
        critical_section::enter (void)
        {
          sigset_t old;
          sigprocmask (SIG_BLOCK, &clock_set, &old);

          return sigismember(&old, clock::signal_number);
        }

        // Exit an IRQ critical section
        inline void
        __attribute__((always_inline))
        critical_section::exit (rtos::interrupts::status_t status)
        {
          sigprocmask (status ? SIG_BLOCK : SIG_UNBLOCK, &clock_set, nullptr);
        }

        // ====================================================================

        // Enter an IRQ uncritical section
        inline rtos::interrupts::status_t
        __attribute__((always_inline))
        uncritical_section::enter (void)
        {
          sigset_t old;
          sigprocmask (SIG_UNBLOCK, &clock_set, &old);

          return sigismember(&old, clock::signal_number);
        }

        // Exit an IRQ critical section
        inline void
        __attribute__((always_inline))
        uncritical_section::exit (rtos::interrupts::status_t status)
        {
          sigprocmask (status ? SIG_BLOCK : SIG_UNBLOCK, &clock_set, nullptr);
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

    // ========================================================================

    } /* namespace port */
  } /* namespace rtos */
} /* namespace os */

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------

#endif /* __cplusplus */

#endif /* CMSIS_PLUS_RTOS_PORT_OS_INLINES_H_ */
