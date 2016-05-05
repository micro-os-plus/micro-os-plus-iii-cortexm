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

#include <cassert>

#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/rtos/port/os-inlines.h>

// Better be the last, to undef putchar()
#include <cmsis-plus/diag/trace.h>

#include <cmsis-plus/iso/malloc.h>

uint32_t signal_nesting;

namespace os
{
  namespace rtos
  {
    namespace port
    {
      // ----------------------------------------------------------------------

      namespace thread
      {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

        void
        Context::create (rtos::thread::Context* context, void* func, void* args)
        {
          memset (&context->port_, 0, sizeof(context->port_));

          ucontext_t* ctx =
              reinterpret_cast<ucontext_t*> (&(context->port_.ucontext));
          trace::printf ("%s() getcontext %p\n", __func__, ctx);
          if (getcontext (ctx) != 0)
            {
              trace::printf ("%s getcontext failed with %s\n", __func__,
                             strerror (errno));
              abort ();
            }

          // The context in itself is not needed, but makecontext()
          // requires a context obtained by getcontext().

          // Remove the parent link.
          // TODO: maybe use this to link to exit code.
          ctx->uc_link = 0;

          // Configure the new stack to default values.
          ctx->uc_stack.ss_sp = estd::malloc (SIGSTKSZ);
          ctx->uc_stack.ss_size = SIGSTKSZ;
          ctx->uc_stack.ss_flags = 0;

          trace::printf ("%s() makecontext %p\n", __func__, ctx);
          makecontext (ctx, reinterpret_cast<func_t> (func), 1, args);

          // context->port_.saved = false;
        }

#pragma GCC diagnostic pop

      } /* namespace thread */

      // ----------------------------------------------------------------------

      namespace scheduler
      {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

        void
        reschedule (bool save)
        {
          // For some complicated reasons, the context save/restore
          // functions must be called in the same the function.
          // The idea to inline functions does not work, since
          // the compiler does not inline functions with context calls.

          if (save)
            {
              if (rtos::scheduler::locked ())
                {
                  return;
                }

              rtos::Thread* old_thread;
              ucontext_t* old_ctx;
              ucontext_t* new_ctx;

                {
                  rtos::interrupts::Critical_section ics;

                  old_thread = rtos::scheduler::current_thread_;
                  if (old_thread->sched_state_ == rtos::thread::state::running)
                    {
                      old_thread->sched_state_ = rtos::thread::state::waiting;
                    }

                  old_ctx =
                      reinterpret_cast<ucontext_t*> (&old_thread->context ().port_.ucontext);
                  // Select the top priority thread
                  rtos::scheduler::current_thread_ =
                      rtos::scheduler::ready_threads_list_.remove_top ();
                  new_ctx =
                      reinterpret_cast<ucontext_t*> (&rtos::scheduler::current_thread_->context ().port_.ucontext);
                }

              if (old_ctx != new_ctx)
                {
                  trace::printf ("%s() swapcontext %s -> %s \n", __func__,
                                 old_thread->name (),
                                 rtos::scheduler::current_thread_->name ());

                  if (swapcontext (old_ctx, new_ctx) != 0)
                    {
                      trace::printf ("%s() swapcontext failed with %s\n",
                                     __func__, strerror (errno));
                      abort ();
                    }
                }
              else
                {
                  trace::printf ("%s() nop %s\n", __func__,
                                 old_thread->name ());

                }
            }
          else
            {
              rtos::thread::Context* context;

                {
                  rtos::interrupts::Critical_section ics;

                  // Select the top priority thread
                  rtos::scheduler::current_thread_ =
                      rtos::scheduler::ready_threads_list_.remove_top ();

                  context = &(rtos::scheduler::current_thread_->context ());
                }

              trace::printf ("%s() setcontext %p %s\n", __func__,
                             &context->port_.ucontext,
                             rtos::scheduler::current_thread_->name ());

              // context->port_.saved = false;
              ucontext_t* ctx =
                  reinterpret_cast<ucontext_t*> (&context->port_.ucontext);
              if (setcontext (ctx) != 0)
                {
                  trace::printf ("%s() setcontext failed with %s\n", __func__,
                                 strerror (errno));
                  abort ();
                }
            }
        }

#pragma GCC diagnostic pop
      } /* namespace scheduler */

      // ----------------------------------------------------------------------

      static void
      systick_clock_signal_handler (int signum)
      {
        if (signum != clock::signal_number)
          {
            char ce = '?';

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

            write (1, &ce, 1);

#pragma GCC diagnostic pop

            return;
          }

#if 0
        char c = '*';
        write (1, &c, 1);
#endif

        signal_nesting++;
        // Call the ticks timer ISR.
        os_systick_handler ();
        signal_nesting--;
      }

      // ======================================================================

      void
      Systick_clock::start (void)
      {
        // set handler
        struct sigaction sa;
#if defined(__APPLE__)
        sa.__sigaction_u.__sa_handler = systick_clock_signal_handler;
#else
#pragma GCC diagnostic push
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
        sa.sa_handler = signal_handler;
#pragma GCC diagnostic pop
#endif
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        if (sigaction (clock::signal_number, &sa, nullptr) != 0)
          {
            trace::printf ("sigaction() failed\n");
            abort ();
          }

        // set timer
        struct itimerval tv;
        // first clear all fields
        timerclear(&tv.it_value);
        // then set the required ones

#if 1
        tv.it_value.tv_sec = 0;
        tv.it_value.tv_usec = 1000000 / rtos::Systick_clock::frequency_hz;
        tv.it_interval.tv_sec = 0;
        tv.it_interval.tv_usec = 1000000 / rtos::Systick_clock::frequency_hz;
#else
        tv.it_value.tv_sec = 1;
        tv.it_value.tv_usec = 0; //1000000 / rtos::Systick_clock::frequency_hz;
        tv.it_interval.tv_sec = 1;
        tv.it_interval.tv_usec = 0;//1000000 / rtos::Systick_clock::frequency_hz;
#endif

        if (setitimer (ITIMER_REAL, &tv, NULL) != 0)
          {
            trace::printf ("setitimer() failed\n");
            abort ();
          }

#if 0
        // Used for initial debugging, to see the signals arriving
        pause ();
        for (int i = 50; i > 0; --i)
          {
            for (int j = 100; j > 0; --j)
              {
                char c = '.';
                write (1, &c, 1);
              }
            char cn = '\n';
            write (1, &cn, 1);
          }
#endif

      }

    } /* namespace port */
  } /* namespace rtos */
} /* namespace os */
