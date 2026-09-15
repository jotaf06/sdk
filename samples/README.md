
# samples

Bring-up firmware for validating `interface/` peripherals on **real ZBook hardware**. These are
not `ztest` suites and are not exercised functionally by `twister` (see `interface/tests/` for
that) — each sample just runs forever on the target and prints PASS/FAIL-style output to the
console for a human to read while probing/wiring the board.

## Layout

Single-firmware sample, nested under its `interface/` category (mirrors
`interface/includes/<category>/`, `interface/src/<category>/`):

```
samples/
└── <category>/
    └── <periph>/  (or "zbook_<periph>_<scenario>", ...)
        ├── CMakeLists.txt
        ├── prj.conf
        ├── sample.yaml     # build_only: true -- twister can compile it for the real board,
        │                   # but a human still has to judge PASS/FAIL from the console
        ├── README.md        # wiring + build/flash + expected output for this sample
        └── src/
            └── main.c
```

`zbook_spi_pingpong/` predates this convention and stays flat at `samples/zbook_spi_pingpong/` rather than
moving under `samples/protocols/` — not touched as part of introducing the nested layout.

A sample that needs a second, independent board (e.g. a peripheral acting as an external SPI
device) is instead a directory of its own per firmware, each a full Zephyr app targeting its own
board, with one README covering the pair — see `zbook_spi_pingpong/`. Only the app that targets
`zbook@*` calls into `interface/`; a peer-board app (e.g. `zbook_spi_pingpong/rp2350a_slave/`) is pure
Zephyr and never links against it.

Every zbook-side app only calls the peripheral through its public `zbook_<periph>_*` API (same
rule as `interface/` itself), so it doubles as a minimal usage example for whoever writes the
next language binding.
