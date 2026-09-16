# Registers pico_zenoh's platform profile with zenoh-pico.
#
# zenoh-pico clears ZP_PLATFORM_DIRS early in its own CMakeLists, so an
# out-of-tree profile cannot be handed over in a variable. The supported
# hook is ZP_EXTERNAL_PACKAGES: zenoh-pico find_package()s each name listed
# there before it looks for a profile, and zp_add_platform_dir() called from
# the package config lands in the right scope.
#
# pico_zenoh/CMakeLists.txt points pico_zenoh_platform_DIR at this file's
# directory -- which is also where pico_zenoh.cmake, the profile itself,
# lives. Nothing is patched into the zenoh-pico tree.
zp_add_platform_dir("${CMAKE_CURRENT_LIST_DIR}")
