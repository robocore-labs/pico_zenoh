/*
 * zenoh-pico serial link over the application's serial_t.
 *
 * The zenoh serial protocol (COBS framing + CRC32, serial_protocol.c) sits
 * on top of this byte transport. The framing layer reads one byte at a time
 * until the 0x00 COBS delimiter, so this layer can implement cooperative
 * blocking:
 *
 *   - at a frame boundary (last delivered byte was the 0x00 delimiter, or
 *     nothing delivered yet) a short timeout applies; returning 0 makes the
 *     framing layer report SIZE_MAX, which the datagram transport treats as
 *     "no data" -- the single-threaded main loop keeps running.
 *   - mid-frame a longer timeout applies so a frame in flight is not
 *     corrupted by an early bailout.
 *
 * While waiting, serial_task() on the link keeps the application alive --
 * USB, and anything it chained in with serial_hook.h.
 */

#include <string.h>

#include "pico/time.h"

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/transport/serial.h"
#include "zenoh-pico/utils/result.h"

#include "pico_zenoh.h"

#if Z_FEATURE_LINK_SERIAL == 1

extern serial_t *pico_zenoh_link(void);

// True when the next byte handed to the framing layer starts a new COBS frame.
static bool s_at_frame_boundary = true;

// Adjustable first-byte timeout: long during session setup (the INIT/ACK
// handshake response can take tens of ms), short during normal operation so
// a read never stalls the main loop.
static uint32_t s_poll_timeout_ms = PICO_ZENOH_POLL_TIMEOUT_MS;

void pico_zenoh_set_poll_timeout_ms(uint32_t ms) { s_poll_timeout_ms = ms; }

// After (re)opening the link, zenoh runs its INIT/ACK handshake, whose
// replies take far longer than the steady-state poll timeout. Give every
// fresh link a grace window with a generous first-byte timeout so
// auto-reconnect works without the application's involvement.
#define HANDSHAKE_GRACE_MS 5000
#define HANDSHAKE_POLL_TIMEOUT_MS 500
static absolute_time_t s_grace_until;

static uint32_t boundary_timeout_ms(void) {
    if (!time_reached(s_grace_until) && s_poll_timeout_ms < HANDSHAKE_POLL_TIMEOUT_MS) {
        return HANDSHAKE_POLL_TIMEOUT_MS;
    }
    return s_poll_timeout_ms;
}

z_result_t _z_serial_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    // The locator's device name and baudrate are ignored: the link is the
    // serial_t the application passed to pico_zenoh_init(), already open and
    // already configured. Both still have to parse, so "serial/cdc" or
    // "serial/uart0" read fine -- pick whichever names your wiring.
    (void)dev;
    (void)baudrate;
    if (pico_zenoh_link() == NULL) {
        return _Z_ERR_INVALID;  // pico_zenoh_init() was never called
    }
    s_at_frame_boundary = true;
    s_grace_until = make_timeout_time_ms(HANDSHAKE_GRACE_MS);
    sock->_open = true;
    return _Z_RES_OK;
}

z_result_t _z_serial_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    // Opening a port by pin number is the application's job, not zenoh's:
    // build the serial_t yourself and hand it to pico_zenoh_init().
    (void)sock;
    (void)txpin;
    (void)rxpin;
    (void)baudrate;
    return _Z_ERR_GENERIC;
}

z_result_t _z_serial_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    (void)sock;
    (void)dev;
    (void)baudrate;
    return _Z_ERR_GENERIC;
}

z_result_t _z_serial_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    (void)sock;
    (void)txpin;
    (void)rxpin;
    (void)baudrate;
    return _Z_ERR_GENERIC;
}

void _z_serial_close(_z_sys_net_socket_t *sock) { sock->_open = false; }

size_t _z_serial_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    serial_t *link = pico_zenoh_link();
    if (!sock._open || link == NULL || len == 0) {
        return 0;
    }

    uint32_t timeout_ms = s_at_frame_boundary ? boundary_timeout_ms() : PICO_ZENOH_FRAME_TIMEOUT_MS;
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);

    size_t n = 0;
    while (n < len) {
        uint32_t got = serial_read(link, &ptr[n], (uint32_t)(len - n));
        if (got > 0) {
            n += got;
            // Once a byte arrived, allow the full frame timeout for the rest.
            if (s_at_frame_boundary) {
                s_at_frame_boundary = false;
                deadline = make_timeout_time_ms(PICO_ZENOH_FRAME_TIMEOUT_MS);
            }
            continue;
        }
        if (time_reached(deadline)) {
            break;
        }
        serial_task(link);  // move bytes off the hardware, keep the app alive
    }

    if (n > 0 && ptr[n - 1] == 0x00) {
        // COBS frame delimiter delivered: next read starts a new frame.
        s_at_frame_boundary = true;
    }
    return n;
}

size_t _z_serial_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    serial_t *link = pico_zenoh_link();
    if (!sock._open || link == NULL) {
        return SIZE_MAX;
    }

    size_t written = 0;
    absolute_time_t deadline = make_timeout_time_ms(PICO_ZENOH_FRAME_TIMEOUT_MS);
    while (written < len) {
        written += serial_write(link, &ptr[written], (uint32_t)(len - written));
        if (written < len) {
            // A short write means the transport could not take the bytes --
            // no host attached, or its buffer is full. Drop the frame rather
            // than block, but keep the link object alive so the session can
            // recover later.
            if (time_reached(deadline)) {
                return SIZE_MAX;
            }
            serial_task(link);
        }
    }
    return written;
}

#endif /* Z_FEATURE_LINK_SERIAL == 1 */
