# StutterClone user manual

Version **0.0.7**. Guide en français : [USER_MANUAL.fr.md](USER_MANUAL.fr.md).

StutterClone is an **audio effect** triggered by **MIDI**. It is not a synthesizer: it processes the sound that already lives on the track, and a MIDI note from **C3 to B3** decides *when* to stutter and *how*.

This project is independent and is not affiliated with any commercial stutter product.

## What it does

Incoming audio is always recorded into a short ring buffer (about four seconds). When you hold a gesture note, the plugin:

1. Waits for the next **Quantize** grid (or starts immediately if Quantize is `None`).
2. Captures a slice whose length follows the **Division** of that note (tempo-synced).
3. Replaces the live sound with that looping slice for as long as the note is held.
4. Reads the note's **action** over one bar in 4/4 (four beats). The action is a set of step curves: stutter division, reverse, pan, fuzz, filter, lo-fi, delay, and reverb.
5. Crossfades back to the live signal when you release the note (about 3 ms).

Notes outside C3–B3 are ignored. If several gesture notes are held at once, **the highest note wins**.

The on-screen keyboard **selects which action you edit** (cyan) and **plays that slot while you hold the click** (orange). MIDI from a controller, clip, or the standalone input also plays the same slots.

## Install

| Format | Platforms | Typical location |
| --- | --- | --- |
| VST3 | macOS, Windows, Linux | macOS: `~/Library/Audio/Plug-Ins/VST3/` |
| AU | macOS | `~/Library/Audio/Plug-Ins/Components/` |
| Standalone | all | next to the build, or the app bundle on macOS |

After copying a new build, rescan plugins or restart the DAW. On macOS, AU is registered as a **Music Effect** (`aumf`) so hosts that support it expose a MIDI input on the audio FX.

## Signal flow

Place StutterClone as an **insert** on an audio track (or a group / return).

- Effects **before** StutterClone are captured into the stutter loop.
- Effects **after** StutterClone process the stuttered (or dry) output.
- With no gesture active, audio passes through dry. The ring buffer still records, so the next capture has recent material.

You always need two things at the insert: **audio in** and **MIDI in** (C3–B3).

## Routing

How you feed MIDI into an audio effect depends on the host. The Ableton Live example below is the pattern to copy elsewhere: one audio track for the sound, one MIDI track aimed at the plugin.

### Ableton Live (example)

Use **two tracks**. Live will not play a MIDI clip on an audio track, so the MIDI must come from a MIDI track whose output is the plugin.

**1. Audio track — the sound**

1. Create an audio track (for example `Vocal`).
2. Load a clip, or set **Audio From** to your interface input.
3. In Device View, insert **StutterClone** (VST3 or AU).
4. Put EQ / compression you want *inside* the loop **before** StutterClone; put send-style FX you want on the result **after** it.
5. Leave the track's **Monitor** as you usually would (`Auto` for clips, `In` for live input).

**2. MIDI track — the gestures**

1. Create a MIDI track (for example `Stutter MIDI`).
2. **MIDI From**: your controller, or `All Ins`. Arm the track (or set Monitor to `In`) so a keyboard reaches Live.
3. **MIDI To**: the audio track (`Vocal`), then in the second dropdown choose **StutterClone** — not `Track In`.
4. Draw or record notes **C3–B3** in a clip, or play them live. Other pitches do nothing.

**3. Check it**

1. Start Live's transport (the plugin follows host tempo / PPQ).
2. Play audio on `Vocal`.
3. Hold C3. The header should go **Pending** (waiting for Quantize) then **Active**. The waveform shows the captured loop.

