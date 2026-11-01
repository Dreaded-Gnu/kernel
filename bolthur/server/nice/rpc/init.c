/**
 * Copyright (C) 2018 - 2026 bolthur project.
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

#include "../rpc.h"
#include <errno.h>
#include <sys/bolthur.h>

/**
 * @fn void rpc_init(void)
 * @brief Initialize rpc handler
 * @returns
 */
bool rpc_init( void ) {
  bolthur_rpc_bind( RPC_VFS_IOCTL_NICE_LEVEL_SET, rpc_nice_set, true );
  if ( errno ) {
    #if defined( CONSOLE_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to register handler close!\r\n" )
    #endif
    return false;
  }
  return true;
}
