
# spi_pingpong

Real-hardware bring-up test for `zbook_spi` (`interface/includes/protocols/zbook_spi.h`) against
a **second, independent board**: the zbook (master) sends `"PING"` every 3 seconds and checks
the reply is `"PONG"`, echoed back by an RP2350A devkit (Raspberry Pi Pico 2) wired up as a
GPIO-bit-banged SPI slave.

Two separate Zephyr apps, built and flashed independently:

```
spi_pingpong/
├── zbook_master/      -- uses interface/'s zbook_spi API, board: zbook@p2/rp2350b/m33
└── rp2350a_slave/      -- pure Zephyr, no interface/ dependency, board: rpi_pico2/rp2350a/m33
```

## Why the slave is bit-banged, not real SPI hardware

Neither Zephyr SPI driver available for RP2350 in this workspace supports peripheral/slave mode:
`spi_pl022.c` ("Peripheral mode is not supported") and `spi_rpi_pico_pio.c` ("Slave mode not
supported") both hard-`return -ENOTSUP` for `SPI_OP_MODE_SLAVE`. So `rp2350a_slave/` shifts bits
by hand on GPIO edges instead — see its `src/main.c` for the mode-0/MSB-first bit-bang state
machine. This is why both sides run at a slow **10 kHz** (`zbook_master`'s `zbook_spi_cfg`) —
comfortably inside GPIO-interrupt latency on Cortex-M33, with plenty of margin to spare for a
4-byte frame every 3 seconds.

## The accelerometer, revisited

Unlike the old loopback sample, this one does **not** touch the onboard BMI323 accelerometer.
`spi1`'s `cs-gpios` is a list, and each child device's `reg` indexes into it — so
`zbook_master/boards/zbook_rp2350b_m33_p2.overlay` just adds a *second* chip-select
(GPIO39, header H1 pin 10) and a second child device (`zbook_spi@1`) on the same bus, alongside
`bmi323@0` on its original CS (GPIO09). Same SCK/MOSI/MISO, separate CS lines — standard SPI
multi-device sharing. The accelerometer keeps working.

## Wiring

| zbook (master)              | RP2350A devkit (slave) |
| ---------------------------- | ----------------------- |
| H1 pin 3 — `SPI_SCLK`        | GP2                      |
| H1 pin 13 — `SPI_MOSI`       | GP3                      |
| H1 pin 12 — `SPI_MISO`       | GP8                      |
| GPIO39 (not on H1 — wire directly from the pin) — `SPI_PINGPONG_CS` | GP9 |
| GND                           | GND                      |

GPIO39 isn't broken out on header H1, so that one wire has to come straight off the pin rather
than through the header. Don't forget a common ground between the two boards.

## Build & flash

Master (zbook):
```sh
west build -d build_master -b zbook@p2/rp2350b/m33 samples/spi_pingpong/zbook_master
west flash -d build_master
```

Slave (RP2350A devkit):
```sh
west build -d build_slave -b rpi_pico2/rp2350a/m33 samples/spi_pingpong/rp2350a_slave
west flash -d build_slave
```

Both overlays/board targets are picked up automatically from each app's own `CMakeLists.txt` /
`boards/` directory — no extra flags needed beyond `-b`. Flash the slave first (or at least
before the master's first PING) so it's already armed and listening.

Open a serial console on each board (115200 baud) to watch the exchange, e.g. `screen
/dev/ttyACM0 115200` (device path will differ per board once both are plugged in).

## Expected output

zbook (master):
```
zbook SPI ping-pong sample: sending PING every 3s, watch this console.
PING -> PONG  (slave alive)
PING -> PONG  (slave alive)
...
```

RP2350A devkit (slave):
```
zbook SPI ping-pong slave: waiting for PING, always answering PONG.
got PING -> replied PONG
got PING -> replied PONG
...
```

If the master instead prints `PING -> 00 00 00 00 (no/garbage reply)`, check the wiring table
above first (especially the CS wire, since it's not on the header) before suspecting `zbook_spi`
itself.
