// --------------------------------------------------------------------
//	The MIT License (MIT)
//	
//	SX|2 indicator
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

using namespace std;

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <bsp/board.h>
#include <pico/time.h>
#include <tusb.h>
#include "tft_driver.h"
#include "usb_host_driver.h"
#include "ps2dev_driver.h"

#define IMAGE_WIDTH		240
#define IMAGE_HEIGHT	135
#define IMAGE_SIZE		(IMAGE_WIDTH * IMAGE_HEIGHT)

static uint16_t buffer1[ IMAGE_SIZE ];
static uint16_t buffer2[ IMAGE_SIZE ];

#include "resource/grp_background.h"
#include "resource/grp_msx.h"
#include "resource/grp_indicator.h"
#include "resource/grp_red_led.h"
#include "resource/grp_led.h"
#include "resource/grp_mouse.h"
#include "resource/grp_font.h"

enum {
	PS2_IDLE = 0,
	PS2_RESET_RECEIVED,
	PS2_SEND_SELFTEST_COMPLETE,
	PS2_SEND_MOUSE_ID,
	PS2_SEND_ACK,
	PS2_SEND_ACK_WITH_ID,
	PS2_SEND_ACK_WITH_PACKET,
	PS2_SEND_1ST_BYTE,
	PS2_SEND_2ND_BYTE,
	PS2_SEND_3RD_BYTE,
};
static int ps2state = PS2_IDLE;

