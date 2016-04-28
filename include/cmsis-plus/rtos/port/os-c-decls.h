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
 * The initial CMSIS++ RTOS API was inspired by CMSIS RTOS API v1.x,
 * Copyright (c) 2013 ARM LIMITED
 */

/*
 * This file is part of the CMSIS++ proposal, intended as a CMSIS
 * replacement for C++ applications.
 *
 * It is included in `cmsis-plus/rtos/os-c-decls.h` to customise
 * it with port specific declarations.
 *
 * These structures (which basically contain handlers)
 * are conditionally included in the system objects
 * when they are implemented using the port native objects.
 */

#ifndef CMSIS_PLUS_RTOS_PORT_OS_C_DECLS_H_
#define CMSIS_PLUS_RTOS_PORT_OS_C_DECLS_H_

#if defined(__APPLE__)
// mainly for ucontext.h, but seems it is needed in other system
// headers, since without it the sleep test crashes
#define _XOPEN_SOURCE 600L
#endif

#include <ucontext.h>

#include <stdbool.h>

#if defined(__APPLE__)
  typedef struct
  {
    int uc_onstack;
    __darwin_sigset_t uc_sigmask; /* signal mask used by this context */
    _STRUCT_SIGALTSTACK uc_stack; /* stack used by this context */
    _STRUCT_UCONTEXT *uc_link; /* pointer to resuming context */
    __darwin_size_t uc_mcsize; /* size of the machine context passed in */
    _STRUCT_MCONTEXT *uc_mcontext; /* pointer to machine specific context */
    _STRUCT_MCONTEXT __mcontext_data;
  } _ucontext_t;

#endif


typedef struct
{
  // bool saved;
#if defined(__APPLE__)
  _ucontext_t ucontext;
#else
  ucontext_t ucontext; //
#endif
} os_port_thread_context_t;

#endif /* CMSIS_PLUS_RTOS_PORT_OS_C_DECLS_H_ */
