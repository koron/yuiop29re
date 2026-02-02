add_library(driver_rotary_encoder INTERFACE)

target_include_directories(driver_rotary_encoder INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(driver_rotary_encoder INTERFACE rotary_encoder.c)

target_link_libraries(driver_rotary_encoder INTERFACE hardware_gpio)

# vim:set ts=4 sts=4 sw=4 tw=0 noet:
