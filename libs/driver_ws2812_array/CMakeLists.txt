add_library(driver_ws2812_array INTERFACE)

target_include_directories(driver_ws2812_array INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(driver_ws2812_array INTERFACE ws2812_array.c)

target_link_libraries(driver_ws2812_array INTERFACE
	hardware_dma
	hardware_pio
)

# vim:set ts=4 sts=4 sw=4 tw=0 noet:
