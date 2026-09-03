/**
 * Copyright (C) 2018 - 2026 bolthur project.
 *
 * This file is part of bolthur/kernel.
 *
 * bolthur/kernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bolthur/kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bolthur/kernel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../entry.h"
#include "../lib/assert.h"
#include "../mm/phys.h"
#include "../mm/virt.h"
#include "pool.h"

static uintptr_t rpc_pool_start;
static uintptr_t rpc_pool_size;

/**
 * @fn void rpc_pool_setup( void )
 * @brief Function to setup rpc pool
 */
void rpc_pool_setup( void ) {
  // rpc pool starts with one page => ~64 possible rpc
  rpc_pool_start = KERNEL_RPC_POOL_START;
  rpc_pool_size = PAGE_SIZE;
  // map rpc pool
  for ( uintptr_t addr = rpc_pool_start; addr < rpc_pool_start + rpc_pool_size; addr += PAGE_SIZE ) {
    assert( virt_map_address_random(
      virt_current_kernel_context,
      addr,
      VIRT_MEMORY_TYPE_NORMAL_NC,
      VIRT_PAGE_TYPE_READ | VIRT_PAGE_TYPE_WRITE
    ) );
  }
}
