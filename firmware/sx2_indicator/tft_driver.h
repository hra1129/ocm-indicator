// --------------------------------------------------------------------
//	The MIT License (MIT)
//	
//	SX|2 indicator TFT driver
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

#ifndef __TFT_DRIVER_H__
#define __TFT_DRIVER_H__

// --------------------------------------------------------------------
//	Initialize TFT driver
//	input:
//		none
//	output:
//		true ..... Success of initialize
//		false .... Failed of initialize
// --------------------------------------------------------------------
bool tft_init( void );

// --------------------------------------------------------------------
//	Send frame buffer
//	input:
//		p_buffer .. address of frame buffer
//	output:
//		none
// --------------------------------------------------------------------
void tft_send_framebuffer( const uint16_t *p_buffer );

// --------------------------------------------------------------------
void tft_pset( uint16_t *p_dest, int dest_width, int dest_height, int x, int y, uint16_t color );

// --------------------------------------------------------------------
uint16_t tft_point( const uint16_t *p_src, int src_width, int src_height, int x, int y );

// --------------------------------------------------------------------
void tft_copy( uint16_t *p_dest, int dest_width, int dest_height, int dx, int dy, const uint16_t *p_src, int src_width, int src_height, int sx, int sy, int copy_width, int copy_height );

#endif
