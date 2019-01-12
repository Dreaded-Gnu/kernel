
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

#include <stdint.h>
#include <stdio.h>

#include "kernel/panic.h"

#include "vendor/rpi/platform.h"
#include "vendor/rpi/mailbox-property.h"

/**
 * @brief Boot parameter data set during startup
 */
platform_boot_parameter_t boot_parameter_data;

/**
 * @brief Platform depending initialization routine
 */
void platform_init( void ) {
  // FIXME: Load firmware revision, board model, board revision, board serial from mailbox
  /*mailbox_property_init();
  mailbox_property_add_tag( TAG_GET_BOARD_MODEL );
  mailbox_property_add_tag( TAG_GET_BOARD_REVISION );
  mailbox_property_add_tag( TAG_GET_FIRMWARE_VERSION );
  mailbox_property_add_tag( TAG_GET_BOARD_SERIAL );
  mailbox_property_process();*/
}
