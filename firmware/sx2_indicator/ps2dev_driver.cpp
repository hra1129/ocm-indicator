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

#include <cstdint>
#include <pico/multicore.h>
#include <hardware/gpio.h>
#include <pico/time.h>
#include "ps2dev_driver.h"

using namespace std;

enum {
	PS2DEV_IDLE = 0,
	// receive state
	PS2DEV_WAIT_START_BIT,
	PS2DEV_WAIT_CLOCK_RELEASE,
	PS2DEV_D0_CLK_TO_LOW,
	PS2DEV_D0_WAIT,
	PS2DEV_D1_CLK_TO_LOW,
	PS2DEV_D1_WAIT,
	PS2DEV_D2_CLK_TO_LOW,
	PS2DEV_D2_WAIT,
	PS2DEV_D3_CLK_TO_LOW,
	PS2DEV_D3_WAIT,
	PS2DEV_D4_CLK_TO_LOW,
	PS2DEV_D4_WAIT,
	PS2DEV_D5_CLK_TO_LOW,
	PS2DEV_D5_WAIT,
	PS2DEV_D6_CLK_TO_LOW,
	PS2DEV_D6_WAIT,
	PS2DEV_D7_CLK_TO_LOW,
	PS2DEV_D7_WAIT,
	PS2DEV_PARITY_CLK_TO_LOW,
	PS2DEV_PARITY_WAIT,
	PS2DEV_STOP_CLK_TO_LOW,
	PS2DEV_STOP_WAIT,
	PS2DEV_SEND_ACK,
	PS2DEV_ACK_CLK_TO_LOW,
	PS2DEV_ACK_CLK_END,
	PS2DEV_ACK_DAT_END,
	// send state
	PS2DEV_SEND_DATA,
	PS2DEV_SEND_START_BIT,
	PS2DEV_SEND_START_BIT_CLK_TO_LOW,
	PS2DEV_SEND_START_BIT_WAIT,
	PS2DEV_SEND_D0,
	PS2DEV_SEND_D0_CLK_TO_LOW,
	PS2DEV_SEND_D0_WAIT,
	PS2DEV_SEND_D1,
	PS2DEV_SEND_D1_CLK_TO_LOW,
	PS2DEV_SEND_D1_WAIT,
	PS2DEV_SEND_D2,
	PS2DEV_SEND_D2_CLK_TO_LOW,
	PS2DEV_SEND_D2_WAIT,
	PS2DEV_SEND_D3,
	PS2DEV_SEND_D3_CLK_TO_LOW,
	PS2DEV_SEND_D3_WAIT,
	PS2DEV_SEND_D4,
	PS2DEV_SEND_D4_CLK_TO_LOW,
	PS2DEV_SEND_D4_WAIT,
	PS2DEV_SEND_D5,
	PS2DEV_SEND_D5_CLK_TO_LOW,
	PS2DEV_SEND_D5_WAIT,
	PS2DEV_SEND_D6,
	PS2DEV_SEND_D6_CLK_TO_LOW,
	PS2DEV_SEND_D6_WAIT,
	PS2DEV_SEND_D7,
	PS2DEV_SEND_D7_CLK_TO_LOW,
	PS2DEV_SEND_D7_WAIT,
	PS2DEV_SEND_PARITY,
	PS2DEV_SEND_PARITY_CLK_TO_LOW,
	PS2DEV_SEND_PARITY_WAIT,
	PS2DEV_SEND_STOP,
	PS2DEV_SEND_STOP_CLK_TO_LOW,
	PS2DEV_SEND_STOP_WAIT,
};
typedef int PS2DEV_STATE_T;

static PS2DEV_STATE_T ps2dev_state;
static uint64_t start_time;
static uint8_t receive_data;
static int send_data;

enum {
	SEND_DATA = 0,
	SEND_ABORT,
	SEND_SUCCESS,
};
static volatile int send_result;
static uint8_t parity_check;
static semaphore_t sem;

static uint8_t receive_fifo[ 8 ];
static int fifo_read_ptr;
static int fifo_write_ptr;

