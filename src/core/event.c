
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

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <list.h>
#include <core/panic.h>
#include <core/debug/debug.h>
#include <core/event.h>

/**
 * @brief event manager structure
 */
event_manager_ptr_t event = NULL;

/**
 * @brief Compare event callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t compare_event_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // get blocks
  event_block_ptr_t block_a = EVENT_GET_BLOCK( a );
  event_block_ptr_t block_b = EVENT_GET_BLOCK( b );

  // -1 if address of a is greater than address of b
  if ( block_a->type > block_b->type ) {
    return -1;
  // 1 if address of b is greater than address of a
  } else if ( block_b->type > block_a->type ) {
    return 1;
  }

  // equal => return 0
  return 0;
}

/**
 * @brief Method to setup event system
 */
void event_init( void ) {
  // create manager structure
  event = ( event_manager_ptr_t )malloc( sizeof( event_manager_t ) );
  // assert result
  assert( NULL != event );
  // prepare
  memset( ( void* )event, 0, sizeof( event_manager_t ) );
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Initialized event manager structure at %p\r\n",
      ( void* )event );
  #endif

  // create tree
  event->tree = avl_create_tree( compare_event_callback );
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Created event tree at: %p\r\n", ( void* )event->tree );
  #endif

  // create queue
  event->queue_kernel = list_construct();
  event->queue_user = list_construct();
}

/**
 * @brief Bind event callback
 *
 * @param type event type
 * @param callback callback to bind
 * @param post post callback mapping
 * @return true on success
 * @return false on error
 */
bool event_bind( event_type_t type, event_callback_t callback, bool post ) {
  // do nothing if not initialized
  if ( NULL == event ) {
    return true;
  }

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Called event_bind( %d, %p, %s )\r\n",
      type, callback, post ? "true" : "false" );
  #endif
  // get correct tree to use
  avl_tree_ptr_t tree = event->tree;

  // try to find node
  avl_node_ptr_t node = avl_find_by_data( tree, ( void* )type );
  event_block_ptr_t block;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Found node %p\r\n", ( void* )node );
  #endif
  // handle not yet added
  if ( NULL == node ) {
    // allocate block
    block = ( event_block_ptr_t )malloc( sizeof( event_block_t ) );
    // assert initialization
    assert( NULL != block );
    // prepare memory
    memset( ( void* )block, 0, sizeof( event_block_t ) );
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Initialized new node at %p\r\n", ( void* )block );
    #endif
    // populate block
    block->type = type;
    block->handler = list_construct();
    block->post = list_construct();
    // prepare and insert node
    avl_prepare_node( &block->node, ( void* )type );
    avl_insert_by_node( tree, &block->node );
  // existing? => gather block
  } else {
    block = EVENT_GET_BLOCK( node );
  }

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Checking for already bound event callback\r\n" );
  #endif
  // get first element
  list_item_ptr_t current = true != post
      ? block->handler->first
      : block->post->first;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Used first element for looping at %p\r\n", ( void* )current );
  #endif
  // loop through list for check callback
  while ( NULL != current ) {
    // get callback from data
    event_callback_wrapper_ptr_t wrapper =
      ( event_callback_wrapper_ptr_t )current->data;
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Check bound callback at %p\r\n", ( void* )wrapper );
    #endif
    // handle match
    if ( wrapper->callback == callback ) {
      // debug output
      #if defined( PRINT_EVENT )
        DEBUG_OUTPUT( "Callback already existing\r\n" );
      #endif
      // return success
      return true;
    }
    // get to next
    current = current->next;
  }

  // create wrapper
  event_callback_wrapper_ptr_t wrapper = ( event_callback_wrapper_ptr_t )malloc(
    sizeof( event_callback_wrapper_t ) );
  // assert initialization
  assert( NULL != wrapper );
  // prepare memory
  memset( ( void* )wrapper, 0, sizeof( event_callback_wrapper_t ) );
  // populate wrapper
  wrapper->callback = callback;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Created wrapper container at %p\r\n", ( void* )wrapper );
  #endif
  // push to list
  list_push_back(
    true != post
      ? block->handler
      : block->post,
    ( void* )wrapper );

  // return success
  return true;
}

