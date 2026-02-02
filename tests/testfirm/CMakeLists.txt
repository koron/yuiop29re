add_executable(testfirm
	main.c
	ssd1306.c
)

pico_enable_stdio_uart(testfirm 1)
pico_enable_stdio_usb(testfirm 0)

target_link_libraries(testfirm
	pico_bootsel_via_double_reset
	pico_stdlib
	hardware_i2c
	driver_rotary_encoder
	driver_switch_matrix
	driver_ws2812_array
)

pico_add_extra_outputs(testfirm)

# vim:set ts=4 sts=4 sw=4 tw=0 noet:
