
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

#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <avl/avl.h>
#include <kernel/debug.h>
#include <kernel/panic.h>
#include <kernel/mm/phys.h>
#include <kernel/mm/virt.h>
#include <kernel/mm/heap.h>

/**
 * @brief Kernel heap
 */
heap_manager_ptr_t kernel_heap = NULL;

/**
 * @brief Getter for heap initialized flag
 *
 * @return true
 * @return false
 */
bool heap_initialized_get( void ) {
  return NULL != kernel_heap;
}

/**
 * @brief Compare address callback necessary for avl tree
 *
 * @param avl_a node a
 * @param avl_b node b
 * @param avl_param parameter
 * @return int32_t
 */
static int32_t compare_address_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b,
  void *param
) {
  // mark parameter as unused
  ( void )param;

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "a = 0x%08x, b = 0x%08x\r\n", a, b );
    DEBUG_OUTPUT( "a->data = 0x%08x, b->data = 0x%08x\r\n", a->data, b->data );
  #endif

  // -1 if address of a is greater than address of b
  if ( a->data > b->data ) {
    return -1;
  // 1 if address of b is greater than address of a
  } else if ( b->data > a->data ) {
    return 1;
  }

  // equal => return 0
  return 0;
}

/**
 * @brief Initialize heap
 */
void heap_init( void ) {
  // assert not initialized
  assert( NULL == kernel_heap );

  // offset for first free block
  uint32_t offset = 0;
  vaddr_t start = ( vaddr_t )HEAP_START;

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "Heap start: 0x%08x, Heap end: 0x%08x\r\n",
      HEAP_START,
      HEAP_START + HEAP_MIN_SIZE
    );
  #endif

  // map heap address space
  for (
    vaddr_t addr = start;
    addr < ( vaddr_t )( ( paddr_t )HEAP_START + HEAP_MIN_SIZE );
    addr = ( vaddr_t )( ( paddr_t )addr + PAGE_SIZE )
  ) {
    // Find free page
    paddr_t phys = ( paddr_t )phys_find_free_page( PAGE_SIZE );

    // debug output
    #if defined( PRINT_MM_HEAP )
      DEBUG_OUTPUT( "Map 0x%08x to 0x%08x\r\n", phys, addr );
    #endif

    // map address
    virt_map_address(
      kernel_context, addr, phys, PAGE_FLAG_BUFFERABLE | PAGE_FLAG_CACHEABLE
    );
  }

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "heap start: 0x%08x\r\n", HEAP_START );
  #endif

  // set kernel heap and increase offset
  kernel_heap = ( heap_manager_ptr_t )HEAP_START;
  offset += sizeof( heap_manager_t );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "HEAP_START content: 0x%08x\t\n",
      *( ( uint32_t* )HEAP_START )
    );
    DEBUG_OUTPUT( "Placed heap management at 0x%08x\r\n", kernel_heap );
    DEBUG_OUTPUT( "offset: %d\r\n", offset );
  #endif

  // initialize management structure
  memset( ( vaddr_t )kernel_heap, 0, sizeof( heap_manager_t ) );

  // populate trees
  kernel_heap->free_area_address.compare = compare_address_callback;
  kernel_heap->used_area.compare = compare_address_callback;

  // create free block
  heap_block_ptr_t free_block = ( heap_block_ptr_t )(
    ( paddr_t )HEAP_START + offset
  );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Created free block at 0x%08x\r\n", free_block );
  #endif

  // increase offset
  offset += sizeof( heap_block_t );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "Heap offset: %d\r\n", offset );
  #endif

  // prepare free block
  free_block->address = ( vaddr_t )(
    ( paddr_t )HEAP_START + offset
  );
  free_block->size = HEAP_MIN_SIZE - offset;
  // populate node data
  free_block->node_address.data = ( vaddr_t )free_block->address;

  // insert into free tree
  avl_insert(
    &kernel_heap->free_area_address,
    &free_block->node_address
  );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "%s", "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area );

    DEBUG_OUTPUT( "%s", "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_area_address );
  #endif
}

/**
 * @brief Allocate block within heap
 *
 * @param size size to allocate
 * @param alignment address aligment
 * @return vaddr_t address of allocated block
 */
