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

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "tusb.h"
#include "tft_driver.h"
#include "usb_host_driver.h"

#define IMAGE_WIDTH		240
#define IMAGE_HEIGHT	135
#define IMAGE_SIZE		(IMAGE_WIDTH * IMAGE_HEIGHT)

static uint16_t buffer1[ IMAGE_SIZE ];
static uint16_t buffer2[ IMAGE_SIZE ];

#include "resource/grp_background.h"
#include "resource/grp_msx.h"
#include "resource/grp_indicator.h"

// --------------------------------------------------------------------
static void response_core( void ) {
	int y = 0;
	uint16_t *p_draw_buffer;

	tft_init();
	p_draw_buffer = buffer1;
	for(;;) {
		if( y < 64 ) {
			tft_copy( p_draw_buffer, 240, 135, 0, 0, grp_background, 240, 135, 0, 0, 240, 135 );
			tft_copy( p_draw_buffer, 240, 135, 38, 35 + 64 - y, grp_msx, 164, 64, 0, 0, 164, y );
			y++;
		}
		else if( y < 128 ) {
			tft_copy( p_draw_buffer, 240, 135, 0, 0, grp_background, 240, 135, 0, 0, 240, 135 );
			tft_copy( p_draw_buffer, 240, 135, 38, 35, grp_msx, 164, 64, 0, 0, 164, 64 );
			y++;
		}
		else {
			tft_copy( p_draw_buffer, 240, 135, 0, 0, grp_indicator, 240, 135, 0, 0, 240, 135 );
		}
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

	stdio_init_all();

	multicore_launch_core1( response_core );

	for( ;; ) {
		tuh_task();
	}
	return 0;
}
