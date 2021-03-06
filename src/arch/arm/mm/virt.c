
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#include <stddef.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <core/debug/debug.h>
#include <core/entry.h>
#include <core/panic.h>
#include <core/mm/phys.h>
#include <core/mm/virt.h>
#include <arch/arm/mm/virt.h>

/**
 * @brief Supported modes
 */
uint32_t supported_modes;

/**
 * @brief user context
 */
virt_context_ptr_t user_context;

/**
 * @brief kernel context
 */
virt_context_ptr_t kernel_context;

/**
 * @brief Method to get supported modes
 */
void virt_setup_supported_modes( void ) {
  #if defined( ELF32 )
    // variable for return
    uint32_t reg = 0;

    // get paging support from mmfr0
    __asm__ __volatile__(
      "mrc p15, 0, %0, c0, c1, 4"
      : "=r" ( reg )
      : : "cc"
    );

    // save supported modes
    supported_modes = reg & 0xF;

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "reg = %#08x, supported_modes = %#08x\r\n",
        reg, supported_modes );
    #endif

    // get memory size from mmfr3
    __asm__ __volatile__(
      "mrc p15, 0, %0, c0, c1, 7"
      : "=r" ( reg )
      : : "cc"
    );

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "reg = %#08x\r\n", reg );
    #endif

    // get only cpu address bus size
    reg = ( reg >> 24 ) & 0xf;

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "reg = %#08x\r\n", reg );
    #endif

    // set paging to v7 short descriptor if more
    // than 32 bit physical addresses arent supported
    if ( 0 == reg && ID_MMFR0_VSMA_V7_PAGING_LPAE & supported_modes ) {
      if ( ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS & supported_modes ) {
        supported_modes = ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS;
      } else if ( ID_MMFR0_VSMA_V7_PAGING_PXN & supported_modes ) {
        supported_modes = ID_MMFR0_VSMA_V7_PAGING_PXN;
      } else {
        PANIC( "No MMU supported!" );
      }
    }
  #elif defined( ELF64 )
    #error "Unsupported"
  #endif
}

/**
 * @brief Method to prepare virtual memory management by architecture
 */
void virt_arch_init( void ) {
  // setup supported mode global
  virt_setup_supported_modes();

  // prepare
  virt_arch_prepare();

  // create a kernel context
  kernel_context = virt_create_context( VIRT_CONTEXT_TYPE_KERNEL );
  assert( NULL != kernel_context );

  // create a dummy user context for all cores
  user_context = virt_create_context( VIRT_CONTEXT_TYPE_USER );
  assert( NULL != user_context );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "kernel_context: %p\r\n", ( void* )kernel_context );
    DEBUG_OUTPUT( "user_context: %p\r\n", ( void* )user_context );
  #endif
}

/**
 * @brief Method checks whether address is mapped or not without generating exceptions
 *
 * @param addr
 * @return true
 * @return false
 */
bool virt_is_mapped( uintptr_t addr ) {
  return
    virt_is_mapped_in_context( kernel_context, addr )
    || virt_is_mapped_in_context( user_context, addr );
}
