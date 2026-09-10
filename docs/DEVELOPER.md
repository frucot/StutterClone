# Developer guide

This document is for people who want to build, debug, or extend StutterClone. For playing the plugin, see the [user manual](USER_MANUAL.md). For a short install overview, see [README.md](../README.md). Guide en français : [DEVELOPER.fr.md](DEVELOPER.fr.md).

## Repository layout

```
CMakeLists.txt          Build, version, JUCE plugin metadata
Source/
  PluginProcessor.*     Audio callback, MIDI, quantize gate, preset RT snapshots
  PluginEditor.*        Main UI
  Action.h              POD actions, divisions, curve evaluation
  PresetBank.*          XML load/save (factory Classic + user files)
  DspChain.*            Filter → lo-fi → delay → reverb
  ActionEditor.*        Per-note curve window
  CurveLane.*           Step lane widget
  NoteKeyboard.*        C3–B3 keyboard (display octave is user-settable)
  WaveformDisplay.*     Ring-buffer view
  Version.h.in          Generated version header
.github/workflows/      CI (Windows + Linux VST3)
```

## Version numbers

There is one source of truth in `CMakeLists.txt`:

1. `project(StutterClone VERSION 0.0.5 …)` — semantic version used by CMake and JUCE (`JucePlugin_Version`).
2. `STUTTERCLONE_VERSION_STRING` (`"0.0.5"`) — string shown in the editor and in human-facing docs.

CMake generates `build/generated/Version.h` from `Source/Version.h.in`. Include `"Version.h"` anywhere you need the display string.

When cutting a release:

1. Bump both values together (for example `0.0.6` / `"0.0.6"`).
2. Add an entry to [CHANGELOG.md](../CHANGELOG.md).
3. Commit, tag (`git tag v0.0.6`), and push the tag.

## Real-time rules

The audio thread (`processBlock`) must not:

- allocate (`new`, `malloc`, `std::vector::resize`, capturing `std::function`)
- lock (`std::mutex`, `juce::CriticalSection`)
- do I/O (`DBG`, `std::cout`, disk)

Allocate DSP objects and buffers in `prepareToPlay`. UI edits copy the twelve `Action` structs into a double buffer, then publish with `std::atomic` index flip. Loop wraps use a short crossfade (about 3 ms) to avoid clicks.

## Audio / MIDI architecture

1. Incoming audio is always written into a 12-second ring buffer.
2. Note On on the current 12-note window (default MIDI 60–71 / C3–B3; Octave - / + shifts by 12) arms a pending gesture (or starts immediately if Quantize is `None`).
3. At the next PPQ grid tick, the plugin captures the last N samples, starts stutter playback, and reads the note's action curves over 4 beats (one bar in 4/4). With Ping-Pong the bar then plays in reverse (8-beat cycle). Loops while the note is held.
4. `GestureDspChain` runs only while the gesture (or its fade-out) is active. Stages: `processFilter` → `processLoFi` → `processDelay` → `processReverb`.

Notes outside the current octave window are ignored. If several gesture notes are held, the highest note wins.

Action curves are **not** APVTS parameters (that would explode the host parameter count). They live in a `ValueTree` saved beside APVTS in `getStateInformation`. The slot window (`firstGestureNote`) is stored on the session root, not in presets. Preset XML still identifies slots by canonical MIDI 60–71.

## Adding a curved parameter

1. Add a value to `stutter::Curve` in `Action.h` (keep the enum contiguous) and extend `numCurves`.
2. Give it XML id in `PresetBank.cpp`, a default in `initActionDefaults`, and evaluation in `evaluateAction` / `EvaluatedStep`.
3. Add or extend a row in `stutter::laneGroups` (title, start index, count). The editor builds one `CurveLane` per `Curve`.
4. Add matching fields to `GestureDspChain::Settings` and a named `process*` stage in `DspChain.cpp` (insert it in the existing filter → lo-fi → delay → reverb order).
5. Consume the evaluated value in `PluginProcessor::processFxSlice` or the stutter loop.

Keep new data POD and fixed-size so copies stay lock-free.

## Local build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSTUTTERCLONE_COPY_PLUGIN_AFTER_BUILD=OFF
cmake --build build --target StutterClone_Standalone --parallel
```

Useful targets: `StutterClone_Standalone`, `StutterClone_VST3`, `StutterClone_AU` (macOS).

Linux packages used by CI are listed in `.github/workflows/build.yml`.

## Tests and CI

There is no unit-test suite yet. GitHub Actions builds Release VST3 on Windows and Ubuntu. macOS/AU is not built in CI; test AU locally before a release.

## Code style

- C++20, JUCE 8 APIs only (no raw Win32 / Cocoa / CoreAudio).
- Prefer `noexcept` on audio-thread helpers.
- ASCII in UI string literals (JUCE `const char*` constructors are not reliable with UTF-8 on all hosts).
