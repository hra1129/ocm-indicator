// --------------------------------------------------------------------
//	The MIT License (MIT)
//	
//	SX|2 indicator USB to PS/2 converter
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

#include "u2p.h"
#include "usb_host_driver.h"
#include "ps2dev_driver.h"
#include <pico/time.h>

enum {
	PS2_IDLE = 0,
	PS2_SEND_DATAS,
	PS2_RECV_DATAS,
};
static int ps2state = PS2_IDLE;

static volatile int ocm_status[16] = {};
static int ocm_status_write_ptr;
static int remain_bytes;
static uint64_t start_time;

// --------------------------------------------------------------------
static uint64_t inline _get_us( void ) {
	return to_us_since_boot( get_absolute_time() );
}

// --------------------------------------------------------------------
static void ps2_send_datas( void ) {
	int16_t delta_x, delta_y;
	int32_t button;
	int mouse_button;

	if( !ps2dev_is_send_fifo_empty() ) {
		return;
	}
	if( is_mouse_active() ) {
		get_mouse_position( &delta_x, &delta_y, &button );
		if( delta_x < -128 ) {
			delta_x = -128;
		}
		else if( delta_x > 127 ) {
			delta_x = 127;
		}
		delta_y = -delta_y;
		if( delta_y < -128 ) {
			delta_y = -128;
		}
		else if( delta_y > 127 ) {
			delta_y = 127;
		}
		mouse_button = 0x08 | button;
	}
	else {
		delta_x = 0;
		delta_y = 0;
		mouse_button = 0;
	}
	ps2dev_send_data( 0xFA );
	ps2dev_send_data( mouse_button );
	ps2dev_send_data( delta_x );
	ps2dev_send_data( delta_y );
	ps2state = PS2_RECV_DATAS;
	ocm_status_write_ptr = 0;
	remain_bytes = -1;
	start_time = _get_us();
}

// --------------------------------------------------------------------
static void ps2_recv_datas( void ) {
	uint8_t data;

	if( !ps2dev_get_receive_data( &data ) ) {
		if( (_get_us() - start_time) > 50000 ) {
			//	time out
			ps2state = PS2_IDLE;
		}
		return;
	}
	start_time = _get_us();
	if( remain_bytes == -1 ) {
		remain_bytes = data;
		return;
	}
	if( remain_bytes ) {
		if( ocm_status_write_ptr < (int)(sizeof(ocm_status) / sizeof(ocm_status[0])) ) {
			ocm_status[ ocm_status_write_ptr++ ] = data;
		}
		remain_bytes--;
	}
	if( remain_bytes == 0 ) {
		ps2state = PS2_IDLE;
	}
}

// --------------------------------------------------------------------
static void ps2_communication( void ) {
	uint8_t data;

	switch( ps2state ) {
	case PS2_IDLE:
		if( ps2dev_get_receive_data( &data ) ) {
			if( data == 0xFF ) {
				ps2dev_send_data( 0xFA );
				ps2dev_send_data( 0xAA );
				ps2dev_send_data( 0x10 );
			}
			else if( data == 0xF3 ) {
				ps2dev_send_data( 0xFA );
			}
			else if( data == 40 ) {
				ps2dev_send_data( 0xFA );
			}
			else if( data == 0xEB ) {
				ps2state = PS2_SEND_DATAS;
			}
		}
		break;
	case PS2_SEND_DATAS:
		ps2_send_datas();
		break;
	case PS2_RECV_DATAS:
		ps2_recv_datas();
		break;
	default:
		break;
	}
}

// --------------------------------------------------------------------
void u2p_init( void ) {
}

// --------------------------------------------------------------------
void u2p_task( void ) {
	ps2_communication();
}

// --------------------------------------------------------------------
int u2p_get_information( int index ) {
	return ocm_status[ index ];
}
