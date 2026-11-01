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
#include <errno.h>
#include <unistd.h>
#include <sys/bolthur.h>

/**
 * @brief Main entry point
 * @param argc
 * @param argv
 * @return
 */
int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] ) {
  _syscall_rpc_wait_for_ready( getppid() );
  EARLY_STARTUP_PRINT( "%d is ready\r\n", getppid() )
  // wait for device
  vfs_wait_for_path( "/dev/manager/device", false );
  // wait for nice daemon
  vfs_wait_for_path( NICE_DEVICE, false );
  // increase nice level
  if ( 0 != nice( 19 ) ) {
    EARLY_STARTUP_PRINT( "Increase of nice level failed with %s\r\n", strerror( errno ) ) ;
    return EXIT_FAILURE;
  }
  // add device file
  if ( ! vfs_dev_add_file( "/dev/idle", nullptr, 0, nullptr ) ) {
    return EXIT_FAILURE;
  }
  // loop and halt
  while ( true ) {
    _syscall_process_halt();
  }
}
