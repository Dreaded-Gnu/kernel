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
#include "../../../../arch.h"
#include "../../../../server.h"
#include "../../../../syscall.h"
#if defined( PRINT_SYSCALL )
  #include "../../../../debug/debug.h"
  #include "../../../../lib/inttypes.h"
#endif

/**
 * @brief void syscall_process_halt( void* )
 * @brief Process halt syscall
 * @param context
 */
void syscall_process_halt( void* context ) {
  // only idle daemon is allowed to halt
  if ( task_thread_current_thread->process->id != SERVER_IDLE_DAEMON_ID ) {
    syscall_populate_error( context, EINVAL );
  }
  // call arch idle
  arch_halt();
  // return success
  syscall_populate_success( context, 0 );
}
