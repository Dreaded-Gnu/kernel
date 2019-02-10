
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

#include "kernel/arch/arm/delay.h"

/**
 * @brief Delay function by given amount of cpu cycles
 *
 * @param count Amount of cycles to delay
 *
 * @todo check and revise
 */
void __attribute__(( optimize( "O0" ) )) delay( uint32_t count ) {
  __asm__ __volatile__(
    "__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n": "=r" ( count ): [ count ] "0" ( count ) : "cc"
  );
}