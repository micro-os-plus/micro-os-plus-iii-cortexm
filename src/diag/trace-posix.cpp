/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2015 Liviu Ionescu.
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

#if defined(TRACE)

#include <cmsis-plus/os-app-config.h>
#include <cmsis-plus/diag/trace.h>
#include <cstddef>
#include <unistd.h>

// ----------------------------------------------------------------------------

namespace os
{
  namespace trace
  {
    // ------------------------------------------------------------------------

    void
    initialize (void)
    {
      ; // For POSIX no inits are required.
    }

    // ------------------------------------------------------------------------

    ssize_t
    write (const void* buf, std::size_t nbyte)
    {
#if defined(OS_USE_TRACE_POSIX_STDOUT)
      return ::write (1, buf, nbyte); // Forward to STDOUT.
#elif defined(OS_USE_TRACE_POSIX_STDERR)
      return ::write (2, buf, nbyte); // Forward to STDERR.
#else
#warning "No trace output channel."
      buf = buf;
      return nbyte;
#endif
    }

  } /* namespace trace */
} /* namespace os */

#endif /* defined(TRACE) */

// ----------------------------------------------------------------------------

#endif /* defined(__APPLE__) || defined(__linux__) */
