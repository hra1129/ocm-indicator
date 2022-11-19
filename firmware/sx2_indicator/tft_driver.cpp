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

#include <stdlib.h>

#define ST7789_DRIVER
#define TFT_WIDTH				135
#define TFT_HEIGHT				240

#define TFT_RESET				0
#define TFT_RS					1
#define TFT_SPI_CLK				2
#define TFT_SPI_TX				3
#define TFT_LEDK				4
#define TFT_SPI_CS				5
#define TFT_PWR_EN				22

#define SPI_PORT				spi0
#define SPI_FREQUENCY			40000000
#define SPI_READ_FREQUENCY		20000000
#define SPI_TOUCH_FREQUENCY		2500000

// ST7789 specific commands used in init
#define ST7789_NOP			0x00
#define ST7789_SWRESET		0x01
#define ST7789_RDDID		0x04
#define ST7789_RDDST		0x09

#define ST7789_RDDPM		0x0A      // Read display power mode
#define ST7789_RDD_MADCTL	0x0B      // Read display MADCTL
#define ST7789_RDD_COLMOD	0x0C      // Read display pixel format
#define ST7789_RDDIM		0x0D      // Read display image mode
#define ST7789_RDDSM		0x0E      // Read display signal mode
#define ST7789_RDDSR		0x0F      // Read display self-diagnostic result (ST7789V)

#define ST7789_SLPIN		0x10
#define ST7789_SLPOUT		0x11
#define ST7789_PTLON		0x12
#define ST7789_NORON		0x13

#define ST7789_INVOFF		0x20
#define ST7789_INVON		0x21
#define ST7789_GAMSET		0x26      // Gamma set
#define ST7789_DISPOFF		0x28
#define ST7789_DISPON		0x29
#define ST7789_CASET		0x2A
#define ST7789_RASET		0x2B
#define ST7789_RAMWR		0x2C
#define ST7789_RGBSET		0x2D      // Color setting for 4096, 64K and 262K colors
#define ST7789_RAMRD		0x2E

#define ST7789_PTLAR		0x30
#define ST7789_VSCRDEF		0x33      // Vertical scrolling definition (ST7789V)
#define ST7789_TEOFF		0x34      // Tearing effect line off
#define ST7789_TEON			0x35      // Tearing effect line on
#define ST7789_MADCTL		0x36      // Memory data access control
#define ST7789_VSCRSADD		0x37      // Vertical screoll address
#define ST7789_IDMOFF		0x38      // Idle mode off
#define ST7789_IDMON		0x39      // Idle mode on
#define ST7789_RAMWRC		0x3C      // Memory write continue (ST7789V)
#define ST7789_RAMRDC		0x3E      // Memory read continue (ST7789V)
#define ST7789_COLMOD		0x3A

#define ST7789_RAMCTRL		0xB0      // RAM control
#define ST7789_RGBCTRL		0xB1      // RGB control
#define ST7789_PORCTRL		0xB2      // Porch control
#define ST7789_FRCTRL1		0xB3      // Frame rate control
#define ST7789_PARCTRL		0xB5      // Partial mode control
#define ST7789_GCTRL		0xB7      // Gate control
#define ST7789_GTADJ		0xB8      // Gate on timing adjustment
#define ST7789_DGMEN		0xBA      // Digital gamma enable
#define ST7789_VCOMS		0xBB      // VCOMS setting
#define ST7789_LCMCTRL		0xC0      // LCM control
#define ST7789_IDSET		0xC1      // ID setting
#define ST7789_VDVVRHEN		0xC2      // VDV and VRH command enable
#define ST7789_VRHS			0xC3      // VRH set
#define ST7789_VDVSET		0xC4      // VDV setting
#define ST7789_VCMOFSET		0xC5      // VCOMS offset set
#define ST7789_FRCTR2		0xC6      // FR Control 2
#define ST7789_CABCCTRL		0xC7      // CABC control
#define ST7789_REGSEL1		0xC8      // Register value section 1
#define ST7789_REGSEL2		0xCA      // Register value section 2
#define ST7789_PWMFRSEL		0xCC      // PWM frequency selection
#define ST7789_PWCTRL1		0xD0      // Power control 1
#define ST7789_VAPVANEN		0xD2      // Enable VAP/VAN signal output
#define ST7789_CMD2EN		0xDF      // Command 2 enable
#define ST7789_PVGAMCTRL	0xE0      // Positive voltage gamma control
#define ST7789_NVGAMCTRL	0xE1      // Negative voltage gamma control
#define ST7789_DGMLUTR		0xE2      // Digital gamma look-up table for red
#define ST7789_DGMLUTB		0xE3      // Digital gamma look-up table for blue
#define ST7789_GATECTRL		0xE4      // Gate control
#define ST7789_SPI2EN		0xE7      // SPI2 enable
#define ST7789_PWCTRL2		0xE8      // Power control 2
#define ST7789_EQCTRL		0xE9      // Equalize time control
#define ST7789_PROMCTRL		0xEC      // Program control
#define ST7789_PROMEN		0xFA      // Program mode enable
#define ST7789_NVMSET		0xFC      // NVM setting
#define ST7789_PROMACT		0xFE      // Program action