// --------------------------------------------------------------------
static bool inline is_fifo_empty( void ) {
	return( fifo_read_ptr == fifo_write_ptr );
}

// --------------------------------------------------------------------
static bool inline is_fifo_full( void ) {
	return( ((fifo_write_ptr + 1) & 7) == fifo_read_ptr );
}

// --------------------------------------------------------------------
static void push_fifo( uint8_t data ) {
	if( is_fifo_full() ) {
		return;
	}
	receive_fifo[ fifo_write_ptr ] = data;
	fifo_write_ptr = (fifo_write_ptr + 1) & 7;
}

// --------------------------------------------------------------------
static uint8_t pop_fifo( void ) {
	uint8_t data;

	if( is_fifo_empty() ) {
		return 0;
	}
	data = receive_fifo[ fifo_read_ptr ];
	fifo_read_ptr = (fifo_read_ptr + 1) & 7;
	return data;
}

// --------------------------------------------------------------------
static uint64_t inline _get_us( void ) {
	return to_us_since_boot( get_absolute_time() );
}

// --------------------------------------------------------------------
bool ps2dev_init( void ) {

	gpio_init( PS2CLK_PORT );
	gpio_init( PS2DAT_PORT );
	gpio_pull_up( PS2CLK_PORT );
	gpio_pull_up( PS2DAT_PORT );
	gpio_set_dir( PS2CLK_PORT, GPIO_IN );
	gpio_set_dir( PS2DAT_PORT, GPIO_IN );
	gpio_put( PS2CLK_PORT, 0 );
	gpio_put( PS2DAT_PORT, 0 );
	ps2dev_state = PS2DEV_IDLE;

	fifo_read_ptr = 0;
	fifo_write_ptr = 0;
	sem_init( &sem, 1, 1 );
	return true;
}

// --------------------------------------------------------------------
void ps2dev_task( void ) {

	switch( ps2dev_state ) {
	case PS2DEV_IDLE:
		gpio_set_dir( PS2CLK_PORT, GPIO_IN );
		if( !gpio_get( PS2CLK_PORT ) ) {
			//	PS2CLK is LOW. It's send request from HOST.
			ps2dev_state = PS2DEV_WAIT_START_BIT;
			start_time = _get_us();
		}
		break;
	case PS2DEV_WAIT_START_BIT:
		gpio_set_dir( PS2DAT_PORT, GPIO_IN );
		if( !gpio_get( PS2DAT_PORT ) ) {
			//	PS2DAT is LOW. This is start bit from HOST.
			ps2dev_state = PS2DEV_WAIT_CLOCK_RELEASE;
			receive_data = 0;
			parity_check = 0;
		}
		else if( (_get_us() - start_time) > 15000 ) {
			//	Time out error.
			ps2dev_state = PS2DEV_IDLE;
		}
		break;
	case PS2DEV_WAIT_CLOCK_RELEASE:
		if( gpio_get( PS2CLK_PORT ) ) {
			//	PS2CLK is HIGH. It's released by HOST.
			ps2dev_state = PS2DEV_D0_CLK_TO_LOW;
			receive_data = 0;
			start_time = _get_us();
		}
		else if( (_get_us() - start_time) > 15000 ) {
			//	Time out error.
			ps2dev_state = PS2DEV_IDLE;
		}
		break;
	case PS2DEV_D0_CLK_TO_LOW:
	case PS2DEV_D1_CLK_TO_LOW:
	case PS2DEV_D2_CLK_TO_LOW:
	case PS2DEV_D3_CLK_TO_LOW:
	case PS2DEV_D4_CLK_TO_LOW:
	case PS2DEV_D5_CLK_TO_LOW:
	case PS2DEV_D6_CLK_TO_LOW:
	case PS2DEV_D7_CLK_TO_LOW:
	case PS2DEV_PARITY_CLK_TO_LOW:
	case PS2DEV_STOP_CLK_TO_LOW:
		if( (_get_us() - start_time) > 30 ) {
			//	Set PS2CLK LOW.
			gpio_set_dir( PS2CLK_PORT, GPIO_OUT );
			ps2dev_state++;
			start_time = _get_us();
		}
		break;
	case PS2DEV_D0_WAIT:
	case PS2DEV_D1_WAIT:
	case PS2DEV_D2_WAIT:
	case PS2DEV_D3_WAIT:
	case PS2DEV_D4_WAIT:
	case PS2DEV_D5_WAIT:
	case PS2DEV_D6_WAIT:
	case PS2DEV_D7_WAIT:
		if( (_get_us() - start_time) > 30 ) {
			//	Set PS2CLK HIGH.
			gpio_set_dir( PS2CLK_PORT, GPIO_IN );
			receive_data >>= 1;
			if( gpio_get( PS2DAT_PORT ) ) {
				receive_data = receive_data | 0x80;
				parity_check = parity_check ^ 1;
			}
			ps2dev_state++;
			start_time = _get_us();
		}
		break;
	case PS2DEV_PARITY_WAIT:
		if( (_get_us() - start_time) > 30 ) {
			//	Set PS2CLK HIGH.
			gpio_set_dir( PS2CLK_PORT, GPIO_IN );
			if( gpio_get( PS2DAT_PORT ) ) {
				parity_check = parity_check ^ 1;
			}
			ps2dev_state++;
			start_time = _get_us();
		}
		break;
	case PS2DEV_STOP_WAIT:
		if( (_get_us() - start_time) > 30 ) {
			//	Set PS2CLK HIGH.
			gpio_set_dir( PS2CLK_PORT, GPIO_IN );
			ps2dev_state++;
			start_time = _get_us();
		}
		break;
	case PS2DEV_SEND_ACK:
		if( (_get_us() - start_time) > 5 ) {
			//	Set PS2DAT LOW.
			gpio_set_dir( PS2DAT_PORT, GPIO_OUT );
			ps2dev_state++;
			start_time = _get_us();
		}
		break;
	case PS2DEV_ACK_CLK_TO_LOW:
		if( (_get_us() - start_time) > 25 ) {
			//	Set PS2CLK LOW.
			gpio_set_dir( PS2CLK_PORT, GPIO_OUT );
			ps2dev_state++;
			start_time = _get_us();
		}
		break;
	case PS2DEV_ACK_CLK_END:
		if( (_get_us() - start_time) > 30 ) {
			//	Set PS2CLK HIGH.
			gpio_set_dir( PS2CLK_PORT, GPIO_IN );

			sem_acquire_blocking( &sem );
			if( !is_fifo_full() ) {
				push_fifo( receive_data );
			}
			sem_release( &sem );
			ps2dev_state = PS2DEV_ACK_DAT_END;
		}
		break;
	case PS2DEV_ACK_DAT_END:
		if( (_get_us() - start_time) > 5 ) {
			//	Set PS2DAT HIGH.
			gpio_set_dir( PS2DAT_PORT, GPIO_IN );

			ps2dev_state = PS2DEV_IDLE;
		}
		break;
	// send state
	case PS2DEV_SEND_DATA:
		//	Set PS2CLK HIGH.
		gpio_set_dir( PS2CLK_PORT, GPIO_IN );
		ps2dev_state++;
		parity_check = 0;
		start_time = _get_us();
		break;
	case PS2DEV_SEND_START_BIT:
	case PS2DEV_SEND_D0:
	case PS2DEV_SEND_D1:
	case PS2DEV_SEND_D2:
	case PS2DEV_SEND_D3:
	case PS2DEV_SEND_D4:
	case PS2DEV_SEND_D5:
	case PS2DEV_SEND_D6:
	case PS2DEV_SEND_D7:
	case PS2DEV_SEND_STOP:
		if( (_get_us() - start_time) > 15 ) {
			if( !gpio_get( PS2CLK_PORT ) ) {
				send_result = SEND_ABORT;
				ps2dev_state = PS2DEV_IDLE;
			}
			if( (send_data & 1) == 0 ) {
				//	Set PS2DAT LOW.
				gpio_set_dir( PS2DAT_PORT, GPIO_OUT );
			}
			else {
				//	Set PS2DAT HIGH.
				gpio_set_dir( PS2DAT_PORT, GPIO_IN );
				parity_check = parity_check ^ 1;
			}
			send_data >>= 1;
			ps2dev_state++;
			start_time = _get_us();
		}
		break;
	case PS2DEV_SEND_PARITY:
		if( (_get_us() - start_time) > 15 ) {
			if( !gpio_get( PS2CLK_PORT ) ) {
				send_result = SEND_ABORT;
				ps2dev_state = PS2DEV_IDLE;
			}
			if( parity_check ) {
				//	Set PS2DAT LOW.
				gpio_set_dir( PS2DAT_PORT, GPIO_OUT );
			}
			else {
				//	Set PS2DAT HIGH.
				gpio_set_dir( PS2DAT_PORT, GPIO_IN );
			}
			ps2dev_state++;
			start_time = _get_us();
		}
		break;
	case PS2DEV_SEND_START_BIT_CLK_TO_LOW:
	case PS2DEV_SEND_D0_CLK_TO_LOW:
	case PS2DEV_SEND_D1_CLK_TO_LOW:
	case PS2DEV_SEND_D2_CLK_TO_LOW:
	case PS2DEV_SEND_D3_CLK_TO_LOW:
	case PS2DEV_SEND_D4_CLK_TO_LOW:
	case PS2DEV_SEND_D5_CLK_TO_LOW:
	case PS2DEV_SEND_D6_CLK_TO_LOW:
	case PS2DEV_SEND_D7_CLK_TO_LOW:
	case PS2DEV_SEND_PARITY_CLK_TO_LOW:
	case PS2DEV_SEND_STOP_CLK_TO_LOW:
		if( (_get_us() - start_time) > 15 ) {
			if( !gpio_get( PS2CLK_PORT ) ) {
				send_result = SEND_ABORT;
				ps2dev_state = PS2DEV_IDLE;
			}
			//	Set PS2CLK LOW.
			gpio_set_dir( PS2CLK_PORT, GPIO_OUT );
			ps2dev_state++;
			start_time = _get_us();
		}
		break;
	case PS2DEV_SEND_START_BIT_WAIT:
	case PS2DEV_SEND_D0_WAIT:
	case PS2DEV_SEND_D1_WAIT:
	case PS2DEV_SEND_D2_WAIT:
	case PS2DEV_SEND_D3_WAIT:
	case PS2DEV_SEND_D4_WAIT:
	case PS2DEV_SEND_D5_WAIT:
	case PS2DEV_SEND_D6_WAIT:
	case PS2DEV_SEND_D7_WAIT:
	case PS2DEV_SEND_PARITY_WAIT:
		if( (_get_us() - start_time) > 30 ) {
			//	Set PS2CLK HIGH.
			gpio_set_dir( PS2CLK_PORT, GPIO_IN );
			ps2dev_state++;
			start_time = _get_us();
		}
		break;
	case PS2DEV_SEND_STOP_WAIT:
		if( (_get_us() - start_time) > 30 ) {
			//	Set PS2CLK HIGH.
			gpio_set_dir( PS2CLK_PORT, GPIO_IN );
			ps2dev_state = PS2DEV_IDLE;
			send_result = SEND_SUCCESS;
		}
		break;
	default:
		break;
	}
}

// --------------------------------------------------------------------
bool ps2dev_check_receive_buffer_empty( void ) {

	return is_fifo_empty();
}

// --------------------------------------------------------------------
bool ps2dev_get_receive_data( uint8_t *p_data ) {

	if( is_fifo_empty() ) {
		return false;
	}
	sem_acquire_blocking( &sem );
	*p_data = pop_fifo();
	sem_release( &sem );
	return true;
}

// --------------------------------------------------------------------
bool ps2dev_send_data( uint8_t data ) {

	if( ps2dev_state != PS2DEV_IDLE ) {
		return false;
	}
	//	dddd_dddd → 1d_dddd_ddd0
	send_data = (data << 1) | 0x200;
	send_result = SEND_DATA;
	ps2dev_state = PS2DEV_SEND_DATA;
	while( send_result == SEND_DATA ) {
		sleep_us( 30 );
	}
	return( send_result == SEND_SUCCESS );
}
