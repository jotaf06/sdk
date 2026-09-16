# actuators/zbook_pwm_shell

Real-hardware bring-up sample for `zbook_pwm` (`interface/includes/actuators/zbook_pwm.h`):
initializes a single default channel ("dimmer") once at boot, then exposes `start`/`stop`/`set`/
`get` as shell commands so a human can drive it manually from the console.

`zbook_pwm` places no restriction on which pin is used or how many channels exist — that's
entirely up to this sample's own `boards/zbook_rp2350b_m33_p2.overlay`, which wires exactly one
channel, labeled `"dimmer"`, to header pin 5 (GPIO01, PWM slice 0 channel B) — one of the GPIO
Header's free/general-purpose pins. A different firmware picking a different pin (or more than
one) just needs its own overlay in the same shape; nothing in `interface/` or the board module
needs to change.

## Wiring

Header pin 5 (GPIO01) → current-limiting resistor (e.g. 220Ω) → LED anode; LED cathode → GND
(header pin 7, `+3V3`/`+5V` rows are the fixed-function rows -- GND is any board GND pin).

## Build & flash

```sh
west build -b zbook@p2/rp2350b/m33 samples/actuators/zbook_pwm_shell
west flash
```

Open a serial console (115200 baud), e.g. `screen /dev/ttyACM0 115200`.

## Commands

```
uart:~$ pwm set dimmer 200 0 normal
"dimmer" set to 200 Hz, 0% duty, normal
uart:~$ pwm start dimmer
"dimmer" started
uart:~$ pwm set dimmer 200 50 normal
"dimmer" set to 200 Hz, 50% duty, normal
uart:~$ pwm get dimmer
"dimmer": 200 Hz, 50% duty, normal
uart:~$ pwm stop dimmer
"dimmer" stopped
```

`pwm set <label> <freq_hz> <duty 0-100> <normal|inverted>` takes effect immediately whether the
channel is currently started or stopped -- if stopped, the LED stays off until the next `pwm
start`, but the frequency/duty/polarity are already applied for when it does. `pwm start`/`pwm
stop` only toggle the duty cycle between 0% and whatever `pwm set` last configured; the rest of
the config is kept across a stop. `pwm get` reads back the last config applied via `pwm set` (or
the default seeded at boot) -- not a live hardware read-back, since the PWM driver has none.

The channel is already initialized (`"dimmer"`) by the time the shell is up, per the boot log --
no `init` command is needed for it. A firmware overlay that adds more channels initializes each
of them itself, e.g. from its own `main()`, the same way this sample's does.
