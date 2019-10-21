
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
#include <assert.h>
#include <stdlib.h>
#include <list.h>

void list_push( list_item_ptr_t *list, void* data ) {
  list_item_ptr_t first, node;

  // assert list is initialized
  assert( NULL != list && NULL != *list );
  // set list head
  first = *list;
  // handle empty list
  if ( NULL == first->data ) {
    first->data = data;
    return;
  }

  // create new node
  node = list_node_create( data );
  // set next to first
  node->next = first;
  // set previous for first element
  first->previous = node;

  // overwrite first element within list pointer
  *list = node;
}
