# Changelog

All notable changes to this project are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the public display version now matches the CMake semver (`0.0.5`).

## [0.0.5] - 2026-09-10

### Added

- Octave - / + move the twelve gesture slots by one octave (default MIDI 60–71)
- Click-and-hold on the plugin keyboard plays the selected slot; cyan = edit selection, orange = sounding note
- Per-action Ping-Pong: the bar plays forward then backward (8 beats) instead of wrapping
- Help overlay (`?`) with a short routing and gesture summary
- Stutter divisions `1/1`, `1/2`, and their triplets

### Changed

- Sextuplet stutter divisions (`1/8S`, `1/16S`, `1/32S`) removed; older presets remap S to the matching T
- Factory Classic B maps to `1/1`
- Capture ring is 12 seconds so a whole-bar slice still fits at slow tempos
- Effect groups and DSP stages are table-driven so extra modules can be added without rewriting the editor layout

## [0.0.5-beta] - 2026-09-10

### Fixed

- Ableton Live 9 (AU) and Live 10.1 (VST3) rejecting or crashing the editor on load:
  heavy UI was constructed before the processor was ready, then `setSize` ran again
  while the host attached IPlugView
- Older VST3 hosts querying disabled/empty bus layouts, which could blacklist the plugin

## [0.0.4] - 2026-09-09

### Changed

- Single collapsible plugin window: the separate Action Editor window is gone
- Compact header (BPM badge, MIDI status and gesture on one line)
- Effect modules laid out in two columns, with Filter and Delay menus next to their group titles
- Gate lanes (Reverse, Alt Pan, Filter/Lo-Fi/Delay/Reverb On) drawn as compact on/off toggles
- Grid, Unfreeze Loop, Loop, Delay Cut and Reverb Cut on one toolbar row

### Fixed

- Reopening the editor restored a too-small host size, which hid lanes behind a scrollbar

## [0.0.3] - 2026-09-09

### Added

- Loop unfreeze per gesture: re-capture the ring loop every 1, 2 or 3 beats, or every 1 or 2 bars,
  with Freeze still the default

### Fixed

- Clicks on every loop repetition: the wrap cross-fade faded the loop head against the loop's own
  tail, which stepped the waveform back by the fade length instead of joining it
- Clicks from the alternating pan, which switched a channel on a single sample boundary
- macOS bundles left with an invalid code signature when the plugin copy step is disabled, which
  made hosts reject the VST3 on a fresh scan

## [0.0.2] - 2026-09-08

### Fixed

- Audio thread stall on non-finite host BPM and on a quantised trigger while transport is stopped
- Torn waveform snapshot copies read from the editor thread

## [0.01] - 2026-09-08

### Added

- MIDI gestures on C3–B3 with per-note action curves (stutter, filter, lo-fi, delay, reverb)
- Preset save/recall (factory Classic + user XML bank)
- Action editor with 4 / 8 / 16 / 32-step lanes
- Quantize start: None, 1/4, 1/8, 1/16, 1/32
- Triplet and sextuplet stutter divisions
- In-plugin Save As overlay
- Public documentation (README, developer guides, contributing, security)

### Changed

- Global FX knobs replaced by per-note actions
- Plugin display version shown in the editor header
