
/**
 * mist-system/kernel
 * Copyright (C) 2017 - 2018 mist-system project.
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

#include <panic.h>
#include <_vendor/_rpi/mailbox.h>

// FIXME: Add logic!
uint32_t mailbox_read( uint8_t channel ) {
  // mark parameter as unused
  ( void )channel;

  // panic until implementation is existing
  PANIC( "mailbox_read not yet implemented!" );

  // dummy return
  return -1;
}

// FIXME: Add logic!
void mailbox_write( uint8_t channel, uint32_t data ) {
  // mark parameter as unused
  ( void )channel;
  ( void )data;

  // panic until implementation is existing
  PANIC( "mailbox_write not yet implemented!" );
}
