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

#endif
