# Changelog

All notable changes to this project are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the public display version now matches the CMake semver (`0.0.3`).

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
