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

#include <errno.h>
#include <sys/bolthur.h>
#include "../../rpc.h"

/**
 * @fn void rpc_nice_set( size_t, pid_t, uint64_t, uint64_t )
 * @brief Nice level set function
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_nice_set(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] uint64_t data_info,
  [[maybe_unused]] uint64_t response_info
) {
  vfs_ioctl_perform_response_t error = { .status = -EINVAL, };
  // validate origin
  if ( ! bolthur_rpc_validate_origin( origin, data_info ) ) {
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // handle no data
  if ( ! data_info ) {
    error.status = -ENODATA;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get request
  size_t data_size;
  vfs_ioctl_perform_request_t* request = bolthur_rpc_fetch_from_mailbox( data_info, &data_size, true, nullptr );
  if ( ! request ) {
    error.status = -ENOMSG;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // get data
  auto const data = ( vfs_nice_level_set_request_t* )request->container;
  /// FIXME: CHECK PRIVILEGES AGAINST AUTHENTICATION IN CASE OF NEGATIVE VALUE
  // call syscall
  _syscall_thread_nice_level( request->origin, data->thread, data->nice_level );
  if ( errno ) {
    error.status = -errno;
    free( request );
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    return;
  }
  // allocate space for return
  constexpr size_t container_size = sizeof( vfs_nice_level_set_response_t );
  constexpr size_t response_size = container_size + sizeof( vfs_ioctl_perform_response_t );
  vfs_ioctl_perform_response_t* response = malloc( response_size );
  if ( ! response ) {
    error.status = -ENOMEM;
    bolthur_rpc_return( RPC_VFS_IOCTL, &error, sizeof( error ), nullptr, 0 );
    free( request );
    return;
  }
  // clear out
  memset( response, 0, response_size );
  // return success response
  bolthur_rpc_return( RPC_VFS_IOCTL, response, response_size, nullptr, 0 );
  // free up stuff
  free( request );
  free( response );
}
