# Changelog

All notable changes to this project are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the public display version now matches the CMake semver (`0.0.2`).

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
