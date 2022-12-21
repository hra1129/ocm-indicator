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
//	日本語（エディタの文字コード自動検出用）
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
#include "u2p.h"

#define IMAGE_WIDTH		240
#define IMAGE_HEIGHT	135
#define IMAGE_SIZE		(IMAGE_WIDTH * IMAGE_HEIGHT)

#define GREEN_LED_X		29
#define GREEN_LED_Y		121
#define GREEN_LED_NEXT	22
#define RED_LED_X		7
#define RED_LED_Y		121
#define CAPS_LED_X		210
#define CAPS_LED_Y		121
#define KANA_LED_X		222
#define KANA_LED_Y		121
#define SLOT1_INFO_X	7
#define SLOT1_INFO_Y	7
#define SLOT2_INFO_X	7
#define SLOT2_INFO_Y	24
#define MASTER_VOL_X	7
#define MASTER_VOL_Y	41
#define PSG_VOL_X		7
#define PSG_VOL_Y		58
#define SCC_VOL_X		7
#define SCC_VOL_Y		75
#define OPLL_VOL_X		7
#define OPLL_VOL_Y		92

#define BIT(d,n)		(((d) >> (n)) & 1)
#define BITS(d,n,b)		(((d) >> (n)) & ((1 << (b)) - 1) )

static uint16_t buffer1[ IMAGE_SIZE ];
static uint16_t buffer2[ IMAGE_SIZE ];

#include "resource/grp_background.h"
#include "resource/grp_msx.h"
#include "resource/grp_indicator.h"
#include "resource/grp_red_led.h"
#include "resource/grp_led.h"
#include "resource/grp_small_led.h"
#include "resource/grp_font.h"

// --------------------------------------------------------------------
static void update_leds( uint16_t *p_draw_buffer ) {
	int d, i;

	//	green LEDs (pLed)
	d = u2p_get_information( U2P_DATA1 );
	for( i = 0; i < 8; i++ ) {
		if( BIT( d, 7 ) != 0 ) {
			tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, GREEN_LED_X + i * GREEN_LED_NEXT, GREEN_LED_Y, grp_led, grp_led_width, grp_led_height, 0, 0, grp_led_width, grp_led_height );
		}
		d <<= 1;
	}

	//	red LED (pLedPwr)
	d = u2p_get_information( U2P_DATA2 );
	if( BIT( d, 7 ) != 0 ) {
		tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, RED_LED_X, RED_LED_Y, grp_red_led, grp_red_led_width, grp_red_led_height, 0, 0, grp_red_led_width, grp_red_led_height );
	}

	//	Caps, Kana
	d = u2p_get_information( U2P_DATA5 );
	if( BIT( d, 4 ) == 0 ) {
		tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, CAPS_LED_X, CAPS_LED_Y, grp_small_led, grp_small_led_width, grp_small_led_height, 0, 0, grp_small_led_width, grp_small_led_height );
	}
	if( BIT( d, 3 ) == 0 ) {
		tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, KANA_LED_X, KANA_LED_Y, grp_small_led, grp_small_led_width, grp_small_led_height, 0, 0, grp_small_led_width, grp_small_led_height );
	}
}

// --------------------------------------------------------------------
static int update_msx_logo( uint16_t *p_draw_buffer, int y ) {

	if( y < 64 ) {
		tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 0, 0, grp_background, 240, 135, 0, 0, 240, 135 );
		tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 38, 35 + 64 - y, grp_msx, 164, 64, 0, 0, 164, y );
		y++;
	}
	else {
		tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 0, 0, grp_background, 240, 135, 0, 0, 240, 135 );
		tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 38, 35, grp_msx, 164, 64, 0, 0, 164, 64 );
		y++;
	}
	return y;
}

// --------------------------------------------------------------------
static void update_page1( uint16_t *p_draw_buffer ) {
	static const char *s_slot_type[] = { "EXTERNAL", "ASC8", "SCC+", "ASC16" };
	static const char *s_volume[] = { "-------", "#------", "##-----", "###----", "####---", "#####--", "#####-", "######" };
	static char s_buffer[31] = {};
	int d2, d3, d4, d5, d6, s;

	d2 = u2p_get_information( U2P_DATA2 );
	d3 = u2p_get_information( U2P_DATA3 );
	d4 = u2p_get_information( U2P_DATA4 );
	d5 = u2p_get_information( U2P_DATA5 );
	d6 = u2p_get_information( U2P_DATA6 );

	//	SLOT#1
	s = BITS( d2, 3, 2 );
	if( s != 0 && BIT( d6, 2 ) != 0 ) {
		strcpy( s_buffer, "S#1 LINEAR" );
	}
	else {
		sprintf( s_buffer, "S#1 %s", s_slot_type[ s ] );
	}
	tft_puts( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, SLOT1_INFO_X, SLOT1_INFO_Y, 0xFFFF, grp_font, s_buffer );
	//	SLOT#2
	s = BITS( d2, 5, 2 );
	if( s != 0 && BIT( d6, 1 ) != 0 ) {
		strcpy( s_buffer, "S#2 LINEAR" );
	}
	else {
		sprintf( s_buffer, "S#2 %s", s_slot_type[ s ] );
	}
	tft_puts( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, SLOT2_INFO_X, SLOT2_INFO_Y, 0xFFFF, grp_font, s_buffer );
	//	Master Volume
	s = BITS( d2, 0, 3 ) ^ 7;
	sprintf( s_buffer, "Vol  %s", s_volume[ s ] );
	tft_puts( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, MASTER_VOL_X, MASTER_VOL_Y, 0xFFFF, grp_font, s_buffer );
	//	PSG Volume
	s = BITS( d3, 2, 3 );
	sprintf( s_buffer, "PSG  %s", s_volume[ s ] );
	tft_puts( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, PSG_VOL_X, PSG_VOL_Y, 0xFFFF, grp_font, s_buffer );
	//	SCC+ Volume
	s = BITS( d4, 5, 3 );
	sprintf( s_buffer, "SCC  %s", s_volume[ s ] );
	tft_puts( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, SCC_VOL_X, SCC_VOL_Y, 0xFFFF, grp_font, s_buffer );
	//	OPLL Volume
	s = BITS( d3, 5, 3 );
	sprintf( s_buffer, "OPLL %s", s_volume[ s ] );
	tft_puts( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, OPLL_VOL_X, OPLL_VOL_Y, 0xFFFF, grp_font, s_buffer );
}

// --------------------------------------------------------------------
static void response_core( void ) {
	int msx_logo_state = 0;
	uint16_t *p_draw_buffer;

	tft_init();
	p_draw_buffer = buffer1;
	for(;;) {
		if( msx_logo_state < 128 ) {
			msx_logo_state = update_msx_logo( p_draw_buffer, msx_logo_state );
		}
		else {
			tft_copy( p_draw_buffer, IMAGE_WIDTH, IMAGE_HEIGHT, 0, 0, grp_indicator, 240, 135, 0, 0, 240, 135 );
			update_leds( p_draw_buffer );
			update_page1( p_draw_buffer );
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

	board_init();
	ps2dev_init();
	u2p_init();

	multicore_launch_core1( response_core );

	usb_init();
	for( ;; ) {
		tuh_task();
		ps2dev_task();
		u2p_task();
	}
	return 0;
}
