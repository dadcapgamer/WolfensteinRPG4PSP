# Wolfenstein RPG4PSP

An unofficial native PlayStation Portable port of Wolfenstein RPG, based on
[WolfensteinRPG-RE](https://github.com/Erick194/WolfensteinRPG-RE) by Erick
Vásquez García and the GEC team.

The copyrighted Wolfenstein RPG game data is not included. You must provide a
legally obtained copy of the original iOS game archive.

> Installation, controls, build instructions, and current status:
> [README_PSP.md](README_PSP.md)

## Quick installation

1. Download the latest release.
2. Copy the `WolfensteinRPG` folder to `PSP/GAME/` on your Memory Stick.
3. Copy your legally obtained `WolfensteinRPG.ipa` into that folder beside
   `EBOOT.PBP`.
4. Launch **Wolfenstein RPG4PSP** from the PSP Game menu.

The final layout should be:

```text
PSP/GAME/WolfensteinRPG/
|-- EBOOT.PBP
`-- WolfensteinRPG.ipa
```

The spaced filename `Wolfenstein RPG.ipa` is also accepted.

## PSP features

- Native PSP homebrew executable with XMB icon and background artwork.
- Aspect-correct 480x320 game presentation within the PSP's 480x272 display.
- PSP-native controls and rewritten in-game button prompts.
- Functional sound, configuration, save, load, and return-to-menu flows.
- PSP-specific renderer fixes for clipped geometry, texture stability,
  transparency, foreground weapons, and melee animations.
- Shared platform behavior aligned with the proven Doom RPG4PSP port where the
  two engines overlap.

## Upstream project

Wolfenstein RPG Reverse Engineering by [GEC], created by Erick Vásquez García.
The PSP port is based on upstream commit
`99720e39770116366442b03608daaf1d0b744160`.

- Upstream source: <https://github.com/Erick194/WolfensteinRPG-RE>
- Project discussion: <https://www.doomworld.com/forum/topic/151475>

## License

The engine source is distributed under the GNU General Public License v3.0;
see [LICENSE](LICENSE). Wolfenstein RPG and its original assets are owned by
their respective rightsholders and are not distributed by this project.