If the header stays **Inactive**, MIDI is not reaching the plugin: confirm **MIDI To** is StutterClone, the MIDI track is armed or playing a clip, and the notes are in C3–B3 (Ableton's C3 is MIDI note 60).

**Typical layout**

```
[Audio: Vocal]   clip or input  -->  (optional FX)  -->  StutterClone  -->  (optional FX)  -->  master
[MIDI: Gestures] controller/clip -->  MIDI To: Vocal / StutterClone
```

You can use a MIDI clip on `Stutter MIDI` to sequence stutters in the arrangement, or play them live from a two-octave controller mapped around C3.

### Other hosts (same idea)

- **Logic Pro**: insert StutterClone as an audio FX; AU Music Effect should show a MIDI input. Route a software instrument or external MIDI to that input.
- **Standalone**: grant microphone access if asked, select the audio input in the device settings, and play C3–B3 from a MIDI keyboard (or the computer keyboard if your JUCE MIDI settings allow it).

## Interface

The plugin opens **expanded**. **Editor** (top right) collapses the curve panel to a compact performance view (header, presets, waveform, keyboard) and expands it again at full height so every lane is visible.

### Header

| Control | Meaning |
| --- | --- |
| Title / version | Plugin name and display version |
| ? | Short overlay: routing, gestures, and how actions work |
| Gesture line | Playing or pending note, current division, and step (for example `C3 \| 1/16 \| step 3`). Idle: `C3-B3 \| hold a note` (range follows Octave - / +) |
| BPM badge | Tempo reported by the host |
| MIDI badge | `Inactive` (red) / `Pending` (yellow, waiting for Quantize) / `Active` (cyan) |

### Updates

When the editor opens, StutterClone checks GitHub for a newer release. If one exists, a banner appears at the top with a **Releases** button that opens the [GitHub releases page](https://github.com/frucot/StutterClone/releases) in your browser. This is a notification only: the plugin does not download or install updates. Dismissing the banner hides it until a later version is published.

### Presets and Quantize

A **preset** is the Quantize setting plus the twelve actions (one per note).

| Control | Meaning |
| --- | --- |
| Preset menu | Load **Classic** (factory) or a user preset |
| Save | Overwrite the current **user** preset. On Classic, this opens Save As |
| Save As | Store a new user preset (name overlay) |
| Delete | Remove a user preset. Classic cannot be deleted |
| Quantize | When a gesture may start: `1/4`, `1/8`, `1/16`, `1/32`, or `None` (immediate) |

User presets on disk:

- macOS: `~/Library/Application Support/StutterClone/Presets/`
- Windows: `%APPDATA%/StutterClone/Presets/`
- Linux: `~/.config/StutterClone/Presets/`

The DAW session also stores the working preset with the plugin state, so a project reloads your last edits even if you never clicked Save.

### Waveform and keyboard

The waveform is the capture ring: live input while idle, the looping slice while a gesture is active (write head in yellow).

The twelve-key keyboard (default C3–B3 = MIDI 60–71). **Octave - / +** move the whole window by 12 MIDI notes:

- Click a key to **edit** that slot's action (cyan) and expand the editor if it was collapsed.
- **Hold the click** to play the slot; an orange overlay shows the note that is sounding.
- Keys also light orange when the matching MIDI note is held.
- Octave - maps slot 1 to MIDI 48 (C2); Octave + maps it to MIDI 72 (C4). The twelve actions stay in the same slots.

### Factory Classic (default divisions)

Until you draw curves, **Classic** only changes **Division** per note. Other modules stay off.

| Note | Default division |
| --- | --- |
| C3, C#3 | 1/8 |
| D3, D#3 | 1/16 |
| E3 | 1/32 |
| F3, F#3 | 1/64 |
| G3 | 1/4 |
| G#3 | 1/8T |
| A3 | 1/16T |
| A#3 | 1/32T |
| B3 | 1/1 |

T = triplet. Older presets that used sextuplets (`S`) load as the matching triplet.

## Action editor

Each note has its own action. Click the keyboard (or play a note, then click the key) to edit it. A red playhead runs across the lanes while that note is the active gesture.

Curves cover **one bar** (four beats in 4/4). In the default mode they wrap for as long as you hold the note. With **Ping-Pong** they play forward, then backward (eight beats), then repeat. Horizontal position is time; the number of columns is **Grid**.

### Toolbar

| Control | Meaning |
| --- | --- |
| Grid | `4`, `8`, `16`, or `32` steps across the bar. Changing grid resamples the existing curves |
| Ping-Pong | Off (default): wrap to the start of the bar. On: read the bar forward then backward (8 beats) |
| Unfreeze Loop | Off (default): keep the slice captured at gesture start. On: recapture from the ring every **Loop** period |
| Loop | Recapture period when Unfreeze is on: `1 Beat`, `2 Beats`, `3 Beats`, `1 Bar`, `2 Bars` |
| Delay Cut | On note release, clear the delay so tails do not continue |
| Reverb Cut | On note release, clear the reverb so tails do not continue |

Without the Cut options, delay and reverb can ring out during the short fade back to dry.

### Drawing lanes

- **Continuous lanes** (Division, Cutoff, Mix, …): click and drag. Height is the value (up = more / shorter division).
- **On/off lanes** (green = On, grey = Off): click a step to toggle; drag paints that value across steps.
- After a drag, the last step shows a numeric readout (Hz, %, On/Off, division name, …).

### Stutter

| Lane | Role |
| --- | --- |
| Division | Loop length in beats: `1/1`, `1/2`, `1/4` … `1/64`, plus triplets (`T`). The lane is ordered by duration: longest at the bottom, shortest at the top |
| Reverse | Play the slice backwards |
| Alt Pan | Alternate the stutter between left and right (the switch is smoothed to avoid clicks) |

### Fuzz

Runs on the stuttered slice, before Filter. **0% = dry** (no colour, no level change).

| Lane | Role |
| --- | --- |
| Gain | Adaptive two-band saturation: round wavefold below ~250 Hz, germanium-style grit above. Smoothed so 16th-note steps do not click |

### Filter

The **Filter** menu next to the group title is the type for this note: **Lowpass**, **Highpass**, or **Bandpass**.

| Lane | Role |
| --- | --- |
| Filter On | Enable the filter on that step |
| Cutoff | About 20 Hz – 20 kHz |
| Resonance | About 0.10 – 4.00 |

### Lo-Fi

| Lane | Role |
| --- | --- |
| Lo-Fi On | Enable bit reduction and downsampling |
| Bits | About 1–16 bits |
| Downsample | Integer factor 1–16 (1 = no extra downsampling) |

### Delay

The **Delay** menu next to the group title is the synced delay time: **1/4**, **1/8**, **1/16**, **1/32**.

| Lane | Role |
| --- | --- |
| Delay On | Enable delay |
| Delay Mix | Dry/wet |
| Feedback | Repeats (capped below 1 so it cannot run away) |

### Reverb

| Lane | Role |
| --- | --- |
| Reverb On | Enable reverb |
| Reverb Mix | Dry/wet |
| Size | Room size |
| Damping | High-frequency damping |

The FX chain is **fuzz → filter → lo-fi → delay → reverb**, and it runs only while a gesture (or its fade-out) is active.

## Performance tips

- Play or hold notes in time with Quantize so attacks land on the grid.
- Keep the host transport running if you rely on Quantize; tempo and PPQ come from the DAW.
- Use Unfreeze when the source should keep evolving inside the stutter (a pad, a vocal phrase) instead of a frozen slice.
- Leave Unfreeze off for a classic “catch and repeat” glitch.
- Several overlapping notes always resolve to the highest pitch; release it to fall back to the next held note, or to dry if none remain.

## Troubleshooting

| Symptom | What to check |
| --- | --- |
| No stutter, header stays Inactive | MIDI is not reaching the plugin, or notes are outside the current Octave window |
| Pending forever | Transport may be stopped, or Quantize never reaches the next grid. Try `None` |
| Stutter is dry / no FX | Filter On, Lo-Fi On, Delay On, Reverb On are off on those steps |
| Delay or reverb hangs after release | Enable Delay Cut / Reverb Cut on that action |
| Host window is tiny / lanes scrolled | Expand **Editor**; the window should grow to show every lane. Reload the plugin if the DAW cached an old size |
| AU has no MIDI pin | Use the AU build (Music Effect). Some hosts only expose MIDI on VST3 |

## License

StutterClone is free software under the [GNU Affero General Public License v3.0](../LICENSE).
