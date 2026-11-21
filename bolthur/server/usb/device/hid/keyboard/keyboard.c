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
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/bolthur.h>
#include <sys/ioctl.h>
#include "keyboard.h"

#include "console.h"
#include "global.h"
#include "keymap.h"
#include "rpc.h"
#include "timer.h"
#include "../../../../libconsole.h"
#include "../../../../../library/hid/hid.h"
#include "../../../../../library/usb/usb.h"
#include "../../../../../library/util/max.h"
#include "../../../../../library/util/min.h"

/**
 * @brief Head of keyboard device list
 */
libusb_keyboard_device_t* keyboard_head = nullptr;

/**
 * @fn void keyboard_append(libusb_keyboard_device_t*)
 * @brief Append keyboard to handled list
 * @param keyboard
 */
void keyboard_append( libusb_keyboard_device_t* keyboard ) {
  // loop to last one
  libusb_keyboard_device_t* current = keyboard_head;
  libusb_keyboard_device_t* found = nullptr;
  while ( current ) {
    found = current;
    current = current->next;
  }
  // handle empty
  if ( ! found ) {
    keyboard_head = keyboard;
    keyboard->prev = nullptr;
    keyboard->next = nullptr;
    return;
  }
  // attach to list
  found->next = keyboard;
  keyboard->prev = found;
}

/**
 * @fn void keyboard_detach(const libusb_keyboard_device_t*)
 * @brief Removes keyboard from list
 * @param keyboard
 */
void keyboard_detach( const libusb_keyboard_device_t* keyboard ) {
  if ( keyboard->prev ) {
    keyboard->prev->next = keyboard->next;
  }
  if ( keyboard->next ) {
    keyboard->next->prev = keyboard->prev;
  }
  if ( keyboard == keyboard_head ) {
    keyboard_head = keyboard->next;
  }
}

/**
 * @fn void keyboard_destroy(libusb_keyboard_device_t*)
 * @brief Method to destroy keyboard device
 * @param device
 */
void keyboard_destroy( libusb_keyboard_device_t* device ) {
  // handle no device
  if ( ! device ) {
    return;
  }
  // remove from list
  if ( device->prev || device->next ) {
    keyboard_detach( device );
  }
  // free key report field
  if ( device->key_report ) {
    hid_destroy_report( device->key_report );
  }
  // free led report field
  if ( device->led_report ) {
    hid_destroy_report( device->led_report );
  }
  // free keymap area if set
  if ( device->keymap ) {
    free( device->keymap );
  }
  // free device itself
  free( device );
}

/**
 * @fn int keyboard_new_index(uint32_t*);
 * @brief Function to get new index
 * @param idx
 * @return
 */
int keyboard_new_index( uint32_t* idx ) {
  // validate parameters
  if ( ! idx ) {
    return EINVAL;
  }
  // initialize max idx
  uint32_t max_index = 0;
  // loop through list and collect max idx
  const libusb_keyboard_device_t* current = keyboard_head;
  while ( current ) {
    // determine max idx
    max_index = uint32_max( max_index, current->index );
    // go to next
    current = current->next;
  }
  // populate idx
  *idx = max_index;
  // return success
  return 0;
}

/**
 * @fn int keyboard_duplicate_report(libusb_hid_parser_report_t**, const libusb_hid_parser_report_t*, size_t)
 * @brief Method to duplicate report
 * @param destination
 * @param source
 * @param size
 * @return
 */
int keyboard_duplicate_report(
  libusb_hid_parser_report_t** destination,
  const libusb_hid_parser_report_t* source,
  const size_t size
) {
  // allocate space
  *destination = malloc( size );
  // handle error
  if ( ! *destination ) {
    return ENOMEM;
  }
  // copy over content
  memcpy( *destination, source, size );
  // duplicate report addresses
  for (size_t i = 0; i < source->fields_length; i++ ) {
    // skip variables or no ptr set
    if (
      source->fields[ i ].attribute.variable
      || ! source->fields[ i ].value.ptr
    ) {
      continue;
    }
    // cache ptr
    const uint8_t* ptr = source->fields[ i ].value.ptr;
    const size_t ptr_size = ( size_t )( source->fields[ i ].size * source->fields[ i ].count / 8 );
    void* new_ptr = malloc( ptr_size );
    // handle allocation error
    if ( ! new_ptr ) {
      // free up allocated stuff
      for ( size_t inner = 0; inner < i; inner++ ) {
        // skip variables or when no ptr is set
        if (
          source->fields[ inner ].attribute.variable
          || ! source->fields[ inner ].value.ptr
        ) {
          continue;
        }
        // free space
        free( (*destination)->fields[ inner ].value.ptr );
      }
      return ENOMEM;
    }
    // copy over stuff
    memcpy( new_ptr, ptr, ptr_size );
    // overwrite ptr
    (*destination)->fields[ i ].value.ptr = new_ptr;
  }
  // return success
  return 0;
}

/**
 * @fn int keyboard_start_polling(libusb_keyboard_device_t*)
 * @brief Function to start keyboard polling
 * @param device
 * @return
 */
int keyboard_start_polling( libusb_keyboard_device_t* device ) {
  // handle invalid parameter
  if ( ! device ) {
    return EINVAL;
  }
  // clear out buffer
  memset( device->buffer, 0, KEYBOARD_REPORT_SIZE );
  // return result of async control message
  const int result = usb_interrupt_poll_async(
    device->device_number,
    device->descriptor.attributes.transfer,
    device->descriptor.endpoint_address.number,
    LIBUSB_DIRECTION_IN,
    device->buffer,
    KEYBOARD_REPORT_SIZE,
    device->descriptor.interval,
    device->descriptor.interval
  );
  // handle error
  #if defined( KEYBOARD_ENABLE_OUTPUT )
    if ( 0 != result ) {
      const int e = errno;
      EARLY_STARTUP_PRINT( "ERROR: %s\r\n", strerror( e ) );
    }
  #endif
  return result;
}

/**
 * @fn libusb_keyboard_device_t* keyboard_get_device(const uint32_t)
 * @brief Method to get device by number
 * @param device_number
 * @return
 */
libusb_keyboard_device_t* keyboard_get_device(const uint32_t device_number) {
  // start with head
  libusb_keyboard_device_t* current = keyboard_head;
  // loop while there is something
  while ( current ) {
    // handle device number match
    if ( current->device_number == device_number ) {
      // return current
      return current;
    }
    // switch to next
    current = current->next;
  }
  // return nullptr
  return nullptr;
}

/**
 * @fn void keyboard_bit_set(uint8_t*, const uint32_t, const uint32_t, const uint32_t)
 * @brief
 * @param buffer
 * @param offset
 * @param length
 * @param value
 */
void keyboard_bit_set(
  uint8_t* buffer,
  const uint32_t offset,
  const uint32_t length,
  const uint32_t value
) {
  for ( size_t i = offset / 8, j = 0; i < ( offset + length + 7 ) / 8; i++ ) {
    if ( offset / 8 == ( offset + length - 1 ) / 8 ) {
      const uint32_t mask = ( uint32_t )( ( 1 << ( offset % 8 + length ) ) - ( 1 << ( offset % 8 ) ) );
      buffer[ i ] = ( uint8_t )( ( buffer[ i ] & ~mask ) | ( ( value << ( offset % 8 ) ) & mask ) );
    } else if ( i == offset / 8 ) {
      const uint32_t mask = ( uint32_t )( 0x100 - ( 1 << ( offset % 8 ) ) );
      buffer[ i ] = ( uint8_t )( ( buffer[ i ] & ~mask ) | ( ( value << ( offset % 8 ) ) & mask ) );
      j += 8 - ( offset % 8 );
    } else if ( i == ( offset + length - 1 ) / 8 ) {
      const uint32_t mask = ( uint32_t )( ( 1 << ( ( offset % 8 ) + length ) ) - 1 );
      buffer[ i ] = ( uint8_t )( ( buffer[ i ] & ~mask ) | ( ( value >> j ) & mask ) );
    } else {
      buffer[ i ] = (uint8_t)(( value >> j ) & 0xff);
      j += 8;
    }
  }
}

