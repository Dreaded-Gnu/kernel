
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

#if ! defined( __ARCH_ARM_MM_VIRT__ )
#define __ARCH_ARM_MM_VIRT__

#include <stdint.h>

#if defined( ELF32 )
  // supported paging defines
  #define ID_MMFR0_VSMA_V6_PAGING 0x2
  #define ID_MMFR0_VSMA_V7_PAGING_REMAP_ACCESS 0x3
  #define ID_MMFR0_VSMA_V7_PAGING_PXN 0x4
  #define ID_MMFR0_VSMA_V7_PAGING_LPAE 0x5

  // methods
  void virt_setup_supported_modes( void );
  // supported modes
  extern uint32_t supported_modes;
#endif

#endif
