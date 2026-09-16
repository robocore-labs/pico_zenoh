# zenoh-pico platform profile for pico_zenoh (pico-sdk, bare metal).
#
# Loaded by zenoh-pico when ZP_PLATFORM=pico_zenoh; found because
# pico_zenoh's CMakeLists.txt appends its cmake/ directory to
# ZP_PLATFORM_DIRS before add_subdirectory(). Nothing is patched into the
# zenoh-pico tree.
set(ZP_PLATFORM_SYSTEM_LAYER pico_zenoh)
set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_GENERIC)
set(ZP_PLATFORM_SOURCE_FILES ${PICO_ZENOH_PLATFORM_SOURCES})
set(ZP_PLATFORM_INCLUDE_DIRS ${PICO_ZENOH_PLATFORM_INCLUDES})
set(CHECK_THREADS OFF)
