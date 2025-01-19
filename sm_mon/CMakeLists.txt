add_executable(sm_mon main.c)

pico_enable_stdio_uart(sm_mon 1)
pico_enable_stdio_usb(sm_mon 0)

target_link_libraries(sm_mon
	pico_bootsel_via_double_reset
	pico_stdlib
	driver_switch_matrix
	)

pico_add_extra_outputs(sm_mon)
