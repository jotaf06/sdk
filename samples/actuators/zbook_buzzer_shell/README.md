
# actuators/zbook_buzzer_shell

Real-hardware bring-up sample for `zbook_buzzer` (`interface/includes/actuators/zbook_buzzer.h`):
initializes the onboard piezo buzzer once at boot, then exposes `on`/`off`/`volume`/`frequency`/
`beep`/`beep_repeated`/`beep_preset`/`beep_full`/`music` as shell commands so a human can drive it
manually from the console.

No extra wiring needed — the buzzer (piezo + Q7 MOSFET on `BUZZER_GPIO`/GPIO32) is onboard the
P2.

## Build & flash

```sh
west build -b zbook@p2/rp2350b/m33 samples/actuators/zbook_buzzer_shell
west flash
```

Open a serial console (115200 baud), e.g. `screen /dev/ttyACM0 115200`.

## Commands

```
uart:~$ buzzer on
buzzer on
uart:~$ buzzer off
buzzer off
uart:~$ buzzer volume 50
buzzer volume set to 50%
uart:~$ buzzer frequency 440
buzzer frequency set to 440 Hz
uart:~$ buzzer beep 200
beeping for 200 ms...
uart:~$ buzzer beep_repeated 3
beeping 3 times...
uart:~$ buzzer beep_preset short
beeped (short)
uart:~$ buzzer beep_full 100 4
beeping 4 times, 100 ms each...
uart:~$ buzzer music
playing happy birthday...
```

`buzzer volume` takes 0-100 (loudest at 100, silent at 0). `buzzer frequency <hz>` retunes the tone
itself (the buzzer is passive). Both take effect immediately whether the buzzer is currently on or off.

`buzzer beep <duration_ms>` blocks the shell for the duration of the beep, at the last configured
frequency/volume. `buzzer beep_repeated <times>` beeps `times` times at the
`CONFIG_ZBOOK_BUZZER_BEEP_DURATION_NORMAL_MS` preset (150ms by default). `buzzer beep_preset
<short|normal|long>` beeps once at one of the three `CONFIG_ZBOOK_BUZZER_BEEP_DURATION_*_MS`
presets (50/150/400ms by default, tunable via menuconfig). `buzzer beep_full <duration_ms>
<times>` is the general form both of those build on: `times` beeps of `duration_ms` each, with a
`duration_ms` silent gap between beeps (none after the last). `buzzer music` plays "Happy
Birthday to You" (hardcoded note table in the sample, 120 BPM) by calling
`zbook_buzzer_set_frequency()` + `zbook_buzzer_beep_ms()` once per note — it's ~13 seconds and
blocks the shell the whole time.
