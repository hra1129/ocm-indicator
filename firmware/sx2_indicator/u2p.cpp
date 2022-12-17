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

enum {
	PS2_IDLE = 0,
	PS2_SEND_DATAS,
};
static int ps2state = PS2_IDLE;
static int mouse_x = 240 / 2, mouse_y = 135 / 2, mouse_button = 0;

// --------------------------------------------------------------------
static void ps2_send_datas( void ) {
	int16_t delta_x, delta_y;
	int32_t button;

	if( !ps2dev_is_send_fifo_empty() ) {
		return;
	}
	delta_x = 0;
	delta_y = 0;
	mouse_button = 0x08;
	if( is_mouse_active() ) {
		get_mouse_position( &delta_x, &delta_y, &button );
		mouse_x += delta_x;
		mouse_y += delta_y;
		if( mouse_x < 0 ) {
			mouse_x = 0;
		}
		else if( mouse_x > 240 ) {
			mouse_x = 240;
		}
		if( mouse_y < 0 ) {
			mouse_y = 0;
		}
		else if( mouse_y > 135 ) {
			mouse_y = 135;
		}
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
		mouse_button |= button;
	}
	ps2dev_send_data( 0xFA );
	ps2dev_send_data( mouse_button );
	ps2dev_send_data( delta_x );
	ps2dev_send_data( delta_y );
	ps2state = PS2_IDLE;
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
void u2p_get_mouse( int *p_x, int *p_y, int *p_button ) {

	*p_x		= mouse_x;
	*p_y		= mouse_y;
	*p_button	= mouse_button;
}
