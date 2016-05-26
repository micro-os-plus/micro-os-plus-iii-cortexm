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

#if defined(__APPLE__) || defined(__linux__)

// ----------------------------------------------------------------------------

#include <cassert>

#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/rtos/port/os-inlines.h>

// Better be the last, to undef putchar()
#include <cmsis-plus/diag/trace.h>

#include <cmsis-plus/iso/malloc.h>

#include <sys/time.h>

uint32_t signal_nesting;

namespace os
{
  namespace rtos
  {
    namespace port
    {
      // ----------------------------------------------------------------------

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

      void
      context::create (void* context, void* func, void* args)
      {
        class rtos::thread::context* th_ctx =
            static_cast<class rtos::thread::context*> (context);
        memset (&th_ctx->port_, 0, sizeof(th_ctx->port_));

        ucontext_t* ctx =
            reinterpret_cast<ucontext_t*> (&(th_ctx->port_.ucontext));

#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
        trace::printf ("%s() getcontext %p\n", __func__, ctx);
#endif

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
        ctx->uc_stack.ss_sp = th_ctx->stack ().bottom ();
        ctx->uc_stack.ss_size = th_ctx->stack ().size ();
        ctx->uc_stack.ss_flags = 0;

#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
        trace::printf ("%s() makecontext %p\n", __func__, ctx);
#endif
        makecontext (ctx, reinterpret_cast<func_t> (func), 1, args);

        // context->port_.saved = false;
      }

#pragma GCC diagnostic pop

      // ----------------------------------------------------------------------

      namespace scheduler
      {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

        void
        start (void)
        {
            {
              rtos::interrupts::critical_section ics;

              // Determine the next thread.
              rtos::scheduler::current_thread_ =
                  rtos::scheduler::ready_threads_list_.unlink_head ();
            }

          ucontext_t* new_context =
              reinterpret_cast<ucontext_t*> (&(rtos::scheduler::current_thread_->context_.port_.ucontext));

#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
          trace::printf ("%s() ctx %p %s\n", __func__, new_context,
                         rtos::scheduler::current_thread_->name ());
#endif

#if defined NDEBUG
          setcontext (new_context);
#else
          int res = setcontext (new_context);
          assert(res == 0);
#endif
          abort ();
        }

        void
        reschedule (void)
        {
          if (rtos::scheduler::locked () || rtos::scheduler::in_handler_mode ())
            {
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
              trace::printf ("%s() nop\n", __func__);
#endif
              return;
            }

#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
          trace::printf ("%s()\n", __func__);
#endif

          // For some complicated reasons, the context save/restore
          // functions must be called in the same the function.
          // The idea to inline functions does not work, since
          // the compiler does not inline functions with context calls.

          bool save = false;
          rtos::thread* old_thread;
          ucontext_t* old_ctx;
          ucontext_t* new_ctx;

            {
              rtos::interrupts::critical_section ics;

              old_thread = rtos::scheduler::current_thread_;
              if ((old_thread->sched_state_ == rtos::thread::state::running)
                  || (old_thread->sched_state_ == rtos::thread::state::waiting)
                  || (old_thread->sched_state_ == rtos::thread::state::ready))
                {
                  save = true;
                }
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
              trace::printf ("%s() old %s %d %d\n", __func__,
                             old_thread->name (), old_thread->sched_state_,
                             save);
#endif

              if (old_thread->sched_state_ == rtos::thread::state::running)
                {
                  old_thread->sched_state_ = rtos::thread::state::waiting;

                  waiting_thread_node& crt_node = old_thread->ready_node_;
                  if (crt_node.next == nullptr)
                    {
                      rtos::scheduler::ready_threads_list_.link (crt_node);
                      // Ready state set in above link().
                    }
                }

              old_ctx =
                  reinterpret_cast<ucontext_t*> (&old_thread->context_.port_.ucontext);
              // Select the top priority thread
              rtos::scheduler::current_thread_ =
                  rtos::scheduler::ready_threads_list_.unlink_head ();
              new_ctx =
                  reinterpret_cast<ucontext_t*> (&rtos::scheduler::current_thread_->context_.port_.ucontext);
            }

          if (old_ctx != new_ctx)
            {
              if (save)
                {
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
                  trace::printf ("%s() swapcontext %s -> %s \n", __func__,
                                 old_thread->name (),
                                 rtos::scheduler::current_thread_->name ());
#endif
                  if (swapcontext (old_ctx, new_ctx) != 0)
                    {
                      trace::printf ("%s() swapcontext failed with %s\n",
                                     __func__, strerror (errno));
                      abort ();
                    }
                }
              else
                {
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
                  trace::printf ("%s() setcontext %s\n", __func__,
                                 rtos::scheduler::current_thread_->name ());
#endif
                  // context->port_.saved = false;
                  if (setcontext (new_ctx) != 0)
                    {
                      trace::printf ("%s() setcontext failed with %s\n",
                                     __func__, strerror (errno));
                      abort ();
                    }
                }
            }
          else
            {
#if defined(OS_TRACE_RTOS_THREAD_CONTEXT)
              trace::printf ("%s() nop %s\n", __func__, old_thread->name ());
#endif
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
      clock_systick::start (void)
      {
        // set handler
        struct sigaction sa;
#if defined(__APPLE__)
        sa.__sigaction_u.__sa_handler = systick_clock_signal_handler;
#elif defined(__linux__)
#pragma GCC diagnostic push
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
        sa.sa_handler = systick_clock_signal_handler;
#pragma GCC diagnostic pop
#else
#error Platform unsupported
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
#if defined(__APPLE__)
        memset (&tv, 0, sizeof(tv));
#else
        timerclear(&tv.it_value);
#endif
        // then set the required ones

#if 1
        tv.it_value.tv_sec = 0;
        tv.it_value.tv_usec = 1000000 / rtos::clock_systick::frequency_hz;
        tv.it_interval.tv_sec = 0;
        tv.it_interval.tv_usec = 1000000 / rtos::clock_systick::frequency_hz;
#else
        tv.it_value.tv_sec = 1;
        tv.it_value.tv_usec = 0; //1000000 / rtos::clock_systick::frequency_hz;
        tv.it_interval.tv_sec = 1;
        tv.it_interval.tv_usec = 0;//1000000 / rtos::clock_systick::frequency_hz;
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

// ----------------------------------------------------------------------------

#endif /* defined(__APPLE__) || defined(__linux__) */
