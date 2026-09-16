/*
 * zenoh-pico on bare-metal RP2040/RP2350, over any serial_t.
 *
 * zenoh-pico needs two things from a platform: a system layer (clock,
 * random, malloc, sleep) and a link. This library is both, for the pico-sdk
 * without an RTOS -- and it takes the link from the caller, so what carries
 * the zenoh serial protocol is the application's choice: a USB CDC
 * interface (pico_cdc_serial), an RS485 transceiver, a PIO UART.
 *
 *     static cdc_serial_t zenoh_cdc;
 *     serial_t *link = cdc_serial_init(&zenoh_cdc, CDC_IDX_ZENOH);
 *     pico_zenoh_init(link);
 *
 * Then use zenoh-pico (or Pico-ROS, or easypicoros) normally, with a serial
 * locator: "serial/cdc#baudrate=921600". The device name in the locator is
 * ignored -- the link is the one you passed in -- and the baudrate is
 * meaningless over USB, but zenoh's locator parser wants both.
 *
 * SINGLE-THREADED ONLY (Z_FEATURE_MULTI_THREAD=0). With no scheduler to
 * yield to, zenoh's blocking waits would otherwise stop the firmware dead,
 * so every one of them calls serial_task() on the link instead. That is the
 * hook the whole design hangs on: the link's task() keeps USB (or whatever
 * else the application chains in with serial_hook.h) alive while zenoh
 * waits, and there is no second idle callback to register.
 */

#ifndef PICO_ZENOH_H
#define PICO_ZENOH_H

#include "pico_serial.h"

#ifdef __cplusplus
extern "C" {
#endif

// How long a read waits for the FIRST byte of a frame before telling zenoh
// "no data" -- which is what lets the single-threaded main loop keep
// turning. Short by default; raise it around a handshake, not in steady
// state.
#ifndef PICO_ZENOH_POLL_TIMEOUT_MS
#define PICO_ZENOH_POLL_TIMEOUT_MS 2
#endif

// How long a read waits for the REST of a frame once one has started, and
// how long a write pushes before giving up. Generous: a frame in flight
// must not be torn apart because the host stalled mid-packet.
#ifndef PICO_ZENOH_FRAME_TIMEOUT_MS
#define PICO_ZENOH_FRAME_TIMEOUT_MS 200
#endif

/*
 * Give zenoh its link. Call once, before opening a session. Returns false
 * if link is NULL.
 */
bool pico_zenoh_init(serial_t *link);

/*
 * Change the first-byte read timeout (see PICO_ZENOH_POLL_TIMEOUT_MS).
 *
 * A session handshake needs tens of milliseconds to answer, so a fresh link
 * gets a grace window at a longer timeout automatically -- you do not have
 * to raise this to connect, or to auto-reconnect. Raise it only to trade
 * main-loop responsiveness for fewer wakeups.
 */
void pico_zenoh_set_poll_timeout_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* PICO_ZENOH_H */
