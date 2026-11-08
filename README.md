# GD Mega Overlay (2.2081 port)

> **Mod By Vinz :3**

A Mega Hack style overlay for **Geometry Dash 2.2081** (Android), built with **Geode SDK 5.11.2**.

This project is a port of the open source [GDMegaOverlay](https://github.com/maxnut/GDMegaOverlay)
by **maxnut** and **SpaghettDev**. The original targeted GD 2.204 on Windows and used hardcoded
x86 offsets; this port replaces those with Geode hooks and adds Android support.

## Features

| Area | What works |
|------|-----------|
| UI | Original overlay windows, draggable round button, touch scrolling |
| Macrobot | Record / Play / Save / Load, checkpoint aware frame counter |
| Recorder | Video recording through the FFmpeg API mod, selectable codec / FPS / bitrate / scale |
| Player | NoClip, No Death Effect, Solid Wave Trail, No Ghost Trail, No Spawn Effect, No Spider Dash, No Burst Effect, No Portal Lightning |
| Level | Show Hitboxes, Safe Mode, Instant Complete, Anticheat Bypass, Layout Mode, No Mirror Portal, No Camera Shake, No Camera Zoom, No Ending Animation |
| Bypass | Icon, Main Levels, Timeout, Slider Limit, Vault, Treasure Room, Text Length, Character Filter |
| Creator | Copy Hack, Object Limit Bypass |

### Not ported yet

Hitbox Multiplier, Show Trajectory, TPS / Physics Bypass, Discord RPC, Keymaster / Guard / Gatekeeper vaults,
Gauntlet and Level Edit bypasses, recorder audio. Contributions are welcome.

## Safe Mode

Cheats turn Safe Mode on automatically. While it is active nothing is saved or submitted.
Instant Complete and Anticheat Bypass force it on.

## Usage (Android)

1. Install Geode and this mod (and the FFmpeg API mod for the recorder).
2. Tap the round **M** button to open the overlay. Drag the button to move it.
3. Drag vertically inside a window to scroll.

## Building

The mod is built with GitHub Actions (`.github/workflows/build.yml`) using
[`geode-sdk/build-geode-mod`](https://github.com/geode-sdk/build-geode-mod). Push to the repository,
then download the `Build Output` artifact.

Local build: install the Geode CLI and SDK, then `geode build`.

## Credits

- **maxnut** and **SpaghettDev**: original GDMegaOverlay
- **Dear ImGui**, and **matcool**'s gd-imgui-cocos
- **Eclipse** / **maxnut**: FFmpeg API mod
- **Geode** team: SDK and bindings
- **Vinz**: 2.2081 / Android port

The original project's license terms apply to the code derived from it. See the upstream repository.
