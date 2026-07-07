# Quaternion

A 4-voice VST3/Standalone instrument, ported from two Web Audio prototypes:

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

## Getting the plugin

Every push builds VST3 + Standalone binaries for Windows, macOS, and Linux
via GitHub Actions. Go to the **Actions** tab → the latest **Build
Quaternion VST3** run → download the `Quaternion-VST3-<platform>` artifact
for your OS, then drop the `.vst3` into your system's VST3 folder:

- Windows: `C:\Program Files\Common Files\VST3\`
- macOS: `~/Library/Audio/Plug-Ins/VST3/` or `/Library/Audio/Plug-Ins/VST3/`
- Linux: `~/.vst3/`

Pushing a tag like `v1.0.0` also cuts a GitHub Release with zipped binaries
attached, for a permanent download link instead of a per-run artifact.

## Building locally

Requires CMake 3.22+ and a C++17 compiler. JUCE is fetched automatically.

```sh
cmake -B build -S .
cmake --build build --config Release
```

On Linux, install dev packages first (see `.github/workflows/build.yml` for
the exact list — ALSA, X11, GTK3, WebKit2GTK dev headers).

## Control map

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

## License

MIT, same as the originals.
