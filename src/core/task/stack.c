
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

#include <string.h>
#include <stdlib.h>
#include <core/debug/debug.h>
#include <core/task/stack.h>
#include <core/task/process.h>

/**
 * @brief Stack management structure
 */
task_stack_manager_ptr_t task_stack_manager = NULL;

/**
 * @brief Compare stack callback necessary for avl tree
 *
 * @param a node a
 * @param b node b
 * @return int32_t
 */
static int32_t task_stack_callback(
  const avl_node_ptr_t a,
  const avl_node_ptr_t b
) {
  // debug output
  #if defined( PRINT_PROCESS )
    DEBUG_OUTPUT( "a = %p, b = %p\r\n", ( void* )a, ( void* )b );
    DEBUG_OUTPUT( "a->data = %p, b->data = %p\r\n", a->data, b->data );
  #endif

  // -1 if address of a is greater than address of b
  if ( ( uintptr_t )a->data > ( uintptr_t )b->data ) {
    return -1;
  // 1 if address of b is greater than address of a
  } else if ( ( uintptr_t )b->data > ( uintptr_t )a->data ) {
    return 1;
  }

  // equal => return 0
  return 0;
}

/**
 * @brief Destroy task stack manager
 *
 * @param manager
 */
void task_stack_manager_destroy( task_stack_manager_ptr_t manager ) {
  // handle invalid
  if ( ! manager ) {
    return;
  }

  // destroy tree
  avl_destroy_tree( manager->tree );

  // free up manager
  free( manager );
}

/**
 * @brief Create stack manager
 *
 * @return task_stack_manager_ptr_t
 */
task_stack_manager_ptr_t task_stack_manager_create( void ) {
  // allocate manager
  task_stack_manager_ptr_t manager = ( task_stack_manager_ptr_t )malloc(
    sizeof( task_stack_manager_t ) );
  // check allocation
  if ( ! manager ) {
    return NULL;
  }
  // prepare
  memset( ( void* )manager, 0, sizeof( task_stack_manager_t ) );
  // create tree
  manager->tree = avl_create_tree( task_stack_callback, NULL, NULL );
  if ( ! manager->tree ) {
    free( manager );
    return NULL;
  }
  // return manager
  return manager;
}

/**
 * @brief Add stack to manager
 *
 * @param stack stack to add
 * @return true
 * @return false
 */
bool task_stack_manager_add(
  uintptr_t stack,
  task_stack_manager_ptr_t manager
) {
  // check manager
  if ( ! manager ) {
    return false;
  }
  // create node
  avl_node_ptr_t node = avl_create_node( ( void* )stack );
  // handle error
  if ( ! node ) {
    return false;
  }
  // insert node
  return avl_insert_by_node( manager->tree, node );
}

/**
 * @brief Remove stack from manager
 *
 * @param stack stack to remove
 * @param manager manager
 * @return true
 * @return false
 */
bool task_stack_manager_remove(
  uintptr_t stack,
  task_stack_manager_ptr_t manager
) {
  // check manager
  if ( ! manager ) {
    return false;
  }
  // try to get node
  avl_node_ptr_t node = avl_find_by_data( manager->tree, ( void* )stack );
  // handle not found
  if ( ! node ) {
    return true;
  }
  // remove node
  avl_remove_by_node( manager->tree, node );
  // free node
  free( node );
  // return success
  return true;
}
