# Wolfenstein RPG4PSP v0.1.0 Alpha

First public alpha of the reverse-engineered Wolfenstein RPG engine for PSP.

## Highlights

- Native PSP build with XMB thumbnail and background artwork.
- Aspect-correct game viewport and legible PSP-sized menus and subtitles.
- PSP-native controls and rewritten tutorial/help prompts.
- OpenAL sound with a PSP-safe audio channel budget.
- Working configuration, save, load, and Save & Exit behavior.
- Stable clipped castle geometry without the black polygons or warped textures
  seen during early porting.
- Correct transparency and placement for world sprites, foreground weapons,
  hands, feet, and alternating left/right punch animations.
- Validated in PPSSPP 1.20.4 and exercised on real PSP hardware through the
  intro and opening castle gameplay.

## Installation

1. Extract `WolfensteinRPG4PSP-v0.1.0-alpha-PSP.zip`.
2. Copy the included `PSP` directory to the root of your Memory Stick.
3. Add your legally obtained iOS archive beside `EBOOT.PBP`, named either
   `WolfensteinRPG.ipa` or `Wolfenstein RPG.ipa`.
4. Start **Wolfenstein RPG4PSP** from the PSP Game menu.

The proprietary game archive is required but is not included in this release.

## Controls

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

## Credits and license

Based on [WolfensteinRPG-RE](https://github.com/Erick194/WolfensteinRPG-RE)
by Erick Vásquez García and the GEC team. Engine source is GPL-3.0. Wolfenstein
RPG and its original assets belong to their respective rightsholders.

## Verification

`WolfensteinRPG4PSP-v0.1.0-alpha-PSP.zip` SHA-256:
`4411fd19c4b64d2a2148dafae0bbf588481bbe44c76c41cd1a547835c8efc871`
