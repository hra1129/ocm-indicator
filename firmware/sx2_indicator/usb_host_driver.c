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
#include "bsp/board.h"
#include <pico/multicore.h>

typedef enum {
	DM_UNKNOWN = 0,
	DM_MOUSE,
	DM_GAMEPAD,
} DETECT_MODE_T;

static volatile DETECT_MODE_T	detect_mode = DM_UNKNOWN;

static volatile int16_t			mouse_delta_x = 0;
static volatile int16_t			mouse_delta_y = 0;
static volatile int				mouse_resolution = 0;
static int32_t					mouse_button = 0;
static semaphore_t				sem;

#define MAX_REPORT	4
#define DEBUG_ON	0

// Each HID instance can has multiple reports
static struct {
	uint8_t report_count;
	tuh_hid_report_info_t report_info[MAX_REPORT];
} hid_info[ CFG_TUH_HID ];

// --------------------------------------------------------------------
void usb_init( void ) {

	tusb_init();
	sem_init( &sem, 1, 1 );
}

// --------------------------------------------------------------------
bool is_mouse_active( void ) {
	return( detect_mode == DM_MOUSE );
}

// --------------------------------------------------------------------
void get_mouse_position( int16_t *p_delta_x, int16_t *p_delta_y, int32_t *p_button ) {

	if( detect_mode == DM_MOUSE ) {
		sem_acquire_blocking( &sem );
		*p_delta_x	= mouse_delta_x;
		*p_delta_y	= mouse_delta_y;
		*p_button	= mouse_button;
		mouse_delta_x = 0;
		mouse_delta_y = 0;
		sem_release( &sem );
	}
	else {
		*p_delta_x	= 0;
		*p_delta_y	= 0;
		*p_button	= 0;
	}
}

// --------------------------------------------------------------------
static void process_mouse_report( hid_mouse_report_t const * report ) {
	int16_t delta_x;
	int16_t delta_y;

	sem_acquire_blocking( &sem );
	delta_x = mouse_delta_x + (int16_t) report->x;
	if( delta_x < -127 ) {
		delta_x = -127;
	}
	else if( delta_x > 127 ) {
		delta_x = 127;
	}

	delta_y = mouse_delta_y + (int16_t) report->y;
	if( delta_y < -127 ) {
		delta_y = -127;
	}
	else if( delta_y > 127 ) {
		delta_y = 127;
	}

	mouse_button = (report->buttons & (MOUSE_BUTTON_RIGHT | MOUSE_BUTTON_LEFT | MOUSE_BUTTON_MIDDLE));
	mouse_delta_x = delta_x;
	mouse_delta_y = delta_y;
	sem_release( &sem );
}

// --------------------------------------------------------------------
//	HIDが接続されたときに呼び出されるコールバック
//
//	Callback to be called when a gamepad is connected.
//
void tuh_hid_mount_cb( uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len ) {

	// Interface protocol (hid_interface_protocol_enum_t)
	#if DEBUG_ON
		static const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
	#endif
	uint8_t const itf_protocol = tuh_hid_interface_protocol( dev_addr, instance );
	#if DEBUG_ON
		printf( "tuh_hid_mount_cb( %d, %d ) : [%s]\r\n", dev_addr, instance, protocol_str[ itf_protocol ] );
	#endif

	// By default host stack will use activate boot protocol on supported interface.
	// Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
	if( itf_protocol == HID_ITF_PROTOCOL_NONE ) {
		hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
		board_led_write( 0 );
		detect_mode = DM_GAMEPAD;
	}
	else if( itf_protocol == HID_ITF_PROTOCOL_MOUSE ) {
		board_led_write( 1 );
		detect_mode = DM_MOUSE;
		mouse_delta_x = 0;
		mouse_delta_y = 0;
		mouse_button = 0;
		mouse_resolution = 0;
	}
	else {
		board_led_write( 0 );
		detect_mode = DM_GAMEPAD;
	}

	// request to receive report
	// tuh_hid_report_received_cb() will be invoked when report is available
	if( !tuh_hid_receive_report( dev_addr, instance ) ) {
		#if DEBUG_ON
			printf( "Error: cannot request to receive report\r\n" );
		#endif
	}
}

// --------------------------------------------------------------------
//	HIDが切断されたときに呼び出されるコールバック
//
//	Callback to be called when the gamepad is disconnected.
//
void tuh_hid_umount_cb( uint8_t dev_addr, uint8_t instance ) {

	(void) dev_addr;
	(void) instance;

	//	ここは、なぜか切断されていなくても頻繁に呼ばれるので何もしない。
	#if DEBUG_ON
		printf( "tuh_hid_umount_cb( %d, %d )\r\n", dev_addr, instance );
	#endif
	detect_mode = DM_UNKNOWN;
}

// --------------------------------------------------------------------
void tuh_hid_report_received_cb( uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len ) {

	uint8_t const itf_protocol = tuh_hid_interface_protocol( dev_addr, instance );

	if( itf_protocol == HID_ITF_PROTOCOL_MOUSE ) {
		process_mouse_report( (hid_mouse_report_t const*) report );
	}
	else {
		// Generic report requires matching ReportID and contents with previous parsed report info
		//	process_gamepad_report( (hid_gamepad_report_t const*) report );
	}

	// continue to request to receive report
	if( !tuh_hid_receive_report( dev_addr, instance ) ) {
		#if DEBUG_ON
			printf( "Error: cannot request to receive report\r\n" );
		#endif
	}
}