/**
 * @brief Unbind event if existing
 *
 * @param type event type
 * @param callback bound callback
 * @param post post callback
 *
 * @todo Add logic for method
 */
void event_unbind(
  __unused event_type_t type,
  __unused event_callback_t callback,
  __unused bool post
) {
  // do nothing if not initialized
  if ( NULL == event ) {
    return;
  }

  PANIC( "event_unbind not yet implemented!" );
}

/**
 * @brief Enqueue event
 *
 * @param type type to enqueue
 * @param origin event origin
 */
void event_enqueue( event_type_t type, event_origin_t origin ) {
  // do nothing if not initialized
  if ( NULL == event ) {
    return;
  }

  // push back event
  list_push_back(
    EVENT_ORIGIN_KERNEL == origin
      ? event->queue_kernel
      : event->queue_user,
    ( void* )type
  );
}

/**
 * @brief Handle enqueued events with data
 *
 * @param data data to pass through
 */
void event_handle( void* data ) {
  // do nothing if not initialized
  if ( NULL == event ) {
    return;
  }

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Enter event_handle( %p )\r\n", data );
  #endif

  // determine origin
  event_origin_t origin = EVENT_DETERMINE_ORIGIN( data );
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "origin = %d\r\n", origin );
  #endif
  // queue to use
  list_manager_ptr_t queue = EVENT_ORIGIN_KERNEL == origin
    ? event->queue_kernel
    : event->queue_user;
  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "queue = %p\r\n", ( void* )queue );
  #endif

  // variables
  list_item_ptr_t current_event = list_pop_front( queue );
  // get correct tree to use
  avl_tree_ptr_t tree = event->tree;

  while ( NULL != current_event ) {
    // try to find node
    avl_node_ptr_t node = avl_find_by_data( tree, current_event );
    event_block_ptr_t block;
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Found node %p\r\n", ( void* )node );
    #endif

    // handle no existing
    if ( NULL == node ) {
      // pop next
      current_event = list_pop_front( queue );
      // skip rest
      continue;
    }
    // get block
    block = EVENT_GET_BLOCK( node );

    // get first element of normal callback list
    list_item_ptr_t current = block->handler->first;
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Used first normal element for looping at %p\r\n",
        ( void* )current );
    #endif
    // loop through list
    while ( NULL != current ) {
      // get callback from data
      event_callback_wrapper_ptr_t wrapper =
        ( event_callback_wrapper_ptr_t )current->data;
      // debug output
      #if defined( PRINT_EVENT )
        DEBUG_OUTPUT( "Executing bound callback %p\r\n", ( void* )wrapper );
      #endif
      // fire with data
      wrapper->callback( origin, data );
      // step to next
      current = current->next;
    }

    // get first element of post callback list
    current = block->post->first;
    // debug output
    #if defined( PRINT_EVENT )
      DEBUG_OUTPUT( "Used first post element for looping at %p\r\n",
        ( void* )current );
    #endif
    // loop through list
    while ( NULL != current ) {
      // get callback from data
      event_callback_wrapper_ptr_t wrapper =
        ( event_callback_wrapper_ptr_t )current->data;
      // debug output
      #if defined( PRINT_EVENT )
        DEBUG_OUTPUT( "Executing bound callback %p\r\n", ( void* )wrapper );
      #endif
      // fire with data
      wrapper->callback( origin, data );
      // step to next
      current = current->next;
    }

    // get next element
    current_event = list_pop_front( queue );
  }

  // debug output
  #if defined( PRINT_EVENT )
    DEBUG_OUTPUT( "Leave event_handle\r\n" );
  #endif
}
