# io/zbook_gpio_shell

Real-hardware bring-up sample for `zbook_gpio` (`interface/includes/io/zbook_gpio.h`): exposes
`init`/`configure`/`write`/`read`/`watch`/`unwatch` as shell commands, addressed by IO name, so a
human can drive any of zbook's GPIO-capable header IOs manually from the console.

Which pins are available is fixed by `interface/`'s own `zbook-sdk` snippet
(`interface/snippets/zbook-sdk/`), same as `zbook_pwm`. This sample doesn't declare an overlay of
its own — it just requests the snippet (see `sample.yaml`'s `required_snippets`, or `-S zbook-sdk`
below), which wires up all four of ZBook's flexible header IOs (`enum zbook_gpio_io` in
`zbook_gpio.h`) as GPIO channels. Those same pins are also selectable as PWM/ADC/UART-PIO by other
`zbook_<periph>` interfaces — only one interface should claim a given pin at a time.

## Wiring

GPIO Header (`H1`, 2x7):

| Header pin | Signal | `zbook_gpio_io` |
| ---------- | ------ | ---------------- |
| 5          | GPIO01 | `ZBOOK_GPIO_IO01` / `"io01"` |
| 10         | GPIO39 | `ZBOOK_GPIO_IO39` / `"io39"` |
| 6          | GPIO45 | `ZBOOK_GPIO_IO45` / `"io45"` |
| 9          | GPIO46 | `ZBOOK_GPIO_IO46` / `"io46"` |
| 2, 4, 11   | GND    | -- |
| 7          | +5V    | -- |
| 8          | +3V3   | -- |

To validate output: wire header pin → current-limiting resistor (e.g. 220Ω) → LED anode; LED
cathode → any GND pin. To validate input/interrupt: wire header pin to a button/switch that pulls
it to GND, and configure with `up` pull.

## Build & flash

```sh
west build -b zbook@p2/rp2350b/m33 samples/io/zbook_gpio_shell -S zbook-sdk
west flash
```

Open a serial console (115200 baud), e.g. `screen /dev/ttyACM0 115200`.

## Commands

```
uart:~$ gpio init io01
"io01" initialized
uart:~$ gpio configure io01 out none high
"io01" configured as out, pull none, active high
uart:~$ gpio write io01 1
"io01" set to 1
uart:~$ gpio configure io01 in up low
"io01" configured as in, pull up, active low
uart:~$ gpio read io01
"io01": 0
uart:~$ gpio watch io01 falling
watching "io01" for falling edges
"io01" edge triggered
uart:~$ gpio unwatch io01
"io01" no longer watched
```

`gpio init <io>` must run once per IO before any other command on it (`<io>` is one of `io01`/
`io39`/`io45`/`io46`). `gpio init` leaves the IO as an input, no pull, active-high — call `gpio
configure` to change direction/pull/active level, or before writing. `active low` means a
physical low level reads as logical 1 (asserted) — useful for a switch wired to pull the pin to
GND. `gpio watch` registers a callback that prints directly from interrupt context whenever the
IO transitions the requested edge; `gpio unwatch`
stops it.
