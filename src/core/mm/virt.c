
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
#include <stdbool.h>

#include <assert.h>
#include <core/debug/debug.h>
#include <core/panic.h>
#include <core/entry.h>
#include <core/initrd.h>
#include <core/firmware.h>
#include <core/mm/phys.h>
#include <core/mm/virt.h>

/**
 * @brief static initialized flag
 */
static bool virt_initialized = false;

/**
 * @brief Generic initialization of virtual memory manager
 */
void virt_init( void ) {
  // assert no initialize
  assert( true != virt_initialized );

  // set global context to null
  kernel_context = NULL;
  user_context = NULL;

  // architecture related initialization
  virt_arch_init();

  // determine start and end for kernel mapping
  uintptr_t start = 0;
  uintptr_t end = VIRT_2_PHYS( &__kernel_end );
  // round up to page size if necessary
  ROUND_UP_TO_FULL_PAGE( end )

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "Map kernel space %p - %p to %p - %p \r\n",
      ( void* )start,
      ( void* )end,
      ( void* )PHYS_2_VIRT( start ),
      ( void* )PHYS_2_VIRT( end )
    );
  #endif

  // map from start to end addresses as used
  while ( start < end ) {
    // map page
    virt_map_address(
      kernel_context,
      PHYS_2_VIRT( start ),
      start,
      VIRT_MEMORY_TYPE_NORMAL,
      VIRT_PAGE_TYPE_EXECUTABLE
    );

    // get next page
    start += PAGE_SIZE;
  }

  // consider possible initrd
  if ( initrd_exist() ) {
    // set start and end from initrd
    start = initrd_get_start_address();
    end = initrd_get_end_address();

    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "Map initrd space %p - %p to %p - %p \r\n",
        ( void* )start,
        ( void* )end,
        ( void* )PHYS_2_VIRT( start ),
        ( void* )PHYS_2_VIRT( end )
      );
    #endif

    // map from start to end addresses as used
    while ( start < end ) {
      // map page
      virt_map_address(
        kernel_context,
        PHYS_2_VIRT( start ),
        start,
        VIRT_MEMORY_TYPE_NORMAL,
        VIRT_PAGE_TYPE_AUTO
      );

      // get next page
      start += PAGE_SIZE;
    }

    // change initrd location
    initrd_set_start_address(
      PHYS_2_VIRT( initrd_get_start_address() )
    );
  }

  // initialize platform related
  virt_platform_init();
  // prepare temporary area
  virt_prepare_temporary( kernel_context );

  // firmware init stuff
  firmware_init();

  // set kernel context
  virt_set_context( kernel_context );
  // flush contexts to take effect
  virt_flush_complete();

  // set dummy user context
  virt_set_context( user_context );
  // flush contexts to take effect
  virt_flush_complete();

  // post init
  virt_platform_post_init();

  // set static
  virt_initialized = true;
}

/**
 * @brief Get initialized flag
 *
 * @return true virtual memory management has been set up
 * @return false virtual memory management has been not yet set up
 */
bool virt_init_get( void ) {
  return virt_initialized;
}

/**
 * @brief Method to check for range is mapped in context
 *
 * @param ctx context to use
 * @param address start address of range
 * @param size range size
 * @return true range is completely mapped
 * @return false range is not or incompletely mapped
 */
bool virt_is_mapped_in_context_range(
  virt_context_ptr_t ctx,
  uintptr_t address,
  size_t size
) {
  uintptr_t start = address;
  uintptr_t end = start + size;
  // loop until end
  while ( start < end ) {
    // return false if not mapped
    if ( ! virt_is_mapped_in_context( ctx, start ) ) {
      return false;
    }
    // get next page
    start  += PAGE_SIZE;
  }
  // return success
  return true;
}

/**
 * @brief Method to check for range is mapped
 *
 * @param address start address of range
 * @param size range size
 * @return true range is completely mapped
 * @return false range is not or incompletely mapped
 */
bool virt_is_mapped_range( uintptr_t address, size_t size ) {
  uintptr_t start = address;
  uintptr_t end = start + size;
  // loop until end
  while ( start < end ) {
    // return false if not mapped
    if ( ! virt_is_mapped( start ) ) {
      return false;
    }
    // get next page
    start  += PAGE_SIZE;
  }
  // return success
  return true;
}

