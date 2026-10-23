# Wolfenstein RPG4PSP

This is a native PSP port of the reverse-engineered WolfensteinRPG-RE engine.
It uses SDL2, OpenGL, OpenAL, and zlib, and preserves
the original 480x320 image at the correct aspect ratio in a 408x272 viewport.
The EBOOT includes native-resolution XMB thumbnail and background artwork.
The imported upstream baseline is commit
`99720e39770116366442b03608daaf1d0b744160`.

## Data

The proprietary game data is not included. Place a legally obtained iOS game
archive beside `EBOOT.PBP`. Both original filename variants are accepted:
`WolfensteinRPG.ipa` and `Wolfenstein RPG.ipa`.

```text
PSP/GAME/WolfensteinRPG/
|-- EBOOT.PBP
`-- WolfensteinRPG.ipa
```

## Default controls

| PSP input | Action |
| --- | --- |
| D-pad up/down | Move forward/backward; navigate menus |
| D-pad left/right | Turn left/right; change menu page |
| Analog stick | Move/strafe |
| Cross | Attack, talk, use, or select |
| Square | Pass turn |
| Triangle | Automap |
| Circle or Start | Open menu or go back |
| L/R | Previous/next weapon |
| Select | Items/info |

## Build

Install a current PSPDEV toolchain with SDL2, OpenAL, zlib, and GL, set
`PSPDEV`, then run:

```sh
./build-psp.sh
```

The output is `build-psp/src/EBOOT.PBP`.
A copy of the latest compiled EBOOT may also be kept at the project root for
local testing; packaged binaries are published through GitHub Releases rather
than committed to the source tree.

The engine source remains under the upstream GPL-3.0 license. Wolfenstein RPG
and its original assets are owned by their respective rightsholders.

## Current status

- The PSP target compiles and packages successfully with PSPDEV.
- The EBOOT boots with the user-supplied IPA in PPSSPP 1.20.4.
- Configuration creation and subsequent loading work beside `EBOOT.PBP`.
- PSP cinematics use the normal gameplay viewport with compact subtitle panels;
  the original iPhone cinematic masks and touch-control overlay are disabled.
- First-level tutorials and Help screens identify the PSP buttons instead of
  referring to tapping, swiping, phone switches, or accelerometer controls.
- Sprite palette index zero is forced transparent, preventing opaque black
  shadow rectangles after loading a saved map.
- Hardware-rendered castle geometry now passes through the engine's back-face
  test, preventing ceiling/sky polygons from flashing across the view.
- Polygons crossing the PSP camera or screen boundary are clipped by TinyGL
  before GPU submission. The clipper uses stable input/output vertex lists and
  overflow-safe intersections, avoiding the corrupted edge wedges produced by
  the original overlapping in-place vertex copies.
- Frustum intersections are calculated in clip space while paired world-space
  vertices and texture coordinates are interpolated at the same edge fraction.
  The resulting world polygon returns through PSPGL's native XYZ transform path,
  preserving perspective-correct textures without unsupported clip-space `w`.
- World-space billboards use that same clipped path, and an enabled scissor
  boundary prevents foreground sprites from spilling into the aspect-ratio bars.
- Save & Exit safely rebuilds the title screen without dereferencing released
  title images or reloading gameplay images that are immediately discarded.
- Buffered save writes now remain contiguous when a record crosses an internal
  stream boundary.
- The PSP lifecycle, input, timing, audio-channel budget, filesystem behavior,
  cleanup, and aspect-correct viewport have been aligned with the known-good
  Doom RPG PSP port where the two engines share platform behavior.
- The initial release has been exercised in PPSSPP and on PSP hardware through
  the intro and opening castle areas, including combat, save/reload, pause-menu
  exit flow, and foreground weapon animations. Broader end-to-end playthrough
  testing is welcome.

See [PSP_PARITY.md](PSP_PARITY.md) for the detailed Doom RPG comparison and
the intentional engine differences.
