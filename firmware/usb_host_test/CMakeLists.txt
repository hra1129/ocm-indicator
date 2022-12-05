set(FAMILY rp2040)

add_executable( usb_host_test
	usb_host_test.c
)

target_include_directories( usb_host_test PRIVATE ${CMAKE_CURRENT_LIST_DIR} )
target_link_libraries( usb_host_test PRIVATE pico_stdlib pico_multicore tinyusb_host tinyusb_board )
pico_add_extra_outputs( usb_host_test )

# disable usb output, enable uart output
pico_enable_stdio_usb( usb_host_test 0 )
pico_enable_stdio_uart( usb_host_test 1 )

# add url via pico_set_program_url
example_auto_set_url( usb_host_test )
