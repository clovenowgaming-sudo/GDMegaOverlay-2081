# GDMegaOverlay -> GD 2.2081 / Geode 5.11.2 / Android (port notes)

Mod By Vinz :3

Original: https://github.com/maxnut/GDMegaOverlay (maxnut, SpaghettDev). This is a port, not a rewrite.

## What this port does
- mod.json: GD 2.2081 (android + win), Geode 5.11.2, new id `vinz.gd_mega_overlay_port`.
- Android: small round draggable button (src/AndroidButton.cpp). Tap = open/close the overlay UI,
  drag = move it (position saved). Survives scene changes (SceneManager::keepAcrossScenes).
- Overlay UI (GUI/, Widgets, Window, Style, Settings) kept as in the original.
- Windows-only code guarded: windows.h, keybd_event (AutoDeafen), ShellExecute, process priority.
- Blur is a no-op on Android (desktop-only GL).
- ghc::filesystem -> std::filesystem.

## Replaced by hooks (byte patches were x86 + 2.204 addresses, so they cannot work on 2.2081/ARM)
- NoClip: src/Hacks/NoClip.cpp (PlayLayer::destroyPlayer).
- Safe Mode: SafeMode.cpp now blocks GJGameLevel::savePercentage + GameStatsManager::completedLevel.
- res/hacks/*.json patches (Bypass / Creator / Global / Level / Player lists) are NOT applied.

## Disabled for now (original files are in legacy_2204/, stubs keep the build compiling)
- Macrobot / Clickpacks / MacroEditor  (needs PlayerObject + checkpoint layout for 2.2081)
- Recorder (ffmpeg, Windows)           - Discord RPC (desktop only)
- Show Hitboxes, Hitbox Multiplier, Show Trajectory, Layout Mode, End Level Layer Info
- TPS / Physics Bypass (wrote to a hardcoded global; no-op for now)
- Instant Complete (relied on an anticheat byte patch - dropped)

## Things marked PORT-CHECK / PORT-TODO in the source (verify against 2.2081 bindings if the build complains)
- GJGameLevel::savePercentage signature, GameStatsManager::completedLevel / uncompleteLevel
- HardStreak::m_pulseSize (trail width)
- CCKeyboardDispatcher::dispatchKeyboardMSG signature (GUI.cpp, desktop only; 2.2081 may have an extra timestamp param)
- GJBaseGameLayer::processCommands(float) hook in Labels.cpp
- Common.cpp section-culling loop (`#if 0`)
- CMake: imgui-cocos is pulled from matcool/gd-imgui-cocos branch `geode`; pin a commit after the first good build.
  If ImGuiCocos::reloadFontTexture / isInitialized / setup().draw() differ in that version, adjust GUI.cpp/main.cpp.

## Not tested
No compiler / Geode SDK was available while writing this. Build with GitHub Actions and send the errors back.
