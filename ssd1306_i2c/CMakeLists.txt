add_executable(ssd1306_i2c
        ssd1306_i2c.c
        )

# pull in common dependencies and additional i2c hardware support
target_link_libraries(ssd1306_i2c
	pico_bootsel_via_double_reset
	pico_stdlib
	hardware_i2c)

# create map/bin/hex file etc.
pico_add_extra_outputs(ssd1306_i2c)
