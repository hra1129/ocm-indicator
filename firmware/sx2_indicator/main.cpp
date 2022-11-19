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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tft_driver.h"

#define IMAGE_WIDTH		240
#define IMAGE_HEIGHT	135
#define IMAGE_SIZE		(IMAGE_WIDTH * IMAGE_HEIGHT)

static uint16_t buffer[ IMAGE_SIZE ];

// --------------------------------------------------------------------
void draw_pset( int x, int y, int color ) {

	if( x < 0 || x >= IMAGE_WIDTH || y < 0 || y >= IMAGE_HEIGHT ) {
		return;
	}
	buffer[ x + y * IMAGE_WIDTH ] = (uint16_t) color;
}

// --------------------------------------------------------------------
void draw_line( int x1, int y1, int x2, int y2, int color ) {
	int vx, vy, dx, dy, x, y, i;

	dx = x2 - x1;
	if( dx < 0 ) {
		vx = -1;
		dx = -dx;
	}
	else if( dx == 0 ) {
		vx = 0;
	}
	else {
		vx = 1;
	}

	dy = y2 - y1;
	if( dy < 0 ) {
		vy = -1;
		dy = -dy;
	}
	else if( dy == 0 ) {
		vy = 0;
	}
	else {
		vy = 1;
	}

	if( dx == 0 && dy == 0 ) {
		draw_pset( x1, y1, color );
	}
	else if( dx < dy ) {
		for( i = 0; i <= dy; i++ ) {
			x = x1 + (x2 - x1) * i / dy;
			draw_pset( x, y1, color );
			y1 += vy;
		}
	}
	else {
		for( i = 0; i <= dx; i++ ) {
			y = y1 + (y2 - y1) * i / dx;
			draw_pset( x1, y, color );
			x1 += vx;
		}
	}
}

// --------------------------------------------------------------------
int main( void ) {
	int i;

	stdio_init_all();
	tft_init();

	memset( buffer, 0, sizeof(buffer) );
	for(;;) {
		for( i = 0; i < 10; i++ ) {
			draw_line( rand() % 240, rand() % 135, rand() % 240, rand() % 135, rand() );
		}
		tft_send_framebuffer( buffer );
	}
	return 0;
}
