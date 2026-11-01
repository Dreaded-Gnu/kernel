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

#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include "../ramdisk.h"
#include "../init.h"
#include "../util.h"
#include "../global.h"
#include "../../../library/vfs/wait.h"

pid_t vfs_pid = 2;
pid_t dev_pid = 3;
pid_t idle_pid = 4;
pid_t nice_pid = 5;
pid_t ramdisk_pid = 6;
pid_t authentication_pid = 7;

/**
 * @fn pid_t stat_handler(const char*)
 * @brief Stat handler helper
 * @param file
 * @return
 */
static pid_t stat_handler( const char* file ) {
  // variables
  vfs_stat_request_t* request = malloc( sizeof( vfs_stat_request_t ) );
  if ( ! request ) {
    errno = ENOMEM;
    return -1;
  }
  // clear message structures
  memset( request, 0, sizeof( vfs_stat_request_t ) );
  // copy stuff to message
  strncpy( request->file_path, file, PATH_MAX - 1 );
  // raise rpc and wait for return
  const uint64_t response_id = bolthur_rpc_raise(
    RPC_VFS_STAT,
    VFS_DAEMON_ID,
    request,
    sizeof( vfs_stat_request_t ),
    nullptr,
    RPC_VFS_STAT,
    request,
    sizeof( vfs_stat_request_t ),
    0,
    0,
    nullptr,
    false
  );
  // handle error
  if ( 0 == response_id ) {
    free( request );
    return -1;
  }
  size_t data_size;
  vfs_stat_response_t* response = bolthur_rpc_fetch_from_mailbox(
    response_id,
    &data_size,
    true,
    NULL
  );
  // handle error
  if ( ! response ) {
    free( request );
    return -1;
  }
  // handle failure
  if ( ! response->success ) {
    free( request );
    free( response );
    errno = EIO;
    return -1;
  }
  // cache handler
  const pid_t handler = response->handler;
  // free request and response
  free( request );
  free( response );
  // return success
  return handler;
}

/**
 * @fn void init_stage1(void)
 * @brief Stage 1 init with start of necessary stuff ( VFS, DEV, and RAMDISK )
 */
void init_stage1( void ) {
  // transform number to string
  int len = snprintf( nullptr, 0, "%"PRIu64, ramdisk_shared_id ) + 1;
  char* shm_id_str = malloc( sizeof( char ) * ( size_t )len );
  if ( ! shm_id_str ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to allocate space for parameter\r\n" )
    #endif
    exit( -1 );
  }
  memset( shm_id_str, 0, sizeof( char ) * ( size_t )len );
  sprintf( shm_id_str, "%"PRIu64, ramdisk_shared_id );

  // get vfs image
  size_t vfs_size;
  void* vfs_image = ramdisk_lookup( disk, "ramdisk/bin/server/fs/vfs", &vfs_size );
  if ( ! vfs_image ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "VFS not found!\r\n" )
    #endif
    exit( -1 );
  }
  #if defined( BOOT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "VFS image: %p!\r\n", vfs_image )
    // fork process and handle possible error
    EARLY_STARTUP_PRINT( "Forking process for vfs start!\r\n" )
  #endif
  pid_t forked_process = _syscall_process_fork();
  if ( errno ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Unable to fork process\r\n" )
      EARLY_STARTUP_PRINT( "%s\r\n", strerror( errno ) )
    #endif
    exit( -1 );
  }
  // fork only
  if ( 0 == forked_process ) {
    // start /dev
    #if defined( BOOT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Starting for dev server...\r\n" )
    #endif
    size_t dev_size;
    void* dev_image = ramdisk_lookup( disk, "ramdisk/bin/server/fs/dev", &dev_size );
    if ( ! dev_image ) {
      #if defined( BOOT_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "dev server not found!\r\n" )
      #endif
      exit( -1 );
    }
    // fork process and handle possible error
    #if defined( BOOT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Forking process for dev start!\r\n" )
    #endif
    pid_t inner_forked_process = _syscall_process_fork();
    if ( errno ) {
      #if defined( BOOT_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Unable to fork process\r\n" )
        EARLY_STARTUP_PRINT( "%s\r\n", strerror( errno ) )
      #endif
      exit( -1 );
    }

    // call for vfs replace
    if ( 0 != inner_forked_process ) {
      #if defined( BOOT_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Replacing fork with vfs image %p!\r\n", vfs_image );
      _syscall_process_replace( vfs_image, nullptr, nullptr );
      #endif
      if ( errno ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Unable to replace process with image\r\n" )
          EARLY_STARTUP_PRINT( "%s\r\n", strerror( errno ) )
        #endif
        exit( -1 );
      }
    }

    // handle fork
    if ( 0 == inner_forked_process ) {
      // enable rpc
      _syscall_rpc_set_ready( true );
      // wait for vfs to be ready
      #if defined( BOOT_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "waiting for vfs and dev!\r\n" )
      #endif
      vfs_wait_for_path( ":/vfs", false );
      // disable rpc again
      _syscall_rpc_set_ready( false );

      // start /dev/idle
      #if defined( BOOT_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Forking process for idle start!\r\n" )
      #endif
      inner_forked_process = _syscall_process_fork();
      if ( errno ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Unable to fork process\r\n" )
          EARLY_STARTUP_PRINT( "%s\r\n", strerror( errno ) )
        #endif
        exit( -1 );
      }
      if ( 0 == inner_forked_process ) {
        // get idle image
        size_t idle_size;
        void* idle_image = ramdisk_lookup( disk, "ramdisk/bin/server/idle", &idle_size );
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Idle image: %p!\r\n", idle_image )
        #endif
        if ( ! idle_image ) {
          #if defined( BOOT_ENABLE_OUTPUT )
            EARLY_STARTUP_PRINT( "Idle not found!\r\n" )
          #endif
          exit( -1 );
        }
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Replacing fork with idle image %p!\r\n", idle_image );
        _syscall_process_replace( idle_image, nullptr, nullptr );
        #endif
        if ( errno ) {
          #if defined( BOOT_ENABLE_OUTPUT )
            const int e = errno;
            EARLY_STARTUP_PRINT( "Unable to replace process with image\r\n" )
            EARLY_STARTUP_PRINT( "%s\r\n", strerror( e ) )
          #endif
          exit( -1 );
        }
      }

      // start /dev/nice
      #if defined( BOOT_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Forking process for nice start!\r\n" )
      #endif
      inner_forked_process = _syscall_process_fork();
      if ( errno ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Unable to fork process\r\n" )
          EARLY_STARTUP_PRINT( "%s\r\n", strerror( errno ) )
        #endif
        exit( -1 );
      }
      if ( 0 == inner_forked_process ) {
        // get idle image
        size_t nice_size;
        void* nice_image = ramdisk_lookup( disk, "ramdisk/bin/server/nice", &nice_size );
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Nice image: %p!\r\n", nice_image )
        #endif
        if ( ! nice_image ) {
          #if defined( BOOT_ENABLE_OUTPUT )
            EARLY_STARTUP_PRINT( "Nice not found!\r\n" )
          #endif
          exit( -1 );
        }
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Replacing fork with nice image %p!\r\n", nice_image );
        _syscall_process_replace( nice_image, nullptr, nullptr );
        #endif
        if ( errno ) {
          #if defined( BOOT_ENABLE_OUTPUT )
            const int e = errno;
            EARLY_STARTUP_PRINT( "Unable to replace process with image\r\n" )
            EARLY_STARTUP_PRINT( "%s\r\n", strerror( e ) )
          #endif
          exit( -1 );
        }
      }

      // start /dev/ramdisk
      void* ramdisk_image = ramdisk_lookup( disk, "ramdisk/bin/server/fs/ramdisk", nullptr );
      if ( ! ramdisk_image ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "ramdisk server not found!\r\n" )
        #endif
        exit( -1 );
      }
      // fork process and handle possible error
      #if defined( BOOT_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Forking process for ramdisk start!\r\n" )
      #endif
      inner_forked_process = _syscall_process_fork();
      if ( errno ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Unable to fork process\r\n" )
          EARLY_STARTUP_PRINT( "%s\r\n", strerror( errno ) )
        #endif
        exit( -1 );
      }
      if ( 0 == inner_forked_process ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Replacing fork with ramdisk image %p!\r\n", ramdisk_image )
        #endif
        // build command
        char* ramdisk_cmd[] = { "ramdisk", shm_id_str, nullptr, };
        // call for replace and handle error
        _syscall_process_replace( ramdisk_image, ramdisk_cmd, nullptr );
        if ( errno ) {
          #if defined( BOOT_ENABLE_OUTPUT )
            EARLY_STARTUP_PRINT( "Unable to replace process with image\r\n" )
            EARLY_STARTUP_PRINT( "%s\r\n", strerror( errno ) )
          #endif
          exit( -1 );
        }
      }

      // start /dev/authentication
      void* authentication_image = ramdisk_lookup(
        disk,
        "ramdisk/bin/server/authentication",
        nullptr
      );
      if ( ! authentication_image ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "authentication server not found!\r\n" )
        #endif
        exit( -1 );
      }
      // fork process and handle possible error
      #if defined( BOOT_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Forking process for authenticate start!\r\n" )
      #endif
      inner_forked_process = _syscall_process_fork();
      if ( errno ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Unable to fork process\r\n" )
          EARLY_STARTUP_PRINT( "%s\r\n", strerror( errno ) )
        #endif
        exit( -1 );
      }
      if ( 0 == inner_forked_process ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Replacing fork with authentication image %p!\r\n", authentication_image )
        #endif
        // build command
        char* authentication_cmd[] = { "authentication", "1", "2", "3", "4", "5", "6", "7", nullptr, };
        // call for replace and handle error
        _syscall_process_replace( authentication_image, authentication_cmd, nullptr );
        if ( errno ) {
          #if defined( BOOT_ENABLE_OUTPUT )
            EARLY_STARTUP_PRINT( "Unable to replace process with image\r\n" )
            EARLY_STARTUP_PRINT( "%s\r\n", strerror( errno ) )
          #endif
          exit( -1 );
        }
      }

      if ( 0 != inner_forked_process ) {
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "waiting for parent to be ready!\r\n" )
        #endif
        // wait for parent process to be ready
        _syscall_rpc_wait_for_ready( forked_process );
        #if defined( BOOT_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Replacing fork with dev image %p!\r\n", dev_image )
        #endif
        // build command
        char* dev_cmd[] = { "dev", nullptr, };
        // call for replace and handle error
        _syscall_process_replace( dev_image, dev_cmd, nullptr );
        if ( errno ) {
          #if defined( BOOT_ENABLE_OUTPUT )
            EARLY_STARTUP_PRINT( "Unable to replace process with image\r\n" )
            EARLY_STARTUP_PRINT( "%s\r\n", strerror( errno ) )
          #endif
          exit( -1 );
        }
      }
    }
  }
  // handle unexpected vfs id returned
  if ( VFS_DAEMON_ID != forked_process ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "Invalid process id for vfs daemon!\r\n" )
    #endif
    exit( -1 );
  }
  // enable rpc and wait for process to be ready
  #if defined( BOOT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "set rpc ready!\r\n" )
  #endif
  _syscall_rpc_set_ready( true );
  // wait for vfs to be ready
  #if defined( BOOT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "wait for vfs becomes ready!\r\n" )
  #endif
  _syscall_rpc_wait_for_ready( VFS_DAEMON_ID );
  // wait for vfs to be ready
  #if defined( BOOT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "waiting for vfs!\r\n" )
  #endif
  vfs_wait_for_path( ":/vfs", true );
  // enough to wait here for ramdisk and authentication, since both need dev server
  #if defined( BOOT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Waiting for authentication device\r\n" )
  #endif
  vfs_wait_for_path( AUTHENTICATION_DEVICE, true );
  #if defined( BOOT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Waiting for ramdisk\r\n" )
  #endif
  vfs_wait_for_path( "/dev/ramdisk", true );
  #if defined( BOOT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Waiting for idle\r\n" )
  #endif
  vfs_wait_for_path( "/dev/idle", true );
  #if defined( BOOT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Waiting for nice\r\n" )
  #endif
  vfs_wait_for_path( "/dev/nice", true );
  assert( idle_pid == stat_handler( "/dev/idle" ) );
  assert( ramdisk_pid == stat_handler( "/dev/ramdisk" ) );
  assert( authentication_pid == stat_handler( "/dev/authentication" ) );
  assert( nice_pid == stat_handler( NICE_DEVICE ) );
  // close ramdisk
  #if defined( BOOT_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "Closing early ramdisk\r\n" )
  #endif
  if ( 0 != tar_close( disk ) ) {
    #if defined( BOOT_ENABLE_OUTPUT )
      EARLY_STARTUP_PRINT( "ERROR: Cannot close ramdisk!\r\n" )
    #endif
    exit( -1 );
  }
  // unset disk
  disk = nullptr;
  free( shm_id_str );
}
