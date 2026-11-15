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

#include <inttypes.h>
#include "../../keyboard.h"
#include "../../rpc.h"
#include "../../timer.h"
#include "../../keymap.h"

/**
 * @fn void rpc_default_timer(size_t, pid_t, uint64_t, uint64_t)
 * @brief Default handler for timer
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_default_timer(
  [[maybe_unused]] size_t type,
  [[maybe_unused]] pid_t origin,
  [[maybe_unused]] uint64_t data_info,
  const uint64_t response_info
) {
  // clear timer if set
  if ( response_info ) {
    _syscall_timer_release( response_info );
  }
  // try to find entry with repeat
  auto entry = keyboard_head;
  if ( ! entry ) {
    return;
  }
  while ( entry ) {
    // handle not active => skip
    if ( entry->repeat.timer != response_info ) {
      entry = entry->next;
      continue;
    }
    // handle not yet repeating
    if ( ! entry->repeat.repeating ) {
      // emit key
      keyboard_emit_key( entry, entry->repeat.key, 0, 0 );
      // set repeating
      entry->repeat.repeating = true;
      entry->repeat.timer = timer_acquire( KEYBOARD_REPEAT_REPEAT_DEADLINE_MILLISECONDS );
      // skip rest
      entry = entry->next;
      continue;
    }
    // emit key
    keyboard_emit_key( entry, entry->repeat.key, 0, 0 );
    // increase repeat
    entry->repeat.timer = timer_acquire( KEYBOARD_REPEAT_REPEAT_DEADLINE_MILLISECONDS );
    // switch to next
    entry = entry->next;
  }
}
