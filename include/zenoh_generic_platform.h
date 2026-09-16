/*
 * Bare-metal RP2040/RP2350 platform header for zenoh-pico (ZENOH_GENERIC).
 *
 * Single-threaded build (Z_FEATURE_MULTI_THREAD=0) on the pico-sdk, no
 * RTOS. The only link type is the zenoh serial protocol, carried over
 * whatever serial_t the application handed to pico_zenoh_init() (see
 * src/serial.c).
 */

#ifndef ZENOH_GENERIC_PLATFORM_PICO_ZENOH_H
#define ZENOH_GENERIC_PLATFORM_PICO_ZENOH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_MULTI_THREAD == 1
#error "pico_zenoh is single-threaded only (set Z_FEATURE_MULTI_THREAD=0)"
#endif

/* Monotonic clock: time since boot, timespec-shaped because consumers
 * (picoros among them) read tv_sec/tv_nsec directly. */
typedef struct {
    int64_t tv_sec;
    int64_t tv_nsec;
} z_clock_t;

/* Wall time: microseconds since boot (no RTC on the board). */
typedef uint64_t z_time_t;

/* The only "socket" is the application's serial link; no state needed
 * beyond a validity marker. */
typedef struct {
    bool _open;
} _z_sys_net_socket_t;

typedef struct {
    uint8_t _dummy;
} _z_sys_net_endpoint_t;

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_GENERIC_PLATFORM_PICO_ZENOH_H */
