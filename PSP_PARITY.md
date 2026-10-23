# Doom RPG to Wolfenstein RPG PSP parity audit

This audit uses the hardware-verified Doom RPG PSP v0.1.6 implementation as
the platform baseline. It does not assume that the two frontends are identical:
Doom RPG renders through SDL2 and mixes with SDL_mixer, while Wolfenstein RPG
uses OpenGL/PSPGL and OpenAL. The goal is parity in PSP platform behavior, not
replacement of Wolfenstein's renderer or sound engine.

## Reused platform behavior

| Area | Doom RPG baseline | Wolfenstein RPG result |
| --- | --- | --- |
| Startup | Initialize the platform before loading game data | SDL/video/GL now initialize before the IPA is opened |
| Working directory | Keep data and writable files beside `EBOOT.PBP` | Startup anchors the working directory to the executable directory |
| Game archive | Proprietary archive beside `EBOOT.PBP` | Accepts `WolfensteinRPG.ipa` and `Wolfenstein RPG.ipa` beside `EBOOT.PBP` |
| Save/config files | Direct PSP filesystem access; no shell | Uses native directory/file operations and saves beside `EBOOT.PBP` |
| Controller | Direct `sceCtrl` polling | Direct `sceCtrl` polling; SDL joystick delivery is disabled on PSP to avoid duplicate input |
| PSP clocks | 333 MHz CPU / 166 MHz bus | Preserved at the Doom RPG known-good values |
| Effect channels | Six | OpenAL source pool is limited to six on PSP |
| Frame timing | Platform timer initialized after video | Timer ordering is aligned with the known-good startup path |
| Shutdown | Main-thread, orderly resource teardown | Idempotent centralized shutdown; stack-owned objects are no longer manually destroyed |
| Errors | Fail cleanly after reporting the real cause | SDL/GL, archive, and construction failures stop startup cleanly |

## Wolfenstein-specific handling

- Rendering remains OpenGL/PSPGL because Wolfenstein's 3D frontend depends on
  it. The 480x320 game image is consistently letterboxed into a 408x272 PSP
  viewport instead of being stretched to 480x272. Coordinate and viewport
  transforms now share the same calculation.
- Audio remains OpenAL. Unlike Doom RPG's 92 effects (about 2.95 MB decoded),
  the Wolfenstein IPA contains 162 WAV files totaling about 29.28 MB. Loading
  every decoded effect at startup would consume too much PSP memory, so the
  port retains a bounded dynamic cache while matching Doom RPG's six-channel
  PSP playback budget.
- The IPA is read directly; it is not renamed or converted to Doom RPG's data
  ZIP format.

## Reliability corrections found during the audit

- Fixed the script end opcode, whose signed `-1` marker was previously read as
  unsigned `255` and could never match.
- Corrected array cleanup throughout map, media, and localization transitions
  to prevent heap corruption during longer sessions.
- Removed manual destructor calls and duplicate ownership of video/archive
  objects.
- Made resource-path construction bounded and removed a wasteful allocation
  when updating save files.
- Restored the correct viewport after every GL reset and 3D render path.
- Made compressed-sprite palette index zero transparent on GL, including
  palettes without the original magenta color key.
- Restored the TinyGL back-face test for hardware-rendered polygons and widened
  its facing calculation to avoid camera-dependent integer overflow.
- Routed only frustum-crossing PSP polygons through TinyGL's clipper, replaced
  its overlapping in-place vertex shifts with stable input/output lists, and
  made clip-edge interpolation overflow-safe.
- Kept clip-space and world-space vertices paired while clipping, then submitted
  the clipped world polygon through PSPGL's native XYZ transform path. This
  avoids both camera-dependent affine texture warping and the unsupported use
  of application-supplied homogeneous `w` on the PSP Graphics Engine.
- Routed the separate world-sprite fast path through the same clipping logic and
  enabled the configured scissor rectangle so close foreground sprites cannot
  render into the PSP pillarbox area.
- Matched Doom RPG's foreground lifecycle for hands, weapons, and attack
  effects: they now render after the 3D scene in logical screen space, use
  alpha-key rejection instead of the world depth/clip path, and retain aligned
  per-draw vertex data until PSPGL finishes the frame.
- Fixed buffered save records that cross the stream boundary and made the
  save-and-return-to-title image lifecycle safe on PSP.

## Verification performed

- PSPDEV release build: successful.
- Native debug build: successful.
- Native undefined-behavior-sanitizer build: successful.
- PPSSPP 1.20.4 boot with the supplied IPA: successful.
- Configuration file creation beside `EBOOT.PBP`: successful.
- A subsequent boot with the existing configuration: successful, with no
  `openZipFile`, SDL library, or `SaveConfig` application error.

## Still required on real hardware

Before designating a hardware-known-good release, run a real PSP through:

1. New-game startup and several map transitions.
2. Combat with overlapping weapon and enemy audio.
3. Save, full exit, relaunch, and load.
4. Analog and D-pad input, menu navigation, and exit-to-XMB.

XMB icon/background artwork is also intentionally absent until suitable
Wolfenstein RPG artwork is supplied.
