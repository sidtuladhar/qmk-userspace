# Sofle Pro R2G keymap

My Vial keymap for the Mechboards Sofle Pro R2G, kept as a
[QMK External Userspace](https://docs.qmk.fm/newbs_external_userspace) so it
lives apart from the firmware tree.

Keymap: [`keyboards/mechboards/sofle/r2g/keymaps/mine`](keyboards/mechboards/sofle/r2g/keymaps/mine)

- Pikachu pet animations on the display
- Right-screen settings menu, with a Debug page for per-stage timing
- KeyGlow: music-reactive RGB, driven by a companion macOS app
- Autocorrect, encoder map, VialRGB

## Build

Firmware builds against Mechboards' Vial fork, branch `r2g`. Upstream QMK does
not have this keyboard.

### GitHub Actions

Every push builds the firmware. The `.uf2` is attached to the `latest` release
and to the workflow run's artifacts.

### Locally

```sh
git clone -b r2g --recurse-submodules https://github.com/MechboardsLTD/vial-qmk.git ~/vial-qmk
git clone https://github.com/<you>/qmk_userspace.git ~/qmk_userspace

qmk config user.qmk_home="$HOME/vial-qmk"
qmk config user.overlay_dir="$HOME/qmk_userspace"

qmk userspace-compile          # or: qmk compile -kb mechboards/sofle/r2g -km mine
```

The `.uf2` is written to both this folder and `~/vial-qmk`.

## Flashing

Both halves need the same firmware. For each half:

1. Unplug the cable between the halves.
2. Hold that half's outer top corner key, then plug USB into **that same half**.
3. `RPI-RP2` mounts; copy `mechboards_sofle_r2g_mine.uf2` onto it.

Flashing resets the Vial keymap (EEPROM), so reload your layout backup in
Vial afterwards.
