# melee-unlocked-macOS

**Super Smash Bros. Melee with Slippi Online, native on Apple silicon Macs.**

melee-unlocked-macOS runs [melee-unlocked](https://github.com/Hero88go/melee-unlocked)'s Source Port on the Mac: Melee
rebuilt from the decompilation's source code, with melee-unlocked's modes, options and mods, in melee-unlocked's own
launcher and with its icon. The game is compiled on your Mac when you install it and plays from your own disc image.

<sub>An independent community project. It is not affiliated with or endorsed by the Slippi team, the melee-unlocked
authors or Nintendo.</sub>

## Install

**You'll need:**

- A Mac with Apple silicon (M1 or newer), macOS 14 or later.
- [OrbStack](https://orbstack.dev) or Docker Desktop, running. melee-unlocked's game is compiled from source in a
  container on your Mac; nothing from the game is downloaded.
- Your own copy of Super Smash Bros. Melee (NTSC 1.02) as a disc image (.iso, .gcm, or .rvz).
- A free [Slippi account](https://slippi.gg) to play online.

Open Terminal (press ⌘ Space, type *Terminal*, press Return), paste this line and press Return:

```
curl -fsSL https://raw.githubusercontent.com/rkoh46/melee-unlocked-macOS/melee-unlocked-macos/install.sh | zsh
```

When it asks, drag your disc image into the Terminal window and press Return. The first build takes about fifteen
minutes. Then **melee-unlocked-macOS** is in your Applications folder and opens. To update, paste the same line again.

**Building by hand:**

```bash
git clone --branch melee-unlocked-macos https://github.com/rkoh46/melee-unlocked-macOS.git ~/Dashdance
~/Dashdance/setup.sh /path/to/melee.iso
```

## Playing

The launcher has four pages:

- **Play:** your disc, the Game Build (**Source Port**, melee-unlocked's game with its codes and unlocks, or **Vanilla**,
  the game exactly as the disc plays it) and PLAY.
- **Settings:** internal resolution, full screen, display sync, widescreen, volume, Slippi Online and the online input
  delay (Auto or 1 to 4 frames; the in-game menu goes higher).
- **Replay Viewer:** every game you play is saved as a Slippi replay. Watch plays one back in the engine, with
  melee-unlocked's viewer controls; Watch live follows a game the Slippi Launcher is broadcasting to its spectate folder.
- **Mods:** melee-unlocked's mod catalog, installed from your own disc. A chosen mod loads at the next offline start.

In game, **F1** or **Esc** (or L + R + Start) opens melee-unlocked's settings panel: controls and remapping, graphics,
Training, overlays, replays and the rest. Keyboard, every controller macOS sees and the GameCube adapter for Wii U and
Switch (no driver) work.

**Online:** Slippi Unranked, Direct and Teams from the game's own Online Play page, with your Slippi sign-in (this app's,
or the Slippi Launcher's when you are signed in there). Ranked is refused, as melee-unlocked refuses it.

The full feature list, row by row against melee-unlocked's, is in [docs/FEATURES.md](docs/FEATURES.md).

## Coming from Dashdance or DashDance2

This app replaces the earlier builds of this repository (Dashdance's statically recompiled engine, installed as
DashDance2). The installer removes DashDance2 from Applications. Your settings, Slippi sign-in, replays and memory card
stay where they were (`~/Library/Application Support/Dashdance`), and the launcher reads the earlier settings: the disc,
resolution, display sync, full screen, volume, Slippi Online, input delay, widescreen, controller mappings, the keyboard
layout, Discord presence, anisotropic filtering and the HUD carry over. A few of the earlier engine's settings have no
counterpart and are ignored: sharpening, MetalFX upscaling and the touch controls. The earlier engine's memory card
(`User/GC/CardA`) is not read: melee-unlocked keeps its own card in `melee-unlocked/Card A`. Replays recorded by the
earlier engine still play, but not frame for frame exactly. The iPhone, iPad and Vision Pro builds belonged to the
earlier engine and are gone: melee-unlocked's engine runs on the Mac only.

## Privacy

The app collects no data and has no analytics. Your disc image stays on your Mac. It connects only to Slippi Online (to
sign in and to find and report matches), to GitHub (the launcher's update check: whether this repository has newer
changes), to the mod downloads you start, and, locally, to the Discord app when Discord presence is on.

## Questions

**Can I just download the app?** No, and there will never be a download: the finished app contains the compiled game, so
it is built on your own Mac and plays from your own disc.

**Can I play people who use Slippi Dolphin?** Yes: the engine speaks Slippi 3.7.0's matchmaking and netcode.

**Something isn't working.** Please [open an issue](https://github.com/rkoh46/melee-unlocked-macOS/issues). The launcher's
*Save crash report* writes a report with names, codes and addresses removed; it stays on your Mac until you attach it.

## For developers

How the app is put together, how to build and test it, and the exactness gate are in the
[technical guide](docs/TECHNICAL.md). Contributors and coding agents should start with [CLAUDE.md](CLAUDE.md).

## Credits and legal

Built on [Hero88go/melee-unlocked](https://github.com/Hero88go/melee-unlocked), the [doldecomp/melee](https://github.com/doldecomp/melee)
contributors, the [Slippi](https://slippi.gg) team, the [Dolphin](https://dolphin-emu.org) project, [SDL](https://libsdl.org),
Dashdance by TheAndersMadsen and contributors, and Melee-NX-NetPlay (the same engine on the Nintendo Switch). melee-unlocked's
name, icon and launcher artwork are melee-unlocked's.

**Licence.** This project's own code is GPL-2.0-or-later; with melee-unlocked's engine (GPL-3.0-or-later) the app is
distributed under GPL-3.0. Parts under other licences, and the evidence for each, are in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md), which also explains how AI-assisted development bears on copyright.

Super Smash Bros. Melee is the property of Nintendo and HAL Laboratory. This repository contains no game code or data.
You must supply your own legally obtained disc image. This is a factual summary, not legal advice.
