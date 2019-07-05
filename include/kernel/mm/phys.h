
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

#if ! defined( __KERNEL_MM_PHYS__ )
#define __KERNEL_MM_PHYS__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_PER_ENTRY ( sizeof( phys_bitmap_length ) * 8 )
#define PAGE_INDEX( address ) ( address / PAGE_PER_ENTRY )
#define PAGE_OFFSET( address ) ( address % PAGE_PER_ENTRY )

#define PHYS_NO_ALIGNMENT 0

#define PHYS_ALL_PAGES_OF_INDEX_USED 0xFFFFFFFF

#define PAGE_SIZE 0x1000

extern uint32_t *phys_bitmap;
extern size_t phys_bitmap_length;

void phys_init( void );
void phys_platform_init( void );

void phys_mark_page_used( uintptr_t );
void phys_mark_page_free( uintptr_t );
uintptr_t phys_find_free_page_range( size_t, size_t );
void phys_free_page_range( uintptr_t, size_t );
void phys_use_page_range( uintptr_t, size_t );
uintptr_t phys_find_free_page( size_t );
void phys_free_page( uintptr_t );
bool phys_initialized_get( void );

#endif
