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

#include <sys/bolthur.h>
#include "console.h"

/**
 * @fn void console_complete(size_t, pid_t, uint64_t, uint64_t)
 * @brief Console transfer complete command
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void console_complete(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const uint64_t data_info,
  const uint64_t response_info
) {
  // get async data and destroy it directly
  bolthur_async_data_t* async_data = bolthur_rpc_pop_async( RPC_VFS_IOCTL, response_info );
  bolthur_rpc_destroy_async( async_data );
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    _syscall_rpc_cleanup();
    return;
  }
  // handle no data
  if ( ! data_info ) {
    _syscall_rpc_cleanup();
    return;
  }
  // get data from mailbox
  size_t data_size;
  vfs_ioctl_perform_response_t* response = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! response ) {
    _syscall_rpc_cleanup();
    return;
  }
  // free response and cleanup
  free( response );
  _syscall_rpc_cleanup();
}
