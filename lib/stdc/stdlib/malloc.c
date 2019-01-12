
/**
 * bolthur/kernel
 * Copyright (C) 2017 - 2019 bolthur project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#if defined( IS_KERNEL )
  #include "kernel/mm/kmalloc.h"
#endif

/**
 * @brief malloc
 *
 * @param size amount to allocate
 * @return void* allocated address
 *
 * @todo Implement non kernel related
 */
void *malloc( size_t size ) {
  #if defined( IS_KERNEL )
    return kmalloc( size, NULL, 0 );
  #else
    return NULL;
  #endif
}
