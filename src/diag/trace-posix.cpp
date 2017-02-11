/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2015 Liviu Ionescu.
 *
 * µOS++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 3.
 *
 * µOS++ is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
