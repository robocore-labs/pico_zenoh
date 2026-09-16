# pico_zenoh

[zenoh-pico](https://github.com/eclipse-zenoh/zenoh-pico) on bare-metal
RP2040/RP2350, over any [`serial_t`](../pico_serial).

zenoh-pico needs two things from a platform: a system layer (clock, random,
malloc, sleep) and a link. This library is both — for the pico-sdk with no
RTOS — and it takes the link from the caller, like every other library here.
What carries the zenoh serial protocol is then the robot's decision, not
zenoh's: a USB CDC interface, an RS485 transceiver, a PIO UART.

```cmake
set(PICO_ZENOH_ZENOH_PICO_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/zenoh-pico)

add_subdirectory(path/to/link-libraries/pico_serial      pico_serial)
add_subdirectory(path/to/link-libraries/pico_cdc_serial  pico_cdc_serial)
add_subdirectory(path/to/link-libraries/pico_zenoh       pico_zenoh)

target_link_libraries(my_firmware pico_zenoh pico_cdc_serial)
```

```c
#include "cdc_serial.h"
#include "pico_zenoh.h"

static cdc_serial_t zenoh_cdc;

serial_t *link = cdc_serial_init(&zenoh_cdc, CDC_IDX_ZENOH);
pico_zenoh_init(link);
```

From there use zenoh-pico normally — or [Pico-ROS](https://github.com/Pico-ROS/Pico-ROS-software),
or [easypicoros](https://github.com/robocore-labs/easyp) — with a serial
locator, `"serial/cdc#baudrate=921600"`. The device name is ignored (the
link is the one you passed in) and the baudrate means nothing over USB, but
zenoh's locator parser wants both, so name it after your wiring.

`pico_zenoh` builds zenoh-pico itself: point `PICO_ZENOH_ZENOH_PICO_DIR` at
a checkout and the platform profile, the config header and the port are
wired up for you. Nothing is patched into the zenoh-pico tree.

## Single-threaded, and what that costs

This is a `Z_FEATURE_MULTI_THREAD=0` build: no tasks, no mutexes, no
background rx thread. zenoh runs when you call it, from your main loop.

Which means every blocking wait inside zenoh would be dead time for the rest
of the firmware — and zenoh waits often: for the first byte of a frame, for
the rest of a frame, for a handshake reply, for a reconnect backoff. So they
don't block. Each one calls `serial_task()` on the link, which is
`pico_serial`'s hook for exactly this: it drains the port, and runs whatever
the application chained in with
[`serial_hook.h`](../pico_serial#task-is-where-usb-stays-alive).

There is no idle callback to register. The link *is* the hook:

```c
static void io_poll(void) { tud_task(); lidar_bridge(); }

static cdc_serial_t zenoh_cdc;
static serial_hook_t zenoh_hook;

serial_t *link = serial_hook_init(&zenoh_hook,
                                  cdc_serial_init(&zenoh_cdc, CDC_IDX_ZENOH),
                                  io_poll);
pico_zenoh_init(link);
```

Now the lidar keeps streaming while zenoh waits on a router that isn't up
yet. The rule from `serial_hook.h` applies: the hook must not call back into
zenoh, and must not `serial_task()` the wrapper it is attached to.

## Timeouts

| Constant | Default | What it bounds |
|---|---|---|
| `PICO_ZENOH_POLL_TIMEOUT_MS` | 2 ms | Waiting for the **first** byte of a frame. This is what keeps your main loop turning: when it expires, zenoh is told "no data" and control returns to you. |
| `PICO_ZENOH_FRAME_TIMEOUT_MS` | 200 ms | Waiting for the **rest** of a frame, and pushing a write. Generous on purpose: a frame in flight must not be torn apart because the host stalled mid-packet. |

`pico_zenoh_set_poll_timeout_ms()` changes the first at runtime — but you
don't need it for connecting. A freshly opened link gets a 5-second grace
window at a longer timeout automatically, so the INIT/ACK handshake
completes and auto-reconnect works without the application's involvement.
Raise it only to trade main-loop responsiveness for fewer wakeups.

## Configuration

Under `ZENOH_GENERIC`, zenoh-pico's generated `config.h` delegates
everything to one header, so the `Z_FEATURE_*` CMake variables do not apply:
[`include/zenoh_generic_config.h`](include/zenoh_generic_config.h) is the
whole configuration. The defaults are client mode, serial link only,
publication/subscription/query on, scouting and every IP link off, and
`rmw_zenoh`-compatible lease settings.

To change any of it, copy that file into your firmware and say where:

```cmake
set(PICO_ZENOH_CONFIG_DIR ${CMAKE_CURRENT_LIST_DIR}/src/zenoh_config)
```

## What is not here

**The ROS layer.** Pico-ROS (`picoros` + `picoserdes`) and
[easypicoros](https://github.com/robocore-labs/easyp) sit on top of this and
are vendored by the firmware; this library is zenoh and nothing above it.

**USB.** The link is a `serial_t`, so a CDC port is
[`pico_cdc_serial`](../pico_cdc_serial)'s job — and a zenoh link over RS485
or a PIO UART needs no change here at all.
