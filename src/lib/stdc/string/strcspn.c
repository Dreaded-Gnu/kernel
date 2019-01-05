
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

#include <string.h>
#include <stdlib.h>

#if defined( IS_KERNEL )
  #include <panic.h>
#endif

// FIXME: Add logic
size_t strcspn( const char* str1, const char* str2 ) {
  // mark parameter as unused
  ( void )str1;
  ( void )str2;

  #if defined( IS_KERNEL )
    PANIC( "strcspn not yet implemented!" );
  #else
    abort();
  #endif

  return 0;
}
