# zenoh-pico on bare-metal RP2040/RP2350, over any serial_t.
#
# This library is zenoh-pico's platform port (system layer + serial link),
# and it builds zenoh-pico itself so a firmware does not have to know how.
# Point it at a zenoh-pico checkout and link pico_zenoh:
#
#     set(PICO_ZENOH_ZENOH_PICO_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/zenoh-pico)
#     add_subdirectory(link-libraries/pico_serial  pico_serial)
#     add_subdirectory(link-libraries/pico_zenoh   pico_zenoh)
#     target_link_libraries(my_firmware pico_zenoh)
#
# The Z_FEATURE_* cache variables do NOT apply: this is a ZENOH_GENERIC
# build, where include/zenoh_generic_config.h is the entire configuration.
# To change it, copy that file and set PICO_ZENOH_CONFIG_DIR to your copy.

if(NOT PICO_ZENOH_ZENOH_PICO_DIR)
    message(FATAL_ERROR
        "pico_zenoh: set PICO_ZENOH_ZENOH_PICO_DIR to a zenoh-pico checkout "
        "before add_subdirectory(pico_zenoh).")
endif()

set(PICO_ZENOH_CONFIG_DIR "${CMAKE_CURRENT_LIST_DIR}/include" CACHE PATH
    "Directory containing zenoh_generic_config.h (copy and edit to reconfigure zenoh)")

# Compiled into zenohpico_static by the platform profile, not into a library
# of our own -- the port IS zenoh-pico's platform layer.
set(PICO_ZENOH_PLATFORM_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/system.c
    ${CMAKE_CURRENT_LIST_DIR}/src/serial.c
    CACHE INTERNAL "")
set(PICO_ZENOH_PLATFORM_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${PICO_ZENOH_CONFIG_DIR}
    CACHE INTERNAL "")

# Where zenoh-pico looks for the profile named by ZP_PLATFORM. It clears
# ZP_PLATFORM_DIRS itself, so the profile directory is registered through
# the external-package hook instead -- see cmake/pico_zenoh_platformConfig.cmake.
set(ZP_EXTERNAL_PACKAGES "pico_zenoh_platform" CACHE STRING "" FORCE)
set(pico_zenoh_platform_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake" CACHE PATH "" FORCE)
set(ZP_PLATFORM "pico_zenoh" CACHE STRING "" FORCE)
set(BUILD_SHARED_LIBS OFF)

# Kept in step with zenoh_generic_config.h: under ZENOH_GENERIC these no
# longer configure anything, but zenoh-pico's CMakeLists still uses a few of
# them to decide which link sources to compile.
set(Z_FEATURE_MULTI_THREAD 0 CACHE STRING "" FORCE)
set(Z_FEATURE_LINK_SERIAL 1 CACHE STRING "" FORCE)
set(Z_FEATURE_LINK_SERIAL_USB 0 CACHE STRING "" FORCE)
set(Z_FEATURE_LINK_TCP 0 CACHE STRING "" FORCE)
set(Z_FEATURE_LINK_UDP_MULTICAST 0 CACHE STRING "" FORCE)
set(Z_FEATURE_LINK_UDP_UNICAST 0 CACHE STRING "" FORCE)
set(Z_FEATURE_LINK_WS 0 CACHE STRING "" FORCE)
set(Z_FEATURE_LINK_TLS 0 CACHE STRING "" FORCE)
set(Z_FEATURE_SCOUTING 0 CACHE STRING "" FORCE)
set(Z_FEATURE_MULTICAST_TRANSPORT 0 CACHE STRING "" FORCE)
set(Z_FEATURE_RAWETH_TRANSPORT 0 CACHE STRING "" FORCE)

add_subdirectory(${PICO_ZENOH_ZENOH_PICO_DIR} zenoh-pico)

# The port's own sources are compiled inside zenohpico_static, so that is
# where their dependencies belong.
target_link_libraries(zenohpico_static pico_stdlib pico_rand pico_serial)

# What a firmware links: the public header, and zenoh-pico behind it.
add_library(pico_zenoh INTERFACE)
target_include_directories(pico_zenoh INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(pico_zenoh INTERFACE zenohpico::lib pico_serial)