/**
 * @fn int32_t keyboard_bit_get_signed(const uint8_t*, const uint32_t, const uint32_t)
 * @brief
 * @param buffer
 * @param offset
 * @param length
 * @return
 */
int32_t keyboard_bit_get_signed( const uint8_t* buffer, const uint32_t offset, const uint32_t length ) {
  uint32_t result = keyboard_bit_get_unsigned( buffer, offset, length );
  if (result & 1 << (length - 1)) {
    result |= 0xffffffff - ( uint32_t )( ( 1 << length ) - 1 );
  }
  return ( int32_t )result;
}

/**
 * @fn uint32_t keyboard_bit_get_unsigned(const uint8_t*, const uint32_t, const uint32_t)
 * @brief
 * @param buffer
 * @param offset
 * @param length
 * @return
 */
uint32_t keyboard_bit_get_unsigned( const uint8_t* buffer, const uint32_t offset, const uint32_t length ) {
  uint32_t result = 0;
  for ( size_t i = offset / 8, j = 0; i < (offset + length + 7) / 8; i++) {
    if ( offset / 8 == ( offset + length - 1 ) / 8 ) {
      const uint32_t mask = ( uint32_t )( ( 1 << ( ( offset % 8 ) + length ) ) - ( 1 << ( offset % 8 ) ) );
      result = ( buffer[ i ] & mask ) >> ( offset % 8 );
    } else if ( i == offset / 8 ) {
      const uint32_t mask = ( uint32_t )( 0x100 - ( 1 << ( offset % 8 ) ) );
      j += 8 - (offset % 8);
      result = ( ( buffer[ i ] & mask ) >> ( offset % 8 ) ) << ( length - j );
    } else if ( i == ( offset + length - 1 ) / 8 ) {
      const uint32_t mask = ( uint32_t )( ( 1 << ( offset % 8 + length ) ) - 1 );
      result |= buffer[ i ] & mask;
    } else {
      j += 8;
      result |= ( uint32_t )( buffer[ i ] << ( length - j ) );
    }
  }
  return result;
}

/**
 * @fn int32_t keyboard_bit_get_value(const libusb_hid_parser_fields_t*, const uint32_t)
 * @brief
 * @param field
 * @param idx
 * @return
 */
int32_t keyboard_bit_get_value( const libusb_hid_parser_fields_t* field, const uint32_t idx ) {
  return keyboard_bit_get_signed(
    field->value.ptr, idx * field->size, field->size );
}

/**
 * @fn int keyboard_set_led(libusb_keyboard_device_t*, libusb_keyboard_led_t*)
 * @brief set keyboard leds
 * @param dev
 * @param led
 * @return
 */
int keyboard_set_led( const libusb_keyboard_device_t* dev, const libusb_keyboard_led_t* led ) {
  // set led fields depending on support
  if ( dev->led.num_lock ) {
    dev->led_field[ 0 ]->value._bool = led->num_lock;
  }
  if ( dev->led.caps_lock ) {
    dev->led_field[ 1 ]->value._bool = led->caps_lock;
  }
  if ( dev->led.scroll_lock ) {
    dev->led_field[ 2 ]->value._bool = led->scroll_lock;
  }
  if ( dev->led.compose ) {
    dev->led_field[ 3 ]->value._bool = led->compose;
  }
  if ( dev->led.kana ) {
    dev->led_field[ 4 ]->value._bool = led->kana;
  }
  if ( dev->led.power ) {
    dev->led_field[ 5 ]->value._bool = led->power;
  }
  if ( dev->led.shift ) {
    dev->led_field[ 6 ]->value._bool = led->shift;
  }
  if ( dev->led.mute ) {
    dev->led_field[ 7 ]->value._bool = led->mute;
  }
  // populate buffer
  memset( dev->buffer, 0, KEYBOARD_REPORT_SIZE );
  for ( size_t i = 0 ; i < dev->led_report->field_count ; i++ ) {
    auto const field = &dev->led_report->fields[ i ];
    if ( field->attribute.variable ) {
      keyboard_bit_set(
        dev->buffer,
        field->offset,
        field->size,
        field->value.u32
      );
    } else {
      for (size_t j = 0; j < field->count; j++) {
        keyboard_bit_set(
          dev->buffer,
          field->offset,
          field->size,
          (uint32_t)keyboard_bit_get_signed(
            field->value.ptr,
            j * field->size,
            field->size
          )
        );
      }
    }
  }
  // return set report
  return hid_set_report(
    dev->device_number,
    ( uint8_t )dev->led_report->type,
    dev->led_report->id,
    dev->buffer,
    dev->led_report->report_length
  );
}

