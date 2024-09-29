add_executable(re_mon main.c)

pico_enable_stdio_uart(re_mon 1)
pico_enable_stdio_usb(re_mon 0)

target_link_libraries(re_mon
	pico_bootsel_via_double_reset
	pico_stdlib
	driver_rotary_encoder
	)

pico_add_extra_outputs(re_mon)
