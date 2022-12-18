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

#ifndef __U2P_H__
#define __U2P_H__

// --------------------------------------------------------------------
//	Initialize u2p
//	input:
//		none
//	output:
//		none
// --------------------------------------------------------------------
void u2p_init( void );

// --------------------------------------------------------------------
//	u2p task
//	input:
//		none
//	output:
//		none
// --------------------------------------------------------------------
void u2p_task( void );

// --------------------------------------------------------------------
void u2p_get_mouse( int *p_x, int *p_y, int *p_button );

// --------------------------------------------------------------------
//	Get information
//	input:
//		index ... information index
//	output:
//		Information
// --------------------------------------------------------------------
enum {
	U2P_LED = 0,
};

int u2p_get_information( int index );

#endif
