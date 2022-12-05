cmake_minimum_required(VERSION 3.13)

set(FAMILY rp2040)

add_executable( sx2_indicator )

target_sources( sx2_indicator PUBLIC
	main.cpp
	tft_driver.cpp
	ps2dev_driver.cpp
	usb_host_driver.c
)

# Make sure TinyUSB can find tusb_config.h
target_include_directories( sx2_indicator PRIVATE 
	${CMAKE_CURRENT_LIST_DIR}
)

# pull in common dependencies
target_link_libraries( sx2_indicator PRIVATE 
	pico_stdlib
	hardware_spi
	hardware_dma
	hardware_gpio
	pico_multicore
	tinyusb_host
	tinyusb_board
)

# enable usb output, disable uart output
#pico_enable_stdio_usb(sx2_indicator 1)
#pico_enable_stdio_uart(sx2_indicator 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(sx2_indicator)
