
/**
 * Copyright (C) 2018 - 2020 bolthur project.
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

#include <core/interrupt.h>
#include <core/syscall.h>

/**
 * @brief Initialize syscalls
 */
void syscall_init( void ) {
  // process system calls
  interrupt_register_handler(
    SYSCALL_PROCESS_CREATE,
    syscall_process_create,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_PROCESS_EXIT,
    syscall_process_exit,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_PROCESS_ID,
    syscall_process_id,
    INTERRUPT_SOFTWARE,
    false
  );
  // thread related system calls
  interrupt_register_handler(
    SYSCALL_THREAD_CREATE,
    syscall_thread_create,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_THREAD_EXIT,
    syscall_thread_exit,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_THREAD_ID,
    syscall_thread_id,
    INTERRUPT_SOFTWARE,
    false
  );
  // memory related
  interrupt_register_handler(
    SYSCALL_MEMORY_ACQUIRE,
    syscall_memory_acquire,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_MEMORY_RELEASE,
    syscall_memory_release,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_SHARED_MEMORY_ACQUIRE,
    syscall_shared_memory_acquire,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_SHARED_MEMORY_CREATE,
    syscall_shared_memory_create,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_SHARED_MEMORY_EXTEND,
    syscall_shared_memory_extend,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_SHARED_MEMORY_RELEASE,
    syscall_shared_memory_release,
    INTERRUPT_SOFTWARE,
    false
  );
  // interrupt related
  interrupt_register_handler(
    SYSCALL_INTERRUPT_ACQUIRE,
    syscall_interrupt_acquire,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_INTERRUPT_RELEASE,
    syscall_interrupt_release,
    INTERRUPT_SOFTWARE,
    false
  );
  // io related
  interrupt_register_handler(
    SYSCALL_IO_ACQUIRE,
    syscall_io_acquire,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_IO_CHECK,
    syscall_io_check,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_IO_RAISE,
    syscall_io_raise,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_IO_RELEASE,
    syscall_io_release,
    INTERRUPT_SOFTWARE,
    false
  );
  // message related
  interrupt_register_handler(
    SYSCALL_MESSAGE_ACQUIRE,
    syscall_message_acquire,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_MESSAGE_RELEASE,
    syscall_message_release,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_MESSAGE_SYNC_SEND,
    syscall_message_sync_send,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_MESSAGE_SYNC_RECEIVE,
    syscall_message_sync_receive,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_MESSAGE_ASYNC_SEND,
    syscall_message_async_send,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_MESSAGE_ASYNC_RECEIVE,
    syscall_message_async_receive,
    INTERRUPT_SOFTWARE,
    false
  );
  // dummy syscalls
  interrupt_register_handler(
    SYSCALL_DUMMY_PUTC,
    syscall_dummy_putc,
    INTERRUPT_SOFTWARE,
    false
  );
  interrupt_register_handler(
    SYSCALL_DUMMY_PUTS,
    syscall_dummy_puts,
    INTERRUPT_SOFTWARE,
    false
  );
}
