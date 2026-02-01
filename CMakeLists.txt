cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(yuiop60pi C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_compile_definitions(
	PICO_DEFAULT_UART_TX_PIN=16
	PICO_DEFAULT_UART_RX_PIN=17
	PICO_DEFAULT_I2C_SDA_PIN=20
	PICO_DEFAULT_I2C_SCL_PIN=21
	ROTALY_ENCODER_1_PIN_A=2
	ROTALY_ENCODER_1_PIN_B=3
	ROTALY_ENCODER_1_COUNT=24
	WS2812_ARRAY_NUM=29
	WS2812_ARRAY_PIN=0
	WS2812_ARRAY_PIO=pio0
	)

add_subdirectory(libs)
add_subdirectory(tests)

# vim:set ts=8 sts=4 sw=4 tw=0 noet:
