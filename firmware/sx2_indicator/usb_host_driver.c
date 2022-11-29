// --------------------------------------------------------------------
//	The MIT License (MIT)
//	
//	SX|2 indicator USB host driver
//	Copyright (c) 2022 Takayuki Hara
//	
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//	
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//	
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.
// --------------------------------------------------------------------

#include <stdlib.h>
#include <tusb.h>
#include "tusb_config.h"
#include "usb_host_driver.h"
#include "hardware/gpio.h"		//	★ 

static volatile bool	mouse_is_active = false;
static volatile int16_t	mouse_delta_x = 0;
static volatile int16_t	mouse_delta_y = 0;
static volatile int		mouse_resolution = 0;
static int32_t			mouse_button = 0;
static bool				mouse_consume_data = false;

#define MAX_REPORT  4

static uint8_t report_count[ CFG_TUH_HID ];
static tuh_hid_report_info_t report_info_arr[ CFG_TUH_HID ][ MAX_REPORT ];

// --------------------------------------------------------------------
bool is_mouse_active( void ) {
	return mouse_is_active;
}

// --------------------------------------------------------------------
void get_mouse_position( int16_t *p_delta_x, int16_t *p_delta_y, int32_t *p_button ) {

	if( mouse_is_active ) {
		*p_delta_x	= mouse_delta_x;
		*p_delta_y	= mouse_delta_y;
		*p_button	= mouse_button;
	}
	else {
		*p_delta_x	= 0;
		*p_delta_y	= 0;
		*p_button	= 0;
	}
	mouse_consume_data = true;
}

// --------------------------------------------------------------------
static void process_mouse_report( hid_mouse_report_t const * report ) {
	int16_t delta_x;
	int16_t delta_y;

	if( mouse_consume_data ) {
		mouse_consume_data = false;
		mouse_delta_x = 0;
		mouse_delta_y = 0;
	}
	delta_x = mouse_delta_x - (int16_t) report->x;
	if( delta_x < -127 ) {
		delta_x = -127;
	}
	else if( delta_x > 127 ) {
		delta_x = 127;
	}

	delta_y = mouse_delta_y - (int16_t) report->y;
	if( delta_y < -127 ) {
		delta_y = -127;
	}
	else if( delta_y > 127 ) {
		delta_y = 127;
	}

	mouse_button = (report->buttons & (MOUSE_BUTTON_RIGHT | MOUSE_BUTTON_LEFT | MOUSE_BUTTON_MIDDLE));

	//	排他制御は面倒なので省略 (^^;
	mouse_delta_x = delta_x;
	mouse_delta_y = delta_y;
}

// --------------------------------------------------------------------
//	HIDが接続されたときに呼び出されるコールバック
//
//	Callback to be called when a gamepad is connected.
//
void tuh_hid_mount_cb( uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len ) {

	//	Check device type
	uint8_t const interface_protocol = tuh_hid_interface_protocol( dev_addr, instance );

	#if DEBUG_UART_ON
		printf( "tuh_hid_mount_cb( %d, %d, %p, %d );\n", dev_addr, instance, desc_report, desc_len );
	#endif
	report_count[instance] = tuh_hid_parse_report_descriptor( report_info_arr[instance], MAX_REPORT, desc_report, desc_len );

	if( interface_protocol == HID_ITF_PROTOCOL_MOUSE ) {
		gpio_put( 25, 1 );	//	★ 
		mouse_is_active = true;
		mouse_delta_x = 0;
		mouse_delta_y = 0;
		mouse_button = 0;
		mouse_resolution = 0;
	}
	else {
		//process_mode = 0;	//	joypad_mode
	}
}

// --------------------------------------------------------------------
//	HIDが切断されたときに呼び出されるコールバック
//
//	Callback to be called when the gamepad is disconnected.
//
void tuh_hid_umount_cb( uint8_t dev_addr, uint8_t instance ) {
	gpio_put( 25, 0 );	//	★ 

	(void) dev_addr;
	(void) instance;

	//process_mode = 0;	//	joypad_mode
	mouse_is_active = false;
}

// --------------------------------------------------------------------
void tuh_hid_report_received_cb( uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len ) {

	(void) dev_addr;

	uint8_t const rpt_count = report_count[ instance ];
	tuh_hid_report_info_t* rpt_info_arr = report_info_arr[ instance ];
	tuh_hid_report_info_t* rpt_info = NULL;

	#if DEBUG_UART_ON
		printf( "tuh_hid_report_received_cb( %d, %d, %p, %d )\n", dev_addr, instance, report, len );
	#endif

	if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0) {
		// Simple report without report ID as 1st byte
		rpt_info = &rpt_info_arr[0];
	} else {
		// Composite report, 1st byte is report ID, data starts from 2nd byte
		uint8_t const rpt_id = report[0];

		// Find report id in the arrray
		for( uint8_t i = 0; i < rpt_count; i++ ) {
			if( rpt_id == rpt_info_arr[i].report_id ) {
				rpt_info = &rpt_info_arr[i];
				break;
			}
		}

		report++;
		len--;
	}

	if( !rpt_info ) {
		#if DEBUG_UART_ON
			printf( "-- rpt_info == NULL\n" );
		#endif
		return;
	}

	#if DEBUG_UART_ON
		printf( "-- rpt_info->usage_page == %d\n", rpt_info->usage_page );
		printf( "-- rpt_info->usage == %d\n", rpt_info->usage );
	#endif
	if( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP ) {
		//if( rpt_info->usage == HID_USAGE_DESKTOP_GAMEPAD ) {
		//	process_gamepad_report( (hid_gamepad_report_t const*) report );
		//}
		//else if( rpt_info->usage == HID_USAGE_DESKTOP_JOYSTICK ) {
		//	process_joystick_report( (my_hid_joystick_report_t const*) report );
		//}
		//else 
		if( rpt_info->usage == HID_USAGE_DESKTOP_MOUSE ) {
			process_mouse_report( (hid_mouse_report_t const*) report );
		}
	}
}
