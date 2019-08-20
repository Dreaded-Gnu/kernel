
/**
 * Copyright (C) 2018 - 2019 bolthur project.
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
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <kernel/panic.h>
#include <kernel/entry.h>
#include <kernel/debug.h>
#include <arch/arm/barrier.h>
#include <kernel/mm/phys.h>
#include <arch/arm/mm/virt/long.h>
#include <arch/arm/v7/mm/virt/long.h>
#include <kernel/mm/virt.h>

/**
 * @brief Temporary space start for long descriptor format
 */
#define TEMPORARY_SPACE_START 0xF1000000

/**
 * @brief Temporary space size for long descriptor format
 */
#define TEMPORARY_SPACE_SIZE 0xFFFFFF

/**
 * @brief Amount of mapped temporary tables
 */
static uint32_t mapped_temporary_tables = 0;

/**
 * @brief Map physical space to temporary
 *
 * @param start physical start address
 * @param size size to map
 * @return uintptr_t mapped address
 *
 * @todo add disable of cache
 */
static uintptr_t map_temporary( uintptr_t start, size_t size ) {
  // find free space to map
  uint32_t page_amount = ( uint32_t )( size / PAGE_SIZE );
  uint32_t found_amount = 0;
  uintptr_t start_address = 0;
  bool stop = false;

  // stop here if not initialized
  if ( true != virt_initialized_get() ) {
    return start;
  }

  // determine offset and subtract start
  uint32_t offset = start % PAGE_SIZE;
  start -= offset;
  uint32_t current_table = 0;
  uint32_t table_idx_offset = LD_VIRTUAL_TABLE_INDEX( TEMPORARY_SPACE_START );

  // minimum: 1 page
  if ( 1 > page_amount ) {
    page_amount++;
  }

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "start = 0x%08x, page_amount = %d, offset = 0x%08x\r\n",
      start, page_amount, offset
    );
  #endif

  // Find free area
  for (
    uintptr_t table = TEMPORARY_SPACE_START;
    table < TEMPORARY_SPACE_START + ( PAGE_SIZE * mapped_temporary_tables ) && !stop;
    table += PAGE_SIZE, ++current_table
  ) {
    // get table
    ld_page_table_t* p = ( ld_page_table_t* )table;

    for ( uint32_t idx = 0; idx < 512; idx++ ) {
      // Not free, reset
      if ( 0 != p->page[ idx ].raw ) {
        found_amount = 0;
        start_address = 0;
        continue;
      }

      // set address if found is 0
      if ( 0 == found_amount ) {
        start_address = TEMPORARY_SPACE_START + (
            current_table * PAGE_SIZE * 512
          ) + ( PAGE_SIZE * idx );
      }

      // increase found amount
      found_amount += 1;

      // reached necessary amount? => stop loop
      if ( found_amount == page_amount ) {
        stop = true;
        break;
      }
    }
  }

  // assert found address
  assert( 0 != start_address );

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "Found virtual address 0x%08x\r\n", start_address );
  #endif

  // map amount of pages
  for ( uint32_t i = 0; i < page_amount; ++i ) {
    uintptr_t addr = start_address + i * PAGE_SIZE;

    // map it
    uint32_t page_idx = LD_VIRTUAL_PAGE_INDEX( addr );
    uint32_t table_idx = LD_VIRTUAL_TABLE_INDEX( addr ) - table_idx_offset;

    // debug putput
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "addr = 0x%08x, table_idx_offset = %d, table_idx = %d, page_idx = %d\r\n",
        addr, table_idx_offset, table_idx, page_idx
      );
    #endif

    // get table
    ld_page_table_t* tbl = ( ld_page_table_t* )(
      TEMPORARY_SPACE_START + table_idx * PAGE_SIZE
    );

    // debug putput
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "tbl = 0x%08x\r\n", tbl );
    #endif

    // handle it non cachable
    // set page
    tbl->page[ page_idx ].raw = start & 0xFFFFF000;

    // set attributes
    tbl->page[ page_idx ].data.type = 3;
    tbl->page[ page_idx ].data.lower_attr_access = 1;

    // increase physical address
    start += i * PAGE_SIZE;
  }

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "ret = 0x%08x\r\n", start_address + offset );
  #endif

  // flush context if running
  if ( virt_initialized_get() ) {
    virt_flush_context();
  }

  // return address with offset
  return start_address + offset;
}

/**
 * @brief Helper to unmap temporary
 *
 * @param addr address to unmap
 * @param size size
 */
static void unmap_temporary( uintptr_t addr, size_t size ) {
  // determine offset and subtract start
  uint32_t page_amount = ( uint32_t )( size / PAGE_SIZE );
  size_t offset = addr % PAGE_SIZE;
  addr = addr - offset;

  // stop here if not initialized
  if ( true != virt_initialized_get() ) {
    return;
  }

  if ( 1 > page_amount ) {
    ++page_amount;
  }

  // determine table index offset
  uint32_t table_idx_offset = LD_VIRTUAL_TABLE_INDEX( TEMPORARY_SPACE_START );

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "page_amount = %d - table_idx_offset = %d\r\n",
      page_amount,
      table_idx_offset
    );
  #endif

  // calculate end
  uintptr_t end = addr + page_amount * PAGE_SIZE;

  // debug putput
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "end = 0x%08x\r\n", end );
  #endif

  // loop and unmap
  while ( addr < end ) {
    uint32_t table_idx = LD_VIRTUAL_TABLE_INDEX( addr ) - table_idx_offset;
    uint32_t page_idx = LD_VIRTUAL_PAGE_INDEX( addr );

    // get table
    ld_page_table_t* tbl = ( ld_page_table_t* )(
      TEMPORARY_SPACE_START + table_idx * PAGE_SIZE
    );

    // debug putput
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT( "tbl = 0x%08x\r\n", tbl );
    #endif

    // unmap
    tbl->page[ page_idx ].raw = 0;

    // next page size
    addr += PAGE_SIZE;
  }

  // flush context if running
  if ( virt_initialized_get() ) {
    virt_flush_context();
  }
}

/**
 * @brief Get the new table object
 *
 * @return uintptr_t address to new table
 */
static uintptr_t get_new_table() {
  // get new page
  uintptr_t addr = phys_find_free_page( PAGE_SIZE );
  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "addr = 0x%08x\r\n", addr );
  #endif

  // map temporarily
  uintptr_t tmp = map_temporary( addr, PAGE_SIZE );
  // overwrite page with zero
  memset( ( void* )tmp, 0, PAGE_SIZE );
  // unmap page again
  unmap_temporary( tmp, PAGE_SIZE );

  // return address
  return addr;
}

/**
 * @brief Internal v7 long descriptor create table function
 *
 * @param ctx context to create table for
 * @param addr address the table is necessary for
 * @param table page table address
 * @return uintptr_t address of created and prepared table
 */
uintptr_t v7_long_create_table(
  virt_context_ptr_t ctx,
  uintptr_t addr,
  __unused uintptr_t table
) {
  // get table idx
  uint32_t pmd_idx = LD_VIRTUAL_PMD_INDEX( addr );
  uint32_t tbl_idx = LD_VIRTUAL_TABLE_INDEX( addr );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "create long descriptor table for address 0x%08x\r\n", addr );
    DEBUG_OUTPUT( "pmd_idx = %d, tbl_idx = %d\r\n", pmd_idx, tbl_idx );
  #endif

  // get context
  ld_global_page_directory_t* context = ( ld_global_page_directory_t* )
    map_temporary( ctx->context, PAGE_SIZE );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "context = 0x%08x\r\n", context );
  #endif

  // get pmd table from pmd
  ld_context_table_level1_t* pmd_tbl = &context->table[ pmd_idx ];
  // create it if not yet created
  if ( 0 == pmd_tbl->raw ) {
    // populate level 1 table
    pmd_tbl->raw = get_new_table() & 0xFFFFF000;
    // set attribute
    pmd_tbl->data.attr_ns_table = ( uint8_t )(
      ctx->type == CONTEXT_TYPE_USER ? 1 : 0
    );
    pmd_tbl->data.type = 3;
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "pmd_tbl->raw = 0x%08x%08x\r\n",
        ( uint32_t )( ( pmd_tbl->raw >> 32 ) & 0xFFFFFFFF ),
        ( uint32_t )pmd_tbl->raw & 0xFFFFFFFF
      );
    #endif
  }

  // page middle directory
  ld_middle_page_directory* pmd = ( ld_middle_page_directory* )
    map_temporary( ( ( uint32_t )pmd_tbl->raw & 0xFFFFF000 ), PAGE_SIZE );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "pmd_tbl = 0x%08x, pmd = 0x%08x\r\n", pmd_tbl, pmd );
  #endif

  // get page table
  ld_context_table_level2_t* tbl_tbl = &pmd->table[ tbl_idx ];
  // create if not yet created
  if ( 0 == tbl_tbl->raw ) {
    // populate level 2 table
    tbl_tbl->raw = get_new_table() & 0xFFFFF000;
    // set attributes
    tbl_tbl->data.type = 3;
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "0x%08x%08x\r\n",
        ( uint32_t )( ( tbl_tbl->raw >> 32 ) & 0xFFFFFFFF ),
        ( uint32_t )tbl_tbl->raw & 0xFFFFFFFF
      );
    #endif
  }

  // page directory
  ld_page_table_t* tbl = ( ld_page_table_t* )(
    ( uint32_t )tbl_tbl->raw & 0xFFFFF000
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "tbl_tbl = 0x%08x, tbl = 0x%08x\r\n", tbl_tbl, tbl );
  #endif

  // unmap temporary
  unmap_temporary( ( uintptr_t )context, PAGE_SIZE );
  unmap_temporary( ( uintptr_t )pmd, PAGE_SIZE );

  // return table
  return ( uintptr_t )tbl;
}

