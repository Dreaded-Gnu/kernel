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

#include "../../../../lib/string.h"
#include "../../../../lib/inttypes.h"
#include "../cpu.h"
#include "../../../../mm/virt.h"
#include "../../../../rpc/backup.h"
#include "../../../../cpu/pool.h"
#include "../../../../rpc/data.h"
#include "../../../../rpc/pool.h"
#if defined( PRINT_RPC )
  #include "../../../../debug/debug.h"
#endif

/**
 * @fn rpc_backup_t* rpc_backup_create(task_thread_t*, const task_process_t*, size_t, const void*, size_t, task_thread_t*, bool, size_t, bool, bool, bool, bool)
 * @brief Helper to create rpc backup
 * @param source
 * @param target
 * @param type
 * @param data
 * @param data_size
 * @param target_thread
 * @param sync
 * @param origin_data_id
 * @param disable_data
 * @param is_interrupt
 * @param is_timer
 * @param measure
 * @return
 */
rpc_backup_t* rpc_backup_create(
  task_thread_t* source,
  const task_process_t* target,
  const size_t type,
  const void* data,
  const size_t data_size,
  task_thread_t* target_thread,
  const bool sync,
  const size_t origin_data_id,
  const bool disable_data,
  const bool is_interrupt,
  const bool is_timer,
  const bool measure
) {
  const uint64_t t_before_thread_look_up = timer_get_current_tick_value();
  // try to use target thread
  task_thread_t* thread = target_thread;
  // choose one from free thread list
  if ( ! thread ) {
    auto current = target->thread_list->first;
    // loop until usable thread has been found
    while ( current && ! thread ) {
      // get thread
      auto const tmp = ( task_thread_t* )current->data;
      // FIXME: CHECK IF ACTIVE
      thread = tmp;
      // get next thread
      current = current->next;
    }
  }
  const uint64_t t_after_thread_look_up = timer_get_current_tick_value();
  // handle no inactive thread
  if ( ! thread ) {
    return nullptr;
  }
  // ensure correct state
  if ( ! thread->process->rpc_ready ) {
    // debug output
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "thread not ready %d!\r\n", thread->state )
    #endif
    return nullptr;
  }

  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "thread->id = %d\r\n", thread->id )
    DEBUG_OUTPUT( "thread->process->id = %d\r\n", thread->process->id )
  #endif

  const uint64_t t_before_rpc_pool_pop = timer_get_current_tick_value();
  // reserve space for backup object
  rpc_backup_t* backup = rpc_pool_pop();
  if ( ! backup ) {
    #if defined( PRINT_RPC )
      DEBUG_OUTPUT( "Unable to reserve memory for backup structure!\r\n" )
    #endif
    return nullptr;
  }
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Reserved backup object: %p\r\n", backup )
  #endif
  const uint64_t t_after_rpc_pool_pop = timer_get_current_tick_value();

  // get thread cpu context
  const cpu_register_context_t* cpu = thread->current_context;
  if ( thread->current_active_backup ) {
    cpu = thread->current_active_backup->context;
  }
  const uint64_t t_before_cpu_pool_pop = timer_get_current_tick_value();
  // reserve space for backup context
  backup->context = cpu_pool_pop();
  if ( ! backup->context ) {
    rpc_backup_destroy( backup );
    return nullptr;
  }
  const uint64_t t_after_cpu_pool_pop = timer_get_current_tick_value();
  // load cpu into cache before copying it
  const uint64_t t_before_pure_cpu_to_cache = timer_get_current_tick_value();
  auto const ptr = ( const uint8_t* )cpu->raw;
  constexpr size_t total_size = sizeof( uint32_t ) * CPU_CONTEXT_WORD_SIZE;
  constexpr size_t cache_size = 32;
  for ( size_t offset = 0; offset < total_size; offset += cache_size ) {
    __builtin_prefetch( ptr + offset, 0, 3 );
  }
  const uint64_t t_after_pure_cpu_to_cache = timer_get_current_tick_value();
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Reserved backup cpu context: %p\r\n", backup->context )
  #endif
  const uint64_t t_before_cpu_copy = timer_get_current_tick_value();
  // prepare and backup context area
  memcpy( backup->context, cpu, sizeof( cpu_register_context_t ) );
  // debug output
  #if defined( PRINT_RPC )
    DUMP_REGISTER( backup->context )
  #endif
  const uint64_t t_after_cpu_copy = timer_get_current_tick_value();
  // backup parameter data as message
  const uint64_t t_before_data_queue = timer_get_current_tick_value();
  backup->data_id = 0;
  if ( ! disable_data ) {
    if ( data && data_size ) {
      const int err = rpc_data_queue_add(
        thread->process->id,
        data,
        data_size,
        &backup->data_id
      );
      if ( err ) {
        // debug output
        #if defined( PRINT_RPC )
          DEBUG_OUTPUT( "Adding to queue failed with code %d\r\n", err )
        #endif
        rpc_backup_destroy( backup );
        return nullptr;
      }
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT(
          "Sent message from process %d to %d\r\n",
          source->process->id,
          thread->process->id
        )
      #endif
    } else {
      constexpr char dummy = '\0';
      const int err = rpc_data_queue_add(
        thread->process->id,
        &dummy,
        sizeof( char ),
        &backup->data_id
      );
      if ( err ) {
        // debug output
        #if defined( PRINT_RPC )
          DEBUG_OUTPUT( "Adding to queue failed with code %d\r\n", err )
        #endif
        rpc_backup_destroy( backup );
        return nullptr;
      }
      // debug output
      #if defined( PRINT_RPC )
        DEBUG_OUTPUT(
          "Sent dummy message from process %d to %d\r\n",
          source->process->id,
          thread->process->id
        )
        DEBUG_OUTPUT( "type = %zu, data_id = %zu\r\n", type, backup->data_id )
      #endif
    }
  }
  const uint64_t t_after_data_queue = timer_get_current_tick_value();
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "async: %d, type: %zu\r\n", sync ? 0 : 1, type )
  #endif
  // populate remaining values
  backup->thread = thread;
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "pid: %d, backup->thread_state = %d, thread->state = %d\r\n",
      thread->process->id, backup->thread_state, thread->state )
  #endif
  // save thread state and state data
  const uint64_t t_before_thread_backup = timer_get_current_tick_value();
  backup->thread_state = thread->state;
  backup->thread_state_data.data_ptr = thread->state_data.data_ptr;
  backup->thread_state_data.data_size = thread->state_data.data_size;
  // in case thread state is rpc wait for call we need to go back to active
  // after rpc, because it may be a sleep that is active
  if ( TASK_THREAD_STATE_RPC_WAIT_FOR_CALL == backup->thread_state ) {
    backup->thread_state = TASK_THREAD_STATE_ACTIVE;
  }
  const uint64_t t_after_thread_backup = timer_get_current_tick_value();
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT(
      "backup->thread_state = %d, backup->thread_state_data.data_ptr = %p\r\n",
      backup->thread_state,
      backup->thread_state_data.data_ptr
    )
  #endif
  const uint64_t t_before_backup_fill = timer_get_current_tick_value();
  backup->prepared = false;
  backup->source = source;
  backup->type = type;
  backup->sync = sync;
  backup->origin_data_id = origin_data_id;
  backup->sync_return_data_id = 0;
  backup->sync_return_blocked_data_id = 0;
  backup->sync_return_on_end = false;
  backup->is_interrupt = is_interrupt;
  backup->is_timer = is_timer;
  backup->state_to_use = TASK_THREAD_STATE_RPC_QUEUED;
  backup->active = false;
  backup->list_item = nullptr;
  backup->squeezed_in = false;
  const uint64_t t_after_backup_fill = timer_get_current_tick_value();
  // debug output
  #if defined( PRINT_RPC )
    DEBUG_OUTPUT( "Pushing backup object to rpc queue!\r\n" )
  #endif
  // push back backup to queue
  const uint64_t t_before_push_back_data = timer_get_current_tick_value();
  list_item_t* item = list_push_back_data( thread->process->rpc_queue, backup );
  if ( ! item ) {
    rpc_backup_destroy( backup );
    return nullptr;
  }
  // cache in backup
  backup->list_item = item;
  const uint64_t t_after_push_back_data = timer_get_current_tick_value();
  if ( measure ) {
    DEBUG_OUTPUT( "t_after_thread_look_up - t_before_thread_look_up = %"PRIu64"\r\n", t_after_thread_look_up - t_before_thread_look_up )
    DEBUG_OUTPUT( "t_after_rpc_pool_pop - t_before_rpc_pool_pop = %"PRIu64"\r\n", t_after_rpc_pool_pop - t_before_rpc_pool_pop )
    DEBUG_OUTPUT( "t_after_cpu_pool_pop - t_before_cpu_pool_pop = %"PRIu64"\r\n", t_after_cpu_pool_pop - t_before_cpu_pool_pop )
    DEBUG_OUTPUT( "t_after_pure_cpu_to_cache - t_before_pure_cpu_to_cache = %"PRIu64"\r\n", t_after_pure_cpu_to_cache - t_before_pure_cpu_to_cache )
    DEBUG_OUTPUT( "t_after_cpu_copy - t_before_cpu_copy = %"PRIu64"\r\n", t_after_cpu_copy - t_before_cpu_copy )
    DEBUG_OUTPUT( "t_after_data_queue - t_before_data_queue = %"PRIu64"\r\n", t_after_data_queue - t_before_data_queue )
    DEBUG_OUTPUT( "t_after_thread_backup - t_before_thread_backup = %"PRIu64"\r\n", t_after_thread_backup - t_before_thread_backup )
    DEBUG_OUTPUT( "t_after_backup_fill - t_before_backup_fill = %"PRIu64"\r\n", t_after_backup_fill - t_before_backup_fill )
    DEBUG_OUTPUT( "t_after_push_back_data - t_before_push_back_data = %"PRIu64"\r\n", t_after_push_back_data - t_before_push_back_data )
  }
  // return created backup
  return backup;
}