/**
 * @fn bool keyboard_contains_key(const uint16_t*, uint16_t, size_t)
 * @brief Helper to check if array contains key
 * @param keys
 * @param key
 * @param max
 * @return
 */
bool keyboard_contains_key( const uint16_t* keys, const uint16_t key, const size_t max ) {
  for ( size_t i = 0; i < max; i++ ) {
    if ( keys[i] == key ) {
      return true;
    }
  }
  return false;
}

/**
 * @fn void keyboard_emit_key(const libusb_keyboard_device_t* uint16_t, pid_t, uint64_t)
 * @brief Emit keyboard key
 * @param dev
 * @param key
 * @param origin
 * @param data_info
 */
void keyboard_emit_key(
  const libusb_keyboard_device_t* dev,
  const uint16_t key,
  const pid_t origin,
  const uint64_t data_info
) {
  // translate key code
  uint16_t translated;
  if ( 0 != keymap_translate( key, dev, &translated ) ) {
    return;
  }
  // handle nothing
  if ( key == KEYMAP_SPECIAL_KEY_NONE ) {
    return;
  }
  // cache key map
  keymap_t* keymap = dev->keymap;
  char tmp_buffer[ 10 ] = { 0, };
  // debug print physical key and key code
  #if defined ( KEYBOARD_ENABLE_OUTPUT )
    EARLY_STARTUP_PRINT( "key: %02"PRIx16" / %02"PRIx16" / %c\r\n",
      key, translated, ( uint8_t )translated );
  #endif
  bool set_led = false;
  switch ( translated ) {
    case KEYMAP_SPECIAL_KEY_CAPS_LOCK:
      keymap->caps_lock = !keymap->caps_lock;
      if ( dev->led.caps_lock ) {
        dev->led_field[ 1 ]->value._bool = keymap->caps_lock;
      }
      set_led = true;
      break;
    case KEYMAP_SPECIAL_KEY_NUM_LOCK:
      keymap->num_lock = !keymap->num_lock;
      if ( dev->led.num_lock ) {
        dev->led_field[ 0 ]->value._bool = keymap->num_lock;
      }
      set_led = true;
      break;
    case KEYMAP_SPECIAL_KEY_SCROLL_LOCK:
      keymap->scroll_lock = !keymap->scroll_lock;
      if ( dev->led.scroll_lock ) {
        dev->led_field[ 2 ]->value._bool = keymap->scroll_lock;
      }
      set_led = true;
      break;
    default:
      // clear buffer and translate to string
      memset( tmp_buffer, 0, sizeof( tmp_buffer ) );
      if ( 0 != keymap_to_string( translated, dev, tmp_buffer ) ) {
        return;
      }
  }
  // handle set led
  if ( set_led ) {
    // set led
    [[maybe_unused]] const int result = keyboard_set_led( dev, &(const libusb_keyboard_led_t){
      .num_lock = dev->led.num_lock ? dev->led_field[ 0 ]->value._bool : false,
      .caps_lock = dev->led.caps_lock ? dev->led_field[ 1 ]->value._bool : false,
      .scroll_lock = dev->led.scroll_lock ? dev->led_field[ 2 ]->value._bool : false,
      .compose = dev->led.compose ? dev->led_field[ 3 ]->value._bool : false,
      .kana = dev->led.kana ? dev->led_field[ 4 ]->value._bool : false,
      .power = dev->led.power ? dev->led_field[ 5 ]->value._bool : false,
      .shift = dev->led.shift ? dev->led_field[ 6 ]->value._bool : false,
      .mute = dev->led.mute ? dev->led_field[ 7 ]->value._bool : false,
    });
    // check result
    #if defined( KEYBOARD_ENABLE_OUTPUT )
      if ( 0 != result ) {
        EARLY_STARTUP_PRINT( "Unable to set led: %s\r\n", strerror( result ) );
      }
    #endif
    // skip rest
    return;
  }
  // handle input buffer
  if ( strlen( tmp_buffer ) ) {
    // allocate input command
    console_command_input_t* input_command = malloc( sizeof( *input_command ) );
    if ( ! input_command ) {
      return;
    }
    // clear out input commend
    memset( input_command, 0, sizeof( *input_command ) );
    // copy over
    strncpy(
      input_command->input,
      tmp_buffer,
      size_min( strlen( tmp_buffer ) + 1, CONSOLE_MAX_INPUT_SEQUENCE - 1 )
    );
    // raise input request async
    // calculate rpc request size
    constexpr size_t rpc_request_size = sizeof( vfs_ioctl_perform_request_t )
      + sizeof( *input_command );
    // allocate rpc structures
    vfs_ioctl_perform_request_t* rpc_request = malloc( rpc_request_size );
    if ( ! rpc_request ) {
      free( input_command );
      _syscall_rpc_cleanup();
      return;
    }
    // clear rpc structures
    memset( rpc_request, 0, rpc_request_size );
    // populate structure
    rpc_request->handle = console_fd;
    rpc_request->command = CONSOLE_INPUT;
    rpc_request->type = IOCTL_RDWR;
    // copy over data
    memcpy( rpc_request->container, input_command, sizeof( *input_command ) );
    // raise rpc and wait for return
    const uint64_t response_id = bolthur_rpc_raise(
      RPC_VFS_IOCTL,
      VFS_DAEMON_ID,
      rpc_request,
      rpc_request_size,
      console_complete,
      RPC_VFS_IOCTL,
      rpc_request,
      rpc_request_size,
      origin,
      data_info,
      nullptr,
      false
    );
    // handle response issue
    if ( ! response_id ) {
      // debug output
      #if defined( KEYBOARD_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "Pushing input to console failed\r\n" )
      #endif
      free( rpc_request );
      free( input_command );
      _syscall_rpc_cleanup();
      return;
    }
    free( rpc_request );
    free( input_command );
  }
}

