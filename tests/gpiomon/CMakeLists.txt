add_executable(gpiomon main.c)

pico_enable_stdio_uart(gpiomon 1)
pico_enable_stdio_usb(gpiomon 0)

target_link_libraries(gpiomon
	pico_bootsel_via_double_reset
	pico_stdlib
)

pico_add_extra_outputs(gpiomon)

# vim:set ts=4 sts=4 sw=4 tw=0 noet:
