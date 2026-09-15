cmake_minimum_required(VERSION 3.20.0)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../unit.cmake)
zbook_unit_test(spi)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/zbook_spi_loopback_emul.c)
