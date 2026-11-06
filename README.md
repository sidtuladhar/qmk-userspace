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

The build is pinned to one commit of that fork: `VIAL_QMK_REF` in
[`.github/workflows/build_binaries.yaml`](.github/workflows/build_binaries.yaml).
The build container is pinned by digest and each action by commit. To pick up
new Mechboards changes, set `VIAL_QMK_REF` to the new head of `r2g` and push.

### GitHub Actions

Every push builds the firmware, on any branch. Only a successful build on
`main` is published, as two releases:

- `build-N`: kept permanently, so older firmware stays downloadable
- `latest`: always the newest build

The `.uf2` is also attached to each workflow run's artifacts.

### Locally

```sh
git clone https://github.com/sidtuladhar/qmk-userspace.git ~/qmk_userspace
git clone -b r2g https://github.com/MechboardsLTD/vial-qmk.git ~/vial-qmk

# Match CI: check out the pinned vial-qmk commit
git -C ~/vial-qmk checkout "$(sed -n 's/^ *VIAL_QMK_REF: *\([0-9a-f]*\).*/\1/p' ~/qmk_userspace/.github/workflows/build_binaries.yaml)"
git -C ~/vial-qmk submodule update --init --recursive

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

Flashing resets the Vial keymap (EEPROM). Afterwards, load
[`latest.vil`](keyboards/mechboards/sofle/r2g/keymaps/mine/latest.vil) in Vial
(File → Load saved layout).

After changing the layout in Vial, save it over that file (File → Save current
layout) and commit it, so the backup stays current.
