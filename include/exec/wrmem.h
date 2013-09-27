/*
 * defines wrmem related functions
 *
 *  Copyright (c) 2003 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/**************************************************************************
 * IO ports API
 */

#ifndef WRMEM_H
#define WRMEM_H

#include "qemu-common.h"
#include "exec/memory.h"

#define MAX_WRMEMS     (64 * 1024)
#define WRMEMS_MASK    (MAX_WRMEMS - 1)

/* These should really be in isa.h, but are here to make pc.h happy.  */
typedef void (WrmemWriteFunc)(void *opaque, uint32_t address, uint32_t data);
typedef uint32_t (WrmemReadFunc)(void *opaque, uint32_t address);
typedef void (WrmemDestructor)(void *opaque);

void writedbg_mem(uint32_t addr, uint32_t val);
uint32_t readdbg_mem(uint32_t addr);

#endif /* WRMEM_H */