// --------------------------------------------------------------------
static void response_core( void ) {
	int y = 0;
	int16_t delta_x, delta_y;
	int32_t button;
	uint16_t *p_draw_buffer;
	int mouse_x = 240 / 2, mouse_y = 135 / 2, mouse_button = 0;
	static char s_buffer[31] = {};
	uint8_t data;

	tft_init();
	p_draw_buffer = buffer1;
	for(;;) {
		if( y < 64 ) {
			tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 0, 0, grp_background, 240, 135, 0, 0, 240, 135 );
			tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 38, 35 + 64 - y, grp_msx, 164, 64, 0, 0, 164, y );
			y++;
		}
		else if( y < 128 ) {
			tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 0, 0, grp_background, 240, 135, 0, 0, 240, 135 );
			tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 38, 35, grp_msx, 164, 64, 0, 0, 164, 64 );
			y++;
		}
		else {
			tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 0, 0, grp_indicator, 240, 135, 0, 0, 240, 135 );
			if( is_mouse_active() ) {
				tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 208, 104, grp_mouse, grp_mouse_width, grp_mouse_height, 0, 0, grp_mouse_width, grp_mouse_height );
			}
		}

		if( is_mouse_active() ) {
			tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, mouse_x, mouse_y, grp_mouse, grp_mouse_width, grp_mouse_height, 0, 0, grp_mouse_width, grp_mouse_height );
		}

		switch( ps2state ) {
		case PS2_IDLE:
			if( ps2dev_get_receive_data( &data ) ) {
				if( data == 0xFF ) {
					sprintf( s_buffer, "RECEIVE PS/2 RESET CMD." );
					ps2state++;
				}
				else if( data == 0xF3 ) {
					sprintf( s_buffer, "RECEIVE PS/2 SAMPLE RATE." );
					ps2state = PS2_SEND_ACK;
				}
				else if( data == 0xC8 ) {
					sprintf( s_buffer, "RECEIVE PS/2 DECIMAL 200." );
					ps2state = PS2_SEND_ACK;
				}
				else if( data == 0x64 ) {
					sprintf( s_buffer, "RECEIVE PS/2 DECIMAL 100." );
					ps2state = PS2_SEND_ACK;
				}
				else if( data == 0x50 ) {
					sprintf( s_buffer, "RECEIVE PS/2 DECIMAL 80." );
					ps2state = PS2_SEND_ACK;
				}
				else if( data == 0xF2 ) {
					sprintf( s_buffer, "RECEIVE PS/2 READ DEVICE TYPE." );
					ps2state = PS2_SEND_ACK_WITH_ID;
				}
				else if( data == 0xF4 ) {
					sprintf( s_buffer, "RECEIVE PS/2 ENABLE DATA REPO." );
					ps2state = PS2_SEND_ACK_WITH_PACKET;
				}
			}
			break;
		case PS2_RESET_RECEIVED:
			if( ps2dev_send_data( 0xFA ) ) {
				sprintf( s_buffer, "SEND PS/2 0xFA." );
				ps2state++;
			}
			else {
				sprintf( s_buffer, "SENDING PS/2 0xFA." );
			}
			break;
		case PS2_SEND_SELFTEST_COMPLETE:
			if( ps2dev_send_data( 0xAA ) ) {
				sprintf( s_buffer, "SEND PS/2 0xAA." );
				ps2state++;
			}
			else {
				sprintf( s_buffer, "SENDING PS/2 0xAA." );
			}
			break;
		case PS2_SEND_MOUSE_ID:
			if( ps2dev_send_data( 0x00 ) ) {
				sprintf( s_buffer, "SEND PS/2 0x00." );
				ps2state = PS2_IDLE;
			}
			else {
				sprintf( s_buffer, "SENDING PS/2 0x00." );
			}
			break;
		case PS2_SEND_ACK:
			if( ps2dev_send_data( 0xFA ) ) {
				sprintf( s_buffer, "SEND PS/2 0xFA." );
				ps2state = PS2_IDLE;
			}
			else {
				sprintf( s_buffer, "SENDING PS/2 0xFA." );
			}
			break;
		case PS2_SEND_ACK_WITH_ID:
			if( ps2dev_send_data( 0xFA ) ) {
				sprintf( s_buffer, "SEND PS/2 0xFA." );
				ps2state = PS2_SEND_MOUSE_ID;
			}
			else {
				sprintf( s_buffer, "SENDING PS/2 0xFA." );
			}
			break;
		case PS2_SEND_ACK_WITH_PACKET:
			if( ps2dev_send_data( 0xFA ) ) {
				sprintf( s_buffer, "SEND PS/2 0xFA." );
				ps2state = PS2_SEND_1ST_BYTE;
			}
			else {
				sprintf( s_buffer, "SENDING PS/2 0xFA." );
			}
			break;
		case PS2_SEND_1ST_BYTE:
			delta_x = 0;
			delta_y = 0;
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
			mouse_button = 0x08;
			if( button & MOUSE_BUTTON_LEFT ) {
				mouse_button |= 0x01;
			}
			if( button & MOUSE_BUTTON_RIGHT ) {
				mouse_button |= 0x02;
			}
			if( button & MOUSE_BUTTON_MIDDLE ) {
				mouse_button |= 0x04;
			}
			if( ps2dev_send_data( mouse_button ) ) {
				sprintf( s_buffer, "SEND PS/2 button." );
				ps2state = PS2_SEND_2ND_BYTE;
			}
			else {
				sprintf( s_buffer, "SENDING PS/2 button." );
			}
			break;
		case PS2_SEND_2ND_BYTE:
			if( ps2dev_send_data( delta_x ) ) {
				sprintf( s_buffer, "SEND PS/2 delta_x." );
				ps2state = PS2_SEND_3RD_BYTE;
			}
			else {
				sprintf( s_buffer, "SENDING PS/2 delta_x." );
			}
			break;
		case PS2_SEND_3RD_BYTE:
			if( ps2dev_send_data( delta_y ) ) {
				sprintf( s_buffer, "SEND PS/2 delta_y." );
				ps2state = PS2_SEND_1ST_BYTE;
			}
			else {
				sprintf( s_buffer, "SENDING PS/2 delta_y." );
			}
			break;
		default:
			break;
		}
		tft_puts( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 0, 0, 0xFFFF, grp_font, s_buffer );

		tft_send_framebuffer( p_draw_buffer );
		if( p_draw_buffer == buffer1 ) {
			p_draw_buffer = buffer2;
		}
		else {
			p_draw_buffer = buffer1;
		}
	}
}

// --------------------------------------------------------------------
int main( void ) {

	board_init();
	ps2dev_init();

	multicore_launch_core1( response_core );

	usb_init();
	for( ;; ) {
		tuh_task();
		ps2dev_task();
	}
	return 0;
}
