
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

#include "kernel/entry.h"
#include "kernel/bss.h"

void bss_clear( void ) {
  bss_type_t *start = ( bss_type_t* )&__bss_start;
  bss_type_t *end = ( bss_type_t* )&__bss_end;

  // FIXME: Translate to physical when higher half is enabled
  #if defined( IS_HIGHER_HALF )
    start -= KERNEL_OFFSET;
    end -= KERNEL_OFFSET;
  #endif

  // loop through bss end and overwrite with zero
  while( start < end ) {
    *start++ = 0;
  }
}