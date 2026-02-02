add_library(driver_switch_matrix INTERFACE)

target_include_directories(driver_switch_matrix INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(driver_switch_matrix INTERFACE switch_matrix.c)

target_link_libraries(driver_switch_matrix INTERFACE hardware_gpio)

# vim:set ts=4 sts=4 sw=4 tw=0 noet:
