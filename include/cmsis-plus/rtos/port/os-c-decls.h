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

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t os_port_clock_timestamp_t;
typedef uint32_t os_port_clock_duration_t;
typedef uint64_t os_port_clock_offset_t;

typedef bool os_port_scheduler_state_t;

typedef uint32_t os_port_irq_state_t;

typedef uint32_t os_port_thread_stack_element_t;
typedef uint64_t os_port_thread_stack_allocation_element_t;

typedef struct
{
  os_port_thread_stack_element_t* stack_ptr;
} os_port_thread_context_t;

#define OS_INTEGER_RTOS_STACK_FILL_MAGIC (0xEFBEADDE)

#define OS_HAS_INTERRUPTS_STACK

#endif /* CMSIS_PLUS_RTOS_PORT_OS_C_DECLS_H_ */
