add_executable(helloworld main.c
	ledarray.c
	)

pico_enable_stdio_uart(helloworld 1)
pico_enable_stdio_usb(helloworld 0)

target_compile_definitions(helloworld PUBLIC
	LEDARRAY_NUM=29
	LEDARRAY_PIN=0
	LEDARRAY_PIO=pio0
	)

target_link_libraries(helloworld
	pico_bootsel_via_double_reset
	pico_stdlib
	hardware_dma
	hardware_pio
	)

pico_add_extra_outputs(helloworld)