/**
 * @brief Internal v7 long descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param paddr pointer to physical address
 * @param flag mapping flags
 *
 * @todo consider flag
 */
void v7_long_map(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uintptr_t paddr,
  __unused uint32_t flag
) {
  // determine page index
  uint32_t page_idx = LD_VIRTUAL_PAGE_INDEX( vaddr );

  // get table with creation if necessary
  ld_page_table_t* table = ( ld_page_table_t* )v7_long_create_table(
    ctx, vaddr, 0
  );

  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: 0x%08x\r\n", table );
  #endif

  // map temporary
  table = ( ld_page_table_t* )map_temporary( ( uintptr_t )table, PAGE_SIZE );

  // assert existance
  assert( NULL != table );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "table: 0x%08x\r\n", table );
    DEBUG_OUTPUT(
      "table->page[ %d ] = 0x%08x\r\n",
      page_idx,
      table->page[ page_idx ]
    );
  #endif

  // ensure not already mapped
  assert( 0 == table->page[ page_idx ].raw );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "page physical address = 0x%08x\r\n", paddr );
  #endif

  // set page
  table->page[ page_idx ].raw = paddr & 0xFFFFF000;

  // set attributes
  table->page[ page_idx ].data.lower_attr_non_secure = ( uint8_t )(
    ( ctx->type == CONTEXT_TYPE_USER ? 1 : 0 )
  );
  table->page[ page_idx ].data.type = 3;
  table->page[ page_idx ].data.lower_attr_access = 1;
  table->page[ page_idx ].data.lower_attr_access_permission = ( uint8_t )(
    ( ctx->type == CONTEXT_TYPE_USER ? 1 : 0 )
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT(
      "table->page[ %d ].data.raw = 0x%08x%08x\r\n",
      page_idx,
      ( uint32_t )( ( table->page[ page_idx ].raw >> 32 ) & 0xFFFFFFFF ),
      ( uint32_t )table->page[ page_idx ].raw & 0xFFFFFFFF
    );
  #endif

  // unmap temporary
  unmap_temporary( ( uintptr_t )table, PAGE_SIZE );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "%s\r\n\r\n", "flush context" );
  #endif

  // flush context if running
  if ( virt_initialized_get() ) {
    virt_flush_context();
  }
}

/**
 * @brief Internal v7 long descriptor mapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 * @param flag mapping flags
 */
void v7_long_map_random(
  virt_context_ptr_t ctx,
  uintptr_t vaddr,
  uint32_t flag
) {
  // get physical address
  uintptr_t phys = phys_find_free_page( PAGE_SIZE );
  // assert
  assert( 0 != phys );
  // map it
  v7_long_map( ctx, vaddr, phys, flag );
}

/**
 * @brief Internal v7 long descriptor unmapping function
 *
 * @param ctx pointer to page context
 * @param vaddr pointer to virtual address
 */
void v7_long_unmap( virt_context_ptr_t ctx, uintptr_t vaddr ) {
  // get page index
  uint32_t page_idx = LD_VIRTUAL_PAGE_INDEX( vaddr );

  // get table for unmapping
  ld_page_table_t* table = ( ld_page_table_t* )v7_long_create_table(
    ctx, vaddr, 0
  );

   // map temporary
  table = ( ld_page_table_t* )map_temporary( ( uintptr_t )table, PAGE_SIZE );

  // assert existance
  assert( NULL != table );

  // ensure not already mapped
  if ( 0 == table->page[ page_idx ].raw ) {
    return;
  }

  // get page
  uintptr_t page = table->page[ page_idx ].data.output_address;

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "page physical address = 0x%08x\r\n", page );
  #endif

  // set page table entry as invalid
  table->page[ page_idx ].raw = 0;

  // free physical page
  phys_free_page( page );

  // unmap temporary
  unmap_temporary( ( uintptr_t )table, PAGE_SIZE );
}

/**
 * @brief Internal v7 long descriptor enable context function
 *
 * @param ctx context structure
 */