#define TFT_MAD_RGB 0x00
#define TFT_MAD_BGR 0x08
#define TFT_MAD_COLOR_ORDER	TFT_MAD_BGR



#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"

static int32_t					dma_tx_channel;
static dma_channel_config		dma_tx_config;

#define _send_8bits(B)			while( !spi_is_writable( SPI_PORT ) ); \
								spi_get_hw( SPI_PORT )->dr = (uint8_t)( B )

//	RSTX
#define _tft_reset()			gpio_put( TFT_RESET, 0 )
#define _tft_active()			gpio_put( TFT_RESET, 1 )

//	DCX
#define _set_command_mode()		gpio_put( TFT_RS, 0 )
#define _set_data_mode()		gpio_put( TFT_RS, 1 )

//	CSX
static int chip_select_count = 0;

//	LEDK
#define _tft_backlight_on()		gpio_put( TFT_LEDK, 1 )
#define _tft_backlight_off()	gpio_put( TFT_LEDK, 0 )

#define _wait_ready(s)			while( spi_get_hw(s)->sr & SPI_SSPSR_BSY_BITS )

// --------------------------------------------------------------------
static void _chip_select( void ) {

	if( chip_select_count == 0 ) {
		gpio_put( TFT_SPI_CS, 0 );
	}
	chip_select_count++;
}

// --------------------------------------------------------------------
static void _chip_deselect( void ) {

	chip_select_count--;
	if( chip_select_count == 0 ) {
		gpio_put( TFT_SPI_CS, 1 );
	}
}

// --------------------------------------------------------------------
//	Send command
static void _send_command( uint16_t command, bool with_sleep = true ) {

	_chip_select();
	_set_command_mode();
	spi_write_blocking( SPI_PORT, (const uint8_t*) &command, 1 );
	_set_data_mode();
	_chip_deselect();
	if( with_sleep ) {
		sleep_us( 70 );
	}
}

// --------------------------------------------------------------------
//	Send data
static void _send_data( uint8_t data, bool with_sleep = true ) {

	_chip_select();
	_set_data_mode();
	spi_write_blocking( SPI_PORT, &data, 1 );
	_chip_deselect();
	if( with_sleep ) {
		sleep_us( 70 );
	}
}

