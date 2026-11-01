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

#include "../../library/vfs/dev.h"
#include "../../library/vfs/wait.h"
#include <sys/bolthur.h>
#include <sys/unistd.h>
#include "rpc.h"

/**
 * @brief Main entry point
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  _syscall_rpc_wait_for_ready( getppid() );
  // wait for parent to be ready
  EARLY_STARTUP_PRINT( "Waiting for /dev/manager/device with sleep\r\n" )
  vfs_wait_for_path( "/dev/manager/device", true );
  // initialize rpc
  EARLY_STARTUP_PRINT( "Bind rpc\r\n" )
  if ( ! rpc_init() ) {
    return EXIT_FAILURE;
  }
  // ready for syscalls
  EARLY_STARTUP_PRINT( "Enable rpc\r\n" )
  _syscall_rpc_set_ready( true );
  // device info
  constexpr uint32_t device_info[] = {
    RPC_VFS_IOCTL_NICE_LEVEL_SET,
  };
  // add device file
  EARLY_STARTUP_PRINT( "Sending %s to vfs\r\n", NICE_DEVICE )
  if ( ! vfs_dev_add_file( NICE_DEVICE, device_info, 1, nullptr ) ) {
    return EXIT_FAILURE;
  }
  // loop until syscall comes in
  EARLY_STARTUP_PRINT( "Waiting for rpc\r\n" )
  bolthur_rpc_wait_block();
  // return success => should never be reached
  return EXIT_SUCCESS;
}