vaddr_t heap_allocate_block( size_t size ) {
  // variables
  heap_block_ptr_t block, new_block;
  avl_node_ptr_t address_node;
  size_t real_size;

  // calculate real size
  real_size = size + sizeof( heap_block_t );
  uint32_t remaining_size;

  /// FIXME: try to find first one by size

  // get max node
  address_node = avl_get_max( &kernel_heap->free_area_address );
  // check for error or empty
  if ( NULL == address_node ) {
    PANIC( "Size tree is empty, need to extend heap!" );
  }

  // get block to split
  block = GET_BLOCK_ADDRESS( address_node );
  // check for enough size for split
  if ( block->size <= real_size ) {
    PANIC( "Block to split is to small, need to extend heap!" );
  }

  // remove nodes from free trees
  avl_remove( &kernel_heap->free_area_address, &block->node_address );
  // calculate remaining size
  remaining_size = block->size - real_size;
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "block->size = %d, remaining_size = %d\r\n",
      block->size,
      remaining_size
    )
  #endif
  // update block
  block->size = remaining_size;

  // insert nodes from free trees
  avl_insert( &kernel_heap->free_area_address, &block->node_address );

  // place new block
  new_block = ( heap_block_ptr_t )(
    ( paddr_t )block->address + remaining_size
  );

  // clear new block
  memset( ( vaddr_t )new_block, 0, real_size );
  // setup new block
  new_block->size = size;
  new_block->address = ( vaddr_t )(
    ( paddr_t )new_block + sizeof( heap_block_t )
  );
  new_block->node_address.data = new_block->address;

  // insert at used block
  avl_insert( &kernel_heap->used_area, &new_block->node_address );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "%s", "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area );

    DEBUG_OUTPUT( "%s", "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_area_address );
  #endif

  // return address of block
  return new_block->address;
}

/**
 * @brief Free block within heap
 *
 * @param addr address to free
 */
void heap_free_block( vaddr_t addr ) {
  // variables
  avl_node_ptr_t address_node, parent_node;

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "addr = 0x%08x\r\n", addr );
  #endif

  // finde node by address within tree
  address_node = avl_find( &kernel_heap->used_area, addr );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT(
      "address_node = 0x%08x, data = 0x%08x\r\n",
      address_node,
      address_node->data
    );
    DEBUG_OUTPUT(
      "used_tree root data: 0x%08x\r\n", kernel_heap->used_area.root->data
    );
  #endif

  // skip if nothing has been found
  if ( NULL == address_node ) {
    PANIC( "NOT FOUND!" );
    return;
  }

  // get memory block
  heap_block_ptr_t current_block = GET_BLOCK_ADDRESS( address_node );

  // debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "current_block = 0x%08x\r\n", current_block );
  #endif

  // remove node from used block
  avl_remove( &kernel_heap->used_area, address_node );

  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "removed 0x%08x from tree\r\n", &current_block->node_address );
  #endif

  // insert into free tree
  avl_insert( &kernel_heap->free_area_address, address_node );

  // find parent node
  parent_node = avl_find_parent(
    &kernel_heap->free_area_address,
    address_node->data
  );

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "%s", "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area );

    DEBUG_OUTPUT( "%s", "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_area_address );
  #endif

  // no parent node found, skip
  if ( NULL == parent_node ) {
    return;
  }

  PANIC( "PARENT!" );

  // get parent and current block
  heap_block_ptr_t parent_block = GET_BLOCK_ADDRESS( parent_node );

  // determine end of parent and begin of current block
  vaddr_t end = ( vaddr_t )( ( paddr_t )&parent_block + parent_block->size );
  vaddr_t begin = ( vaddr_t )( ( paddr_t )&current_block );

  // merge on match
  if ( begin == end ) {
    // remove parent to keep tree balanced
    avl_remove( &kernel_heap->free_area_address, &parent_block->node_address );

    // increase size
    parent_block->size += current_block->size;

    // remove current block as it has been merged into parent
    avl_remove( &kernel_heap->free_area_address, &current_block->node_address );

    // insert parent again
    avl_insert( &kernel_heap->free_area_address, &parent_block->node_address );
  }

  // Debug output
  #if defined( PRINT_MM_HEAP )
    DEBUG_OUTPUT( "%s", "Used tree:\r\n" );
    avl_print( &kernel_heap->used_area );

    DEBUG_OUTPUT( "%s", "Free address tree:\r\n" );
    avl_print( &kernel_heap->free_area_address );
  #endif
}
