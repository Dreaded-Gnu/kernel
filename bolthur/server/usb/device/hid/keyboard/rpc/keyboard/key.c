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
#include <inttypes.h>
#include "../../keymap.h"
#include "../../rpc.h"
#include "../../keyboard.h"
#include "../../console.h"
#include "../../../../../../libusbd.h"

/**
 * @fn void rpc_keyboard_key(size_t, pid_t, uint64_t, uint64_t)
 * @brief Key rpc handler callback
 * @param type
 * @param origin
 * @param data_info
 * @param response_info
 */
void rpc_keyboard_key(
  [[maybe_unused]] size_t type,
  const pid_t origin,
  const uint64_t data_info,
  [[maybe_unused]] uint64_t response_info
) {
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
  // get message
  auto const message = ( usbd_interrupt_return_t* )response->container;
  // try to get device by number
  libusb_keyboard_device_t* dev = keyboard_get_device( message->device_number );
  // handle no device found
  if ( ! dev ) {
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // handle error
  if ( message->error & LIBUSB_TRANSFER_ERROR_PROCESSING ) {
    // handle stall / toggle error by clearing stall bit
    if (
      message->error & LIBUSB_TRANSFER_ERROR_STALL
      || message->error & LIBUSB_TRANSFER_ERROR_DATA_TOGGLE
    ) {
      libusb_transfer_error_t error;
      uint32_t last_transfer;
      const int result = usb_control_message(
        message->device_number,
        LIBUSB_TRANSFER_CONTROL,
        LIBUSB_DIRECTION_OUT,
        nullptr,
        0,
        &( libusb_device_request_t ){
          .request = LIBUSB_DEVICE_REQUEST_CLEAR_FEATURE,
          .type = 0x02,
          .index = dev->descriptor.endpoint_address.number,
          .value = 0,
          .length = 0,
        },
        USB_TIMEOUT_VALUE,
        &error,
        &last_transfer
      );
      // handle error
      if ( 0 != result ) {
        #if defined( KEYBOARD_ENABLE_OUTPUT )
          EARLY_STARTUP_PRINT( "Unable to clear feature\r\n" )
        #endif
        free( response );
        _syscall_rpc_cleanup();
        return;
      }
      // restart polling
      keyboard_start_polling( dev );
    } else {
      #if defined( KEYBOARD_ENABLE_OUTPUT )
        EARLY_STARTUP_PRINT( "ERROR: %x\r\n", message->error );
      #endif
    }
    // cleanup everything and return
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // handle not enough transferred
  if ( message->length != KEYBOARD_REPORT_SIZE ) {
    free( response );
    _syscall_rpc_cleanup();
    return;
  }
  // copy over buffer
  memcpy( dev->buffer, message->buffer, KEYBOARD_REPORT_SIZE );
  // iterate through reports
  for ( size_t i = 0; i < dev->key_report->field_count; i++ ) {
    // get current field
    libusb_hid_parser_fields_t* field = &dev->key_report->fields[ i ];
    // handle variable
    if ( field->attribute.variable ) {
      // set value depending on minimum
      if ( field->logical_minimum < 0 ) {
        field->value.i32 = keyboard_bit_get_signed(
          dev->buffer, field->offset, field->size );
      } else {
        field->value.u32 = keyboard_bit_get_unsigned(
          dev->buffer, field->offset, field->size );
      }
      // skip rest
      continue;
    }
    // loop through field count
    for ( size_t j = 0; j < field->count; j++ ) {
      keyboard_bit_set(
        field->value.ptr,
        j * field->size,
        field->size,
        field->logical_minimum < 0
          ? ( uint32_t )keyboard_bit_get_signed(
            dev->buffer,
            field->offset + j * field->size,
            field->size
          ) : keyboard_bit_get_unsigned(
            dev->buffer,
            field->offset + j * field->size,
            field->size
          )
      );
    }
  }
  // set modifiers
  if ( dev->key_field[ 0 ] ) {
    dev->modifier.left_control = dev->key_field[ 0 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_OUTPUT )
      if ( dev->modifier.left_control ) {
        EARLY_STARTUP_PRINT( "Left control\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 1 ] ) {
    dev->modifier.left_shift = dev->key_field[ 1 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_OUTPUT )
      if ( dev->modifier.left_shift ) {
        EARLY_STARTUP_PRINT( "Left shift\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 2 ] ) {
    dev->modifier.left_alt = dev->key_field[ 2 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_OUTPUT )
      if ( dev->modifier.left_alt ) {
        EARLY_STARTUP_PRINT( "Left alt\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 3 ] ) {
    dev->modifier.left_gui = dev->key_field[ 3 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_OUTPUT )
      if ( dev->modifier.left_gui ) {
        EARLY_STARTUP_PRINT( "Left gui\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 4 ] ) {
    dev->modifier.right_control = dev->key_field[ 4 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_OUTPUT )
      if ( dev->modifier.right_control ) {
        EARLY_STARTUP_PRINT( "Right control\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 5 ] ) {
    dev->modifier.right_shift = dev->key_field[ 5 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_OUTPUT )
      if ( dev->modifier.right_shift ) {
        EARLY_STARTUP_PRINT( "Right shift\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 6 ] ) {
    dev->modifier.right_alt = dev->key_field[ 6 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_OUTPUT )
      if ( dev->modifier.right_alt ) {
        EARLY_STARTUP_PRINT( "Right alt\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 7 ] ) {
    dev->modifier.right_gui = dev->key_field[ 7 ]->value._bool;
    #if defined( KEYBOARD_ENABLE_OUTPUT )
      if ( dev->modifier.right_gui ) {
        EARLY_STARTUP_PRINT( "Right gui\r\n" )
      }
    #endif
  }
  if ( dev->key_field[ 8 ] ) {
    // get first value
    const int32_t val = keyboard_bit_get_value( dev->key_field[ 8 ], 0 );
    size_t max_key = KEYBOARD_MAX_KEYS > dev->key_field[ 8 ]->count
      ? dev->key_field[ 8 ]->count : KEYBOARD_MAX_KEYS;
    // handle no error
    if ( val != LIBUSB_HID_USAGE_PAGE_KEYBOARD_ERROR_ROLL_OVER ) {
      // iterate over max possible keys and detect key down
      for ( size_t i = 0; i < max_key; i++ ) {
        // extract whether key is down or not
        const uint16_t key_value = ( uint16_t )keyboard_bit_get_value( dev->key_field[ 8 ], i );
        // set to key value
        dev->current_key_down[ i ] = key_value;
        // handle invalid or already down detected
        if ( 0 != key_value && ! keyboard_contains_key( dev->previous_key_down, key_value, max_key ) ) {
          keyboard_key_down( dev, key_value, origin, data_info );
        }
      }
      // iterate over max possible keys and detect key up
      for ( size_t i = 0; i < max_key; i++ ) {
        // get key from previous key down
        const uint16_t key_value = dev->previous_key_down[ i ];
        // handle invalid or still down
        if ( 0 != key_value && ! keyboard_contains_key( dev->current_key_down, key_value, max_key ) ) {
          keyboard_key_up( dev, key_value );
        }
      }
    }
  }
  // copy over current key down as previous down
  memcpy( dev->previous_key_down, dev->current_key_down, KEYBOARD_MAX_KEYS * sizeof( uint16_t ) );
  free( response );
  _syscall_rpc_cleanup();
}