/**
 * @brief Method to unmap address range
 *
 * @param context ctx to perform unmap in
 * @param address start address of range
 * @param size range size
 * @param free_phys free physical memory
 */
void virt_unmap_address_range(
  virt_context_ptr_t ctx,
  uintptr_t address,
  size_t size,
  bool free_phys
) {
  uintptr_t start = address;
  uintptr_t end = start + size;
  // loop until end
  while ( start < end ) {
    // unmap virtual
    virt_unmap_address( ctx, address, free_phys );
    // get next page
    start  += PAGE_SIZE;
  }
}

/**
 * @brief Find free page range within context
 *
 * @param context ctx to use for lookup
 * @param size necessary amount
 * @return uintptr_t
 */
uintptr_t virt_find_free_page_range( virt_context_ptr_t ctx, size_t size ) {
  // get min and max by context
  uintptr_t min = virt_get_context_min_address( ctx );
  uintptr_t max = virt_get_context_max_address( ctx );

  // round up to full page
  ROUND_UP_TO_FULL_PAGE( size )

  // determine amount of pages
  size_t page_amount = size / PAGE_SIZE;
  size_t found_amount = 0;

  // found address range
  uintptr_t address = 0;
  bool stop = false;

  while ( min <= max && !stop ) {
    // skip if mapped
    if ( virt_is_mapped_in_context( ctx, min ) ) {
      found_amount = 0;
      address = 0;
      continue;
    }
    // set address if we start
    if ( 0 == found_amount ) {
      address = min;
    }
    // increase found amount
    found_amount += 1;
    // handle necessary amount reached
    if ( found_amount == page_amount ) {
      stop = true;
    }
    // next page
    min += PAGE_SIZE;
  }

  // assert address
  assert( 0 != address );

  // return found address or null
  return address;
}

/**
 * @brief Map physical address range to virtual address range
 *
 * @param ctx context
 * @param address start address
 * @param size size
 * @param type memory type
 * @param page page attributes
 */
void virt_map_address_range(
  virt_context_ptr_t ctx,
  uintptr_t address,
  uint64_t phys,
  size_t size,
  virt_memory_type_t type,
  uint32_t page
) {
  // mark range as used
  phys_use_page_range( phys, size );
  // determine end
  uintptr_t end = address + size;
  // loop and map
  while ( address < end ) {
    virt_map_address( ctx, address, phys, type, page );
    // next page
    address += PAGE_SIZE;
    phys += PAGE_SIZE;
  }
}

/**
 * @brief Map random physical pages to virtual range
 *
 * @param ctx context
 * @param address start address
 * @param size size
 * @param type memory type
 * @param page page attributes
 */
void virt_map_address_range_random(
  virt_context_ptr_t ctx,
  uintptr_t address,
  size_t size,
  virt_memory_type_t type,
  uint32_t page
) {
  // determine end
  uintptr_t end = address + size;
  // loop and map
  while ( address < end ) {
    // get physical page
    uint64_t phys = phys_find_free_page( PAGE_SIZE );
    // map address
    virt_map_address( ctx, address, phys, type, page );
    // next page
    address += PAGE_SIZE;
  }
}

/**
 * @brief Get context min address
 *
 * @param ctx
 * @return uintptr_t
 */
uintptr_t virt_get_context_min_address( virt_context_ptr_t ctx ) {
  if ( ctx->type == VIRT_CONTEXT_TYPE_KERNEL ) {
    return KERNEL_AREA_START;
  } else if ( ctx->type == VIRT_CONTEXT_TYPE_USER ) {
    return USER_AREA_START;
  } else {
    PANIC( "Unsupported context type!" );
  }
}

/**
 * @brief Get context max address
 *
 * @param ctx
 * @return uintptr_t
 */
uintptr_t virt_get_context_max_address( virt_context_ptr_t ctx ) {
  if ( ctx->type == VIRT_CONTEXT_TYPE_KERNEL ) {
    return KERNEL_AREA_END;
  } else if ( ctx->type == VIRT_CONTEXT_TYPE_USER ) {
    return USER_AREA_END;
  } else {
    PANIC( "Unsupported context type!" );
  }
}
