add_executable(helloworld main.c)

pico_enable_stdio_uart(helloworld 1)
pico_enable_stdio_usb(helloworld 0)

target_compile_definitions(helloworld PUBLIC
	PICO_DEFAULT_UART_TX_PIN=16
	PICO_DEFAULT_UART_RX_PIN=17
	)

target_link_libraries(helloworld
	pico_bootsel_via_double_reset
	pico_stdlib
	)

pico_add_extra_outputs(helloworld)