// --------------------------------------------------------------------
//	DMA busy check
static bool _is_dma_busy( void ) {

	if( dma_channel_is_busy( dma_tx_channel ) ) {
		return true;
	}
	_wait_ready( SPI_PORT );
	hw_write_masked( &spi_get_hw( SPI_PORT )->cr0, (16 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS );
	return false;
}

// --------------------------------------------------------------------
//	DMA wait
static bool _wait_dma_ready( void ) {

	while( dma_channel_is_busy( dma_tx_channel ) );
	_wait_ready( SPI_PORT );
	hw_write_masked( &spi_get_hw( SPI_PORT )->cr0, (16 - 1) << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS );
	return false;
}

// --------------------------------------------------------------------
bool tft_init( void ) {

	spi_init( SPI_PORT, SPI_FREQUENCY );

	//	setup SPI MODE3 (CPOL=1, CPHA=1), MSB first
//	spi_set_format( SPI_PORT,  8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST );
	spi_set_format( SPI_PORT,  8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST );

	gpio_set_function( TFT_SPI_CLK, GPIO_FUNC_SPI );
	gpio_set_function( TFT_SPI_TX, GPIO_FUNC_SPI );

	dma_tx_channel = dma_claim_unused_channel( false );
	if( dma_tx_channel < 0 ) {
		return false;	//	error.
	}
	dma_tx_config = dma_channel_get_default_config( dma_tx_channel );
	channel_config_set_transfer_data_size( &dma_tx_config, DMA_SIZE_16 );
	channel_config_set_dreq( &dma_tx_config, spi_get_index( SPI_PORT ) ? DREQ_SPI1_TX : DREQ_SPI0_TX );

	gpio_init( TFT_RESET );
	gpio_init( TFT_SPI_CS );
	gpio_init( TFT_RS );
	gpio_init( TFT_LEDK );
	gpio_init( TFT_PWR_EN );
	gpio_set_dir( TFT_RESET, GPIO_OUT );
	gpio_set_dir( TFT_SPI_CS, GPIO_OUT );
	gpio_set_dir( TFT_RS, GPIO_OUT );
	gpio_set_dir( TFT_LEDK, GPIO_OUT );
	gpio_set_dir( TFT_PWR_EN, GPIO_OUT );

	gpio_put( TFT_SPI_CS, 1 );
	_set_data_mode();
	_tft_backlight_on();

	_tft_active();
	sleep_ms( 1 );
	gpio_put( TFT_PWR_EN, 1 );
	sleep_ms( 5 );
	_tft_reset();
	sleep_ms( 20 );
	_tft_active();
	sleep_ms( 150 );

	// ---------------- initialize command
	_send_command( ST7789_SLPOUT );		// Sleep out
	sleep_ms( 120 );
	_send_command( ST7789_NORON );		// Normal display mode on

	// ---------------- display and color format setting
	_send_command( ST7789_MADCTL );
	_send_data( TFT_MAD_COLOR_ORDER );

	// JLX240 display datasheet
	_send_command( 0xB6 );
	_send_data( 0x0A );
	_send_data( 0x82 );

	_send_command( ST7789_RAMCTRL );
	_send_data( 0x00 );
	_send_data( 0xE0 );					// 5 to 6 bit conversion: r0 = r5, b0 = b5

	_send_command( ST7789_COLMOD );
	_send_data( 0x55 );
	sleep_ms( 10 );

	// -------------------------------ST7789V Frame rate setting
	_send_command( ST7789_PORCTRL );
	_send_data( 0x0c );
	_send_data( 0x0c );
	_send_data( 0x00 );
	_send_data( 0x33 );
	_send_data( 0x33 );

	_send_command( ST7789_GCTRL );		// Voltages: VGH / VGL
	_send_data( 0x35 );

	//---------------------------------ST7789V Power setting
	_send_command( ST7789_VCOMS );
	_send_data( 0x28 );					// JLX240 display datasheet

	_send_command(ST7789_LCMCTRL);
	_send_data(0x0C);

	_send_command( ST7789_VDVVRHEN );
	_send_data( 0x01 );
	_send_data( 0xFF );

	_send_command( ST7789_VRHS );		// voltage VRHS
	_send_data( 0x10 );

	_send_command( ST7789_VDVSET );
	_send_data( 0x20 );

	_send_command( ST7789_FRCTR2 );
	_send_data( 0x0f );

	_send_command( ST7789_PWCTRL1 );
	_send_data( 0xa4 );
	_send_data( 0xa1 );

	// -------------------------------ST7789V gamma setting
	_send_command( ST7789_PVGAMCTRL );
	_send_data( 0xd0 );
	_send_data( 0x00 );
	_send_data( 0x02 );
	_send_data( 0x07 );
	_send_data( 0x0a );
	_send_data( 0x28 );
	_send_data( 0x32 );
	_send_data( 0x44 );
	_send_data( 0x42 );
	_send_data( 0x06 );
	_send_data( 0x0e );
	_send_data( 0x12 );
	_send_data( 0x14 );
	_send_data( 0x17 );

	_send_command( ST7789_NVGAMCTRL );
	_send_data( 0xd0 );
	_send_data( 0x00 );
	_send_data( 0x02 );
	_send_data( 0x07 );
	_send_data( 0x0a );
	_send_data( 0x28 );
	_send_data( 0x31 );
	_send_data( 0x54 );
	_send_data( 0x47 );
	_send_data( 0x0e );
	_send_data( 0x1c );
	_send_data( 0x17 );
	_send_data( 0x1b );
	_send_data( 0x1e );

	//_send_command( ST7789_INVON );
	_send_command( ST7789_INVOFF );

	_send_command( ST7789_CASET );		// Column address set
	_send_data( 0x00 );
	_send_data( 0x00 );
	_send_data( 0x00 );
	_send_data( 0xE5 );    // 239

	_send_command( ST7789_RASET );		// Row address set
	_send_data( 0x00 );
	_send_data( 0x00 );
	_send_data( 0x01 );
	_send_data( 0x3F );    // 319
	sleep_ms( 120 );

	_send_command( ST7789_DISPON );		// Display on
	sleep_ms( 120 );
	return true;
}

// --------------------------------------------------------------------
void tft_send_framebuffer( const uint16_t *p_buffer ) {
	int i, d;
	//	const uint32_t len = TFT_WIDTH * TFT_HEIGHT;

//	_wait_dma_ready();
//	dma_channel_configure( dma_tx_channel, &dma_tx_config, &spi_get_hw( SPI_PORT )->dr, p_buffer, len, true );

	_send_command( ST7789_MADCTL );
	_send_data( TFT_MAD_COLOR_ORDER );

	_send_command( ST7789_MADCTL );
	_send_data( 0x68 );					// ??

	_chip_select();
	_send_command( ST7789_CASET, false );
	_send_data( 0x00, false );					// ??
	_send_data( 0x28, false );					// ??
	_send_data( 0x01, false );					// ??
	_send_data( 0x17, false );					// ??

	_send_command( ST7789_RASET, false );
	_send_data( 0x00, false );					// ??
	_send_data( 0x35, false );					// ??
	_send_data( 0x00, false );					// ??
	_send_data( 0xBB, false );					// ??

	_send_command( ST7789_RAMWR, false );

	for( i = 0; i < 240 * 320; i++ ) {
		d = rand();
		_send_data( d >> 8, false );					// ??
		_send_data( d & 255, false );					// ??
	}
	_chip_deselect();
}
