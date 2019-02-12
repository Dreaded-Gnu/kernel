
/**
 * Copyright (C) 2017 - 2019 bolthur project.
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
#include <stdbool.h>

#include "lib/stdc/stdio.h"
#include "kernel/kernel/panic.h"
#include "kernel/kernel/entry.h"
#include "kernel/kernel/mm/phys.h"

/**
 * @brief Physical bitmap
 */
uintptr_t *phys_bitmap;

/**
 * @brief physical bitmap length set by vendor
 */
size_t phys_bitmap_length;

/**
 * @brief placement address starting at kernel end
 */
uintptr_t placement_address = VIRT_2_PHYS( &__kernel_end );

/**
 * @brief static initialized flag
 */
static bool phys_initialized = false;

/**
 * @brief Mark physical page as used on arm
 *
 * @param address address to mark as free
 */
void phys_mark_page_used( void* address ) {
  // get frame, index and offset
  size_t frame = ( uintptr_t )address / PAGE_SIZE;
  size_t index = PAGE_INDEX( frame );
  size_t offset = PAGE_OFFSET( frame );

  // mark page as used
  phys_bitmap[ index ] |= ( uintptr_t )( 0x1 << offset );

  // debug output
  #if defined( PRINT_MM_PHYS )
    printf(
      "[ phys mark used ]: frame: %06i, index: %04i, offset: %02i, address: 0x%08x, phys_bitmap[ %04d ]: 0x%08x\r\n",
      frame, index, offset, address, index, phys_bitmap[ index ]
    );
  #endif
}

/**
 * @brief Mark physical page as free on arm
 *
 * @param address address to mark as free
 */
void phys_mark_page_free( void*  address ) {
  // get frame, index and offset
  size_t frame = ( uintptr_t )address / PAGE_SIZE;
  size_t index = PAGE_INDEX( frame );
  size_t offset = PAGE_OFFSET( frame );

  // mark page as free
  phys_bitmap[ index ] &= ( uintptr_t )( ~( 0x1 << offset ) );

  // debug output
  #if defined( PRINT_MM_PHYS )
    printf(
      "[ phys mark used ]: frame: %06i, index: %04i, offset: %02i, address: 0x%08x, phys_bitmap[ %04d ]: 0x%08x\r\n",
      frame, index, offset, address, index, phys_bitmap[ index ]
    );
  #endif
}

/**
 * @brief Method to free phys page range
 *
 * @param address start address
 * @param amount amount of memory
 */
void phys_free_page_range( void* address, size_t amount ) {
  // debug output
  #if defined( PRINT_MM_PHYS )
    printf(
      "[ phys free page range ]: address: 0x%08x, amount: %i\r\n",
      address, amount
    );
  #endif

  // loop until amount and mark as free
  for (
    size_t idx = 0;
    idx < amount / PAGE_SIZE;
    idx++, address = ( void* )( ( uintptr_t ) address + PAGE_SIZE )
  ) {
    phys_mark_page_free( address );
  }
}

/**
 * @brief Method to free phys page range
 *
 * @param address start address
 * @param amount amount of memory
 */
void phys_use_page_range( void* address, size_t amount ) {
  // debug output
  #if defined( PRINT_MM_PHYS )
    printf(
      "[ phys use page range ]: address: 0x%08x, amount: %i\r\n",
      address, amount
    );
  #endif

  // round down address to page start
  address = ( void* )( ( uintptr_t )address - ( uintptr_t )address % PAGE_SIZE );

  // loop until amount and mark as free
  for (
    size_t idx = 0;
    idx < amount / PAGE_SIZE;
    idx++, address = ( void* )( ( uintptr_t ) address + PAGE_SIZE )
  ) {
    phys_mark_page_used( address );
  }
}

/**
 * @brief Method to find free page range
 *
 * @param memory_amount amount of memory to find free page range for
 * @return uintptr_t address of found memory
 */
void* phys_find_free_page_range( size_t memory_amount, size_t alignment ) {
  // debug output
  #if defined( PRINT_MM_PHYS )
    printf(
      "[ phys find free page range ]: memory_amount: %i, allignment: 0x%08x\r\n",
      memory_amount, alignment
    );
  #endif

  // round up to full page
  if ( 0 < memory_amount % PAGE_SIZE ) {
    memory_amount += PAGE_SIZE - ( memory_amount % PAGE_SIZE );
  }

  // determine amount of pages
  size_t page_amount = memory_amount / PAGE_SIZE;
  size_t found_amount = 0;

  // found address range
  void* address = NULL;
  void* tmp = NULL;
  bool stop = false;

  // loop through bitmap to find free continouse space
  for ( size_t idx = 0; idx < phys_bitmap_length && !stop; idx++ ) {
    // skip completely used entries
    if ( PHYS_ALL_PAGES_OF_INDEX_USED == phys_bitmap[ idx ] ) {
      continue;
    }

    // loop through bits per entry
    for ( size_t offset = 0; offset < PAGE_PER_ENTRY && !stop; offset++ ) {
      // not free? => reset counter and continue
      if ( phys_bitmap[ idx ] & ( uintptr_t )( 0x1 << offset ) ) {
        found_amount = 0;
        address = NULL;
        continue;
      }

      // set address if found is 0
      if ( 0 == found_amount ) {
        address = ( void* )(
          idx * PAGE_SIZE * PAGE_PER_ENTRY + offset * PAGE_SIZE
        );

        // check for alignment
        if (
          0 < alignment
          && 0 != ( uintptr_t )address % alignment
        ) {
          found_amount = 0;
          address = NULL;
          continue;
        }
      }

      // increase found amount
      found_amount += 1;

      // reached necessary amount? => stop loop
      if ( found_amount == page_amount ) {
        stop = true;
      }
    }
  }

  // assert found address
  ASSERT( NULL != address );

  // set temporary address
  tmp = address;

  // loop until amount and mark as used
  for (
    size_t idx = 0;
    idx < found_amount;
    idx++, tmp = ( void* )( ( uintptr_t ) tmp + PAGE_SIZE )
  ) {
    phys_mark_page_used( tmp );
  }

  // return found / not found address
  return address;
}

/**
 * @brief Shorthand to find single free page
 *
 * @param alignment
 * @return void*
 */
void* phys_find_free_page( size_t alignment ) {
  return phys_find_free_page_range( PAGE_SIZE, alignment );
}

/**
 * @brief Shorthand for free one single page
 *
 * @param address address to free
 */
void phys_free_page( void* address ) {
  phys_free_page_range( address, PAGE_SIZE );
}

/**
 * @brief Generic initialization of physical memory manager
 */
void phys_init( void ) {
  // execute vendor initialization
  phys_vendor_init();

  // determine start and end for kernel mapping
  uintptr_t start = 0;
  uintptr_t end = placement_address + placement_address % PAGE_SIZE;

  // adjust placement address
  placement_address = end;

  // map from start to end addresses as used
  while( start < end ) {
    // mark used
    phys_mark_page_used( ( void* )start );

    // get next page
    start += PAGE_SIZE;
  }

  // mark initialized
  phys_initialized = true;
}

/**
 * @brief Get initialized flag
 *
 * @return true physical memory management has been set up
 * @return false physical memory management has been not yet set up
 */
bool phys_initialized_get( void ) {
  return phys_initialized;
}
