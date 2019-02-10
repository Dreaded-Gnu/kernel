
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

#include "lib/stdc/stdlib.h"
#include "kernel/kernel/serial.h"
#include "kernel/vendor/rpi/framebuffer.h"

/**
 * @brief Initialize TTY
 */
void tty_init( void ) {
  #if defined( TTY_ENABLE )
    #if defined( TTY_FRAMEBUFFER )
      framebuffer_init();
    #endif

    #if defined( TTY_SERIAL ) && ! defined( DEBUG )
      serial_init();
    #endif
  #endif
}

/**
 * @brief Print character to TTY
 *
 * @param c Character to print
 */
void tty_putc( uint8_t c ) {
  #if defined( TTY_ENABLE )
    // print also to serial if enabled
    #if defined( TTY_SERIAL )
      serial_putc( c );
    #endif

    // print to framebuffer if enabled
    #if defined( TTY_FRAMEBUFFER )
      framebuffer_putc( c );
    #endif
  #else
    ( void )c;
  #endif
}