/**
 * @fn void keyboard_key_down(libusb_keyboard_device_t*, uint16_t, pid_t, uint64_t)
 * @brief Key down handler
 * @param dev
 * @param key
 * @param origin
 * @param data_info
 */
void keyboard_key_down(
  libusb_keyboard_device_t* dev,
  const uint16_t key,
  const pid_t origin,
  const uint64_t data_info
) {
  if ( dev->keyboard_state[ key ] ) {
    return;
  }
  // set key down state to true
  dev->keyboard_state[ key ] = true;
  // emit key
  keyboard_emit_key( dev, key, origin, data_info );
  // clear running timer if active
  if ( dev->repeat.active ) {
    _syscall_timer_release( dev->repeat.timer );
    dev->repeat.timer = 0;
  }
  // set repeat key
  dev->repeat.key = key;
  // change repeat to active and repeating to false
  dev->repeat.active = true;
  dev->repeat.repeating = false;
  // start initial timer
  dev->repeat.timer = timer_acquire( KEYBOARD_REPEAT_INITIAL_DEADLINE_MILLISECONDS );
}

/**
 * @fn void keyboard_key_up(libusb_keyboard_device_t*, uint16_t)
 * @brief Key up handler
 * @param dev
 * @param key
 */
void keyboard_key_up( libusb_keyboard_device_t* dev, const uint16_t key ) {
  if ( ! dev->keyboard_state[ key ] ) {
    return;
  }
  // set key down state to false
  dev->keyboard_state[ key ] = false;
  // handle repeat active for key
  if ( dev->repeat.active && dev->repeat.key == key ) {
    // reset timer if set
    if ( dev->repeat.timer ) {
      _syscall_timer_release( dev->repeat.timer );
      dev->repeat.timer = 0;
    }
    // reset active and repeating
    dev->repeat.active = false;
    dev->repeat.repeating = false;
  }
}
