# Quaternion + Serenity

This repo builds two JUCE VST3/Standalone instruments:

- **Quaternion** — a 4-voice instrument, deliberately unstable: it needs a
  held MIDI note, and HAUNT can intrude on it with ghost/memory/swap
  intrusions.
- **Serenity** — its calmer counterpart: a free-running ambient generator.
  No MIDI note required — five pad voices swell and fade on independent
  random clocks over a slowly root-drifting pentatonic/modal chord, while
  a random scheduler fires soft FM "chime" bells. See `original-html/
  serenity.html` for a standalone Web Audio prototype of the same idea, and
  `PureData/serenity.pd` for a vanilla-Pd patch of the same design (no
  external libraries — open it in Pure Data 0.46+ and hit the POWER toggle).

## Quaternion

A 4-voice instrument, ported from two Web Audio prototypes:

- **SHIFT** (`original-html/shift.html`) — a Bode frequency shifter built from
  8 stock `BiquadFilterNode`s realizing a Hilbert transformer, plus a
  phase-locked quadrature LO.
- **CAYLEY II** (`original-html/cayley-ii.html`) — an 8-voice octonion
  harmonic sequencer: each voice is one octonion component, cross-mixed by
  a spinning unit-octonion rotor (`Z = Y·X`), with harmonic-series step
  patterns, wave-morph voices, and "haunted" memory intrusions.

Quaternion keeps SHIFT's DSP core untouched (still bit-for-bit the same
Niemitalo allpass coefficients and quadrature-LO math) and reduces CAYLEY
II's 8 octonion components to exactly **4**, so each voice *is* one
quaternion component — the Hamilton product (`Z = Y·X`) mixes the four
voices by literally adding and subtracting them every sample, the native
equivalent of the original's octonion multiply. On top of that:

- Each voice continuously morphs **sine → triangle → saw → vocoder/formant**
  (the original's 3-point morph circle extended with vocoder as a 4th
  waypoint), with `FAN` spreading the 4 voices around that circle.
- A new **FM AMOUNT** knob cross-modulates the four voices' phases in a
  circular chain (voice 0 → 1 → 2 → 3 → 0) — the explicit "frequency
  modulation between all 4 waves" layered on top of the quaternion mixing.
- Vocoder mode blends a fixed formant curve (`FORMANT` knob morphs between
  two vowels) with a live sidechain-input filterbank envelope (`SC MIX`),
  so it works as a self-contained formant voice *or* a true carrier +
  external-modulator vocoder.
- Quaternions are associative (octonions aren't), so CASCADE/ASSOC modes
  don't carry over unchanged. The third algebra mode is **CONJUGATE**
  (`Y·X·Y⁻¹`), the textbook quaternion rotation formula, in place of the
  original's non-associativity trick.

Everything else — the harmonic-series sequencer (DRONE/LADDER/WALK/PRIME/
FRACTAL), SPIN/DRIFT rotor, HAUNT's ghost/memory/swap/stasis intrusions, and
the Bode shifter's BALANCE/FEEDBACK/FB TIME/MIX — is a direct port.

Quaternion ships two factory programs (host program list, or `getNumPrograms`
in a DAW's plugin browser): **Default** and **Ambient Serenity** — the same
DRONE pattern, slow rotor, and CONJUGATE algebra as Serenity below, recreated
within Quaternion's own engine as a gentler starting patch.

### Control map

| Section | Control | What it does |
| --- | --- | --- |
| Voices | WAVE / FAN | Position + spread around the sine→tri→saw→vocoder morph circle |
| Voices | FORMANT | Vowel morph for the vocoder waypoint |
| Voices | SC MIX | Blend fixed formant curve vs. live sidechain-input filterbank |
| Sequence | PATTERN | DRONE / LADDER / WALK / PRIME / FRACTAL harmonic-number assignment |
| Sequence | RATE / RANGE / GLIDE / GATE / ENV | Step clock, harmonic ceiling, portamento, per-step trigger odds, pluck decay |
| Quaternion | ALGEBRA | ROTATE (`Y·X`) / CASCADE (`W·(Y·X)`) / CONJUGATE (`Y·X·Y⁻¹`) |
| Quaternion | SPIN / DRIFT / DEPTH | Rotor speed, axis random-walk, dry(X)↔rotated(Z) mix |
| Quaternion | FM AMOUNT | Circular cross-voice phase modulation depth |
| Shift | SHIFT HZ / BALANCE / FEEDBACK / FB TIME / SHIFT MIX | Bode shifter: Δf, up↔ring-mod↔down, barberpole feedback, dry/wet |
| Memory | HAUNT | Frequency of ghost/memory/swap/stasis intrusions |
| Output | OUTPUT | Master level |

MIDI note input sets the root pitch (last-note-priority, matching the
originals' single-drone-root design); the harmonic sequencer spreads
integer multiples of that root across the 4 voices.

## Serenity

A free-running ambient instrument — load it and it plays on its own, no
MIDI note required. Five `PadVoice`s hold a 5-degree scale chord (MAJOR
PENT / MINOR PENT / DORIAN / LYDIAN) spread across two octaves; each pad's
amplitude swells and fades on its own random-period clock (4–14s), so the
chord breathes unevenly rather than pulsing in lockstep. A shared root
slowly random-walks by a few semitones, bounded by `DRIFT`. Independently,
a `ChimeBank` schedules short two-oscillator FM "bell" hits at random scale
degrees, random octaves, and random pan, at an average rate set by
`DENSITY`. Both layers feed a small feedback-delay network (`SPACE`) built
from three non-commensurate, individually-damped delay lines. An incoming
MIDI note re-centers the root but doesn't gate the sound — Serenity is
always audible while loaded.

### Control map

| Section | Control | What it does |
| --- | --- | --- |
| Pads | SCALE | MAJOR PENT / MINOR PENT / DORIAN / LYDIAN degree table |
| Pads | BRIGHTNESS | Lowpass cutoff on the pad bus |
| Pads | DRIFT | How far the shared root is allowed to random-walk |
| Chimes | DENSITY | Average rate of random FM bell hits |
| Space | SPACE | Feedback-delay wet/dry mix |
| Output | OUTPUT | Master level |

## Getting the plugin

Every push builds VST3 + Standalone binaries for Windows, macOS, and Linux
via GitHub Actions. Go to the **Actions** tab → the latest **Build
Quaternion VST3** run → download the `Quaternion-VST3-<platform>` or
`Serenity-VST3-<platform>` artifact for your OS, then drop the `.vst3` into
your system's VST3 folder:

- Windows: `C:\Program Files\Common Files\VST3\`
- macOS: `~/Library/Audio/Plug-Ins/VST3/` or `/Library/Audio/Plug-Ins/VST3/`
- Linux: `~/.vst3/`

Pushing a tag like `v1.0.0` also cuts a GitHub Release with zipped binaries
attached, for a permanent download link instead of a per-run artifact.

## Building locally

Requires CMake 3.22+ and a C++17 compiler. JUCE is fetched automatically.
Building the default target builds both Quaternion and Serenity.

```sh
cmake -B build -S .
cmake --build build --config Release
```

On Linux, install dev packages first (see `.github/workflows/build.yml` for
the exact list — ALSA, X11, GTK3, WebKit2GTK dev headers).

## License

MIT, same as the originals.