void v7_long_set_context( virt_context_ptr_t ctx ) {
  // user context handling
  if ( CONTEXT_TYPE_USER == ctx->type ) {
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "TTBR0: 0x%08x\r\n",
        ( ( ld_global_page_directory_t* )ctx->context )->raw
      );
    #endif
    // Copy page table address to cp15 ( ttbr0 )
    __asm__ __volatile__(
      "mcrr p15, 0, %0, %1, c2"
      : : "r" ( ( ( ld_global_page_directory_t* )ctx->context )->raw ),
        "r" ( 0 )
      : "memory"
    );
  // kernel context handling
  } else if ( CONTEXT_TYPE_KERNEL == ctx->type ) {
    // debug output
    #if defined( PRINT_MM_VIRT )
      DEBUG_OUTPUT(
        "TTBR1: 0x%08x\r\n",
        ( ( ld_global_page_directory_t* )ctx->context )->raw
      );
    #endif
    // Copy page table address to cp15 ( ttbr1 )
    __asm__ __volatile__(
      "mcrr p15, 1, %0, %1, c2"
      : : "r" ( & ( ( ld_global_page_directory_t* )ctx->context )->raw[ 2 ] ),
        "r" ( 0 )
      : "memory"
    );
  // invalid type
  } else {
    PANIC( "Invalid virtual context type!" );
  }
}

/**
 * @brief Flush context
 */
void v7_long_flush_context( void ) {
  ld_ttbcr_t ttbcr;

  // read ttbcr register
  __asm__ __volatile__(
    "mrc p15, 0, %0, c2, c0, 2"
    : "=r" ( ttbcr.raw )
    : : "cc"
  );
  // set split to use ttbr1 and ttbr2
  ttbcr.data.ttbr0_size = 1;
  ttbcr.data.ttbr1_size = 1;
  ttbcr.data.large_physical_address_extension = 1;
  // push back value with ttbcr
  __asm__ __volatile__(
    "mcr p15, 0, %0, c2, c0, 2"
    : : "r" ( ttbcr.raw )
    : "cc"
  );

  // invalidate instruction cache
  __asm__ __volatile__( "mcr p15, 0, %0, c7, c5, 0" : : "r" ( 0 ) );
  // invalidate entire tlb
  __asm__ __volatile__( "mcr p15, 0, %0, c8, c7, 0" : : "r" ( 0 ) );
  // instruction synchronization barrier
  barrier_instruction_sync();
  // data synchronization barrier
  barrier_data_sync();
}

/**
 * @brief Helper to reserve temporary area for mappings
 *
 * @param ctx context structure
 */
void v7_long_prepare_temporary( virt_context_ptr_t ctx ) {
  // ensure kernel for temporary
  assert( CONTEXT_TYPE_KERNEL == ctx->type );

  // last physical table address
  uintptr_t table_physical = 0;
  mapped_temporary_tables = 0;
  // mapped virtual table address
  uintptr_t table_virtual = TEMPORARY_SPACE_START;

  for (
    uintptr_t v = TEMPORARY_SPACE_START;
    v < ( TEMPORARY_SPACE_START + TEMPORARY_SPACE_SIZE );
    v += PAGE_SIZE
  ) {
    // create table if not created
    uintptr_t table = v7_long_create_table( ctx, v, 0 );
    // check if table has changed
    if ( table_physical != table ) {
      // map page table
      v7_long_map( ctx, table_virtual, table, 0 );
      // debug output
      #if defined( PRINT_MM_VIRT )
        DEBUG_OUTPUT(
          "table_virtual = 0x%08x, table_physical = 0x%08x, table = 0x%08x\r\n",
          table_virtual, table_physical, table
        );
      #endif
      // increase virtual and set last physical
      table_virtual += PAGE_SIZE;
      mapped_temporary_tables++;
      table_physical = table;
    }
  }

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "mapped_temporary_tables = %d\r\n", mapped_temporary_tables );
  #endif
}

/**
 * @brief Create context for v7 long descriptor
 *
 * @param type context type to create
 */
virt_context_ptr_t v7_long_create_context( virt_context_type_t type ) {
  // create new context
  uintptr_t ctx = phys_find_free_page_range( PAGE_SIZE, PAGE_SIZE );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "type: %d, ctx: 0x%08x\r\n", type, ctx );
  #endif

  // map temporary
  uintptr_t tmp = map_temporary( ctx, PAGE_SIZE );
  // initialize with zero
  memset( ( void* )tmp, 0, PAGE_SIZE );
  // unmap temporary
  unmap_temporary( tmp, PAGE_SIZE );

  // create new context structure for return
  virt_context_ptr_t context = ( virt_context_ptr_t )malloc(
    sizeof( virt_context_t )
  );

  // debug output
  #if defined( PRINT_MM_VIRT )
    DEBUG_OUTPUT( "context: 0x%08x, ctx: 0x%08x\r\n", context, ctx );
  #endif

  // initialize with zero
  memset( ( void* )context, 0, sizeof( virt_context_t ) );

  // populate type and context
  context->context = ctx;
  context->type = type;

  // return blank context
  return context;
}

/**
 * @brief Method to prepare
 *
 * @todo do some general setup like mair
 */
void v7_long_prepare( void ) {}
