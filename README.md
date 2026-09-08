# StutterClone

[![Build](https://github.com/frucot/StutterClone/actions/workflows/build.yml/badge.svg)](https://github.com/frucot/StutterClone/actions/workflows/build.yml)
[![License: AGPL v3](https://img.shields.io/badge/License-AGPL_v3-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.01-cyan.svg)](CHANGELOG.md)

**Version 0.01** — MIDI-triggered, tempo-synced stutter / glitch effect for VST3, AU, and standalone.

[Français](README.fr.md) · [Developer guide](docs/DEVELOPER.md) · [Guide développeur](docs/DEVELOPER.fr.md)

## What it is

StutterClone is an open-source audio effect built with [JUCE](https://juce.com) 8. Hold a MIDI note from **C3 to B3** and the plugin captures the live audio into a loop whose length follows the host tempo. Each of the twelve notes stores its own **action**: step-sequenced curves (4 / 8 / 16 / 32 steps over one bar) that drive stutter division, reverse, pan, filter, lo-fi, delay, and reverb.

The twelve actions together make a **preset**, which you can save and recall from the plugin.

This project is independent and is not affiliated with any commercial stutter product.

## Features

- Tempo-synced beat repeat with straight, triplet, and sextuplet divisions
- Per-note action editor (C3–B3) with drawable parameter curves
- Quantize start: `None`, `1/4`, `1/8`, `1/16`, `1/32`
- Factory preset **Classic** plus user presets on disk
- Live waveform display of the capture ring
- Formats: **VST3** (macOS, Windows, Linux), **AU** (macOS), **Standalone**

## Requirements

- CMake 3.22 or newer
- A C++20 compiler (Xcode Command Line Tools, Visual Studio 2022, or GCC 12+)
- On Linux: ALSA/JACK headers and the usual JUCE GUI packages (see the [developer guide](docs/DEVELOPER.md))

JUCE 8.0.15 is downloaded automatically by CMake (FetchContent). You do not need a local JUCE install.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Optional CMake flags:

| Flag | Default | Meaning |
| --- | --- | --- |
| `STUTTERCLONE_FORMATS` | `VST3;AU;Standalone` | Plugin formats (AU is ignored on Windows/Linux) |
| `STUTTERCLONE_COPY_PLUGIN_AFTER_BUILD` | `ON` | Copy VST3/AU into the user plugin folders |

After a successful macOS build with copy enabled:

- VST3 → `~/Library/Audio/Plug-Ins/VST3/StutterClone.vst3`
- AU → `~/Library/Audio/Plug-Ins/Components/StutterClone.component`

Rescan plugins (or restart the DAW) before loading a new build.

## Usage

1. Insert **StutterClone** as an **audio effect** on an audio track.
2. Route MIDI into the plugin (AU is registered as a Music Effect so hosts expose a MIDI input).
3. Play or hold notes **C3–B3**. The effect waits for the next quantize grid (or starts immediately if Quantize is `None`), then loops the captured slice for one bar while the note is held.
4. Open **Edit Action** to draw curves for that note. Save the twelve actions as a preset.

User presets are stored in:

- macOS: `~/Library/Application Support/StutterClone/Presets/`
- Windows: `%APPDATA%/StutterClone/Presets/`
- Linux: `~/.config/StutterClone/Presets/`

## Versioning

The public version is **0.01**. CMake and plugin metadata use semantic version `0.0.1` (major.minor.patch). The display string `STUTTERCLONE_VERSION_STRING` is defined in [`CMakeLists.txt`](CMakeLists.txt) and generated into `Version.h`. Change those two places together when you bump a release. See [CHANGELOG.md](CHANGELOG.md).

## Contributing

Bug reports and pull requests are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) and the [developer guide](docs/DEVELOPER.md) (real-time audio constraints apply).

## License

StutterClone is free software under the [GNU Affero General Public License v3.0](LICENSE).

It links against JUCE, which is also available under the AGPLv3 for open-source projects. If you distribute a modified plugin (including a hosted service that lets users run it), you must provide the corresponding source under the AGPL.

## Security

Please do not file public issues for vulnerabilities. See [SECURITY.md](SECURITY.md).
