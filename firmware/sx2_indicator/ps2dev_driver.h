// --------------------------------------------------------------------
//	The MIT License (MIT)
//	
//	SX|2 indicator PS/2 for device side driver
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

#ifndef __PS2DEV_DRIVER_H__
#define __PS2DEV_DRIVER_H__

#define PS2CLK_PORT		11
#define PS2DAT_PORT		10

// --------------------------------------------------------------------
//	Initialize PS2DEV driver
//	input:
//		none
//	output:
//		true ..... Success of initialize
//		false .... Failed of initialize
// --------------------------------------------------------------------
bool ps2dev_init( void );

// --------------------------------------------------------------------
//	ps2dev task
//	input:
//		none
//	output:
//		none
// --------------------------------------------------------------------
void ps2dev_task( void );

// --------------------------------------------------------------------
//	Check receive buffer empty
//	input:
//		none
//	output:
//		false ........... It's empty.
//		true ............ It's not empty.
// --------------------------------------------------------------------
bool ps2dev_check_receive_buffer_empty( void );

// --------------------------------------------------------------------
//	Get receive data (1byte)
//	input:
//		p_data .......... Address of buffer to return read results.
//	output:
//		true ............ Success. *p_data is active.
//		false ........... Failed. Not found received data.
// --------------------------------------------------------------------
bool ps2dev_get_receive_data( uint8_t *p_data );

// --------------------------------------------------------------------
//	Send data
//	input:
//		send data
//	output:
//		true ........... Success
//		false .......... Failed (aborted)
// --------------------------------------------------------------------
bool ps2dev_send_data( uint8_t data );

// --------------------------------------------------------------------
//	Check send fifo
//	input:
//		none
//	output:
//		true ........... empty
//		false .......... not empty
// --------------------------------------------------------------------
bool ps2dev_is_send_fifo_empty( void );

// --------------------------------------------------------------------
//	デバッグ用
// --------------------------------------------------------------------
int ps2dev_get_state( void );

#endif
