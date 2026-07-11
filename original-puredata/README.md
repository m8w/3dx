# LARSEN // CAYLEY (Pure Data port)

A Pd-vanilla port of the Max 9 `larsen-cayley.maxpat` patch — same design:
four Cayley voices cross-mixed by a spinning rotor, into a Larsen (runaway
feedback) delay/resonator loop, into a driven tanh distortion stage, into a
Schroeder reverb, to a safety-limited output. Built entirely from Pd's own
stock objects; no externals, no extra libraries to install.

## Why this exists

Max 9 kept crashing on the original setup — twice in Max's own internal
package/license code, once in Apple's Metal GPU shader compiler, none of it
caused by the patch itself. Pd has no licensing system (nothing to crash
there) and its default UI runs on plain Tcl/Tk canvas drawing rather than a
GPU-accelerated interface, so the second failure mode is much less likely
too. It's also free, so there's no cost risk in trying it.

## What changed in the translation

- `cycle~` → `osc~` (identical behavior)
- `reson~` → `bp~` — Pd's bandpass filter has no separate gain inlet, just
  audio/frequency/Q (3 inlets), which is actually simpler than Max's reson~
  (whose real inlet order — audio/gain/frequency/Q — is what caused a bug
  in the Max version's first draft).
- `tapin~`/`tapout~`/`vd~` → `delwrite~`/`delread~`/`vd~`. Pd links a writer
  to its readers by a **shared name string** (like `send~`/`receive~`), not
  a patch cord — so there's no cord between `delwrite~ larsendelay 2000`
  and `vd~ larsendelay` even though they're functionally paired.
- Max's `drunk <range> <step>` (bounded random walk) has no vanilla Pd
  equivalent, so every use is rebuilt from primitives: `random` → recenter
  → accumulate → `clip` → `trigger` (fans out to the downstream consumer
  and back into the accumulator for next time).
- `toggle`/`button`/`slider` → Pd's `tgl`/`bng`/`vsl` GUI objects, which use
  a long fixed-order attribute list (size, colors, send/receive names...)
  in the `.pd` text format.

## Confidence level

Everything DSP-related (oscillators, filters, delays, the arithmetic
network, the distortion/reverb signal flow) is built from objects I'm
confident about, and every connection in the generator was self-checked to
reference a real, declared object. The one part I'm least certain of
pixel/attribute-perfect is the **GUI object attribute lists** (`tgl`/`bng`/
`vsl`/`vu` — the long strings of numbers for size, color, position). If a
slider or toggle looks visually off, that's the first thing to check — it
shouldn't affect whether the patch makes sound, just how the controls look.

I don't have a Pd install to test this against directly, so treat the first
load the same way as the Max version originally was: a real test, not a
guarantee.

**Fixed after first real-world test:** Pd's `expr~` uses `$v1` for signal
(vector) inlets — `$f1`/`$i1` are float/int-only in Pd, unlike Max where
`$f1` covers both. Both `expr~ tanh(...)` calls (distortion waveshaper and
output limiter) originally used `$f1` and failed to instantiate, which
silently broke every signal connection touching them — including the
entire path to `dac~`. That's now `expr~ tanh($v1)` in both places.

## Usage

Open `larsen-cayley.pd` in Pd (vanilla, no extra libraries needed), turn on
DSP (`Media → DSP On`, or `Ctrl/Cmd+/`), and it should be audible
immediately — the 4 voices start at real pitches and a `loadbang` sets sane
starting levels on every gain stage. Toggle `ENGINE` on to start the
background modulation (pitch/delay/filter wander); `PANIC` instantly zeroes
the Larsen feedback gain if it runs away.
