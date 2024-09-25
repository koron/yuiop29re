add_executable(ssd1306_i2c
        ssd1306_i2c.c
        )

target_compile_definitions(ssd1306_i2c PUBLIC
	PICO_DEFAULT_UART_TX_PIN=16
	PICO_DEFAULT_UART_RX_PIN=17
	PICO_DEFAULT_I2C_SDA_PIN=20
	PICO_DEFAULT_I2C_SCL_PIN=21
	)

# pull in common dependencies and additional i2c hardware support
target_link_libraries(ssd1306_i2c pico_stdlib hardware_i2c)

# create map/bin/hex file etc.
pico_add_extra_outputs(ssd1306_i2c)
