# LARSEN // CAYLEY

A Max 9 patch (`larsen-cayley.maxpat`) that generates its own sound: four
"Cayley" voices are cross-mixed by a spinning rotor, dumped into a **Larsen**
feedback loop (a modulated delay + resonant filter run at or past unity gain,
the same runaway-squeal principle behind a mic-into-speaker howl or a
feedback-effect guitar pedal), then pushed through a driven tanh distortion
and a stock Schroeder reverb (4 parallel combs + 2 series allpass, built from
plain `tapin~`/`tapout~`/`vd~` — no external objects required).

Nothing needs to be plugged in: the patch seeds itself, so turning `ENGINE`
on is enough to hear it drift. Everything is built from **stock Max/MSP
objects only**, so it opens in vanilla Max 9 with no external dependencies.

## Signal flow

```
4x cycle~ (slow random-walk pitch, drunk -> line~)
        |
        v
  rotor (phasor~ -> cos~ / cos~+0.25) rotates voice pairs (0,1) and (2,3)
        |
        v
  Cayley cross-mix: rotated0 + rotated1 + crossAmt*(rotated2 * rotated3)
        |
        v
  Larsen loop: tapin~ -> vd~ (wandering delay time) -> reson~ (wandering
  center freq) --fed back into the mix at a controllable gain, >1.0 = self-
  oscillates
        |
        v
  distortion: *~ drive -> expr~ tanh($f1) -> crossfade with dry
        |
        v
  reverb: 4x comb (tapin~/tapout~ + feedback) -> 2x allpass -> crossfade with
  dry
        |
        v
  output gain -> expr~ tanh($f1) safety soft-limiter -> meter~ -> dac~ 1 2
```

## Controls

| Control | Effect |
| --- | --- |
| ENGINE (toggle) | Starts/stops the three background modulation clocks (voice pitch, Larsen delay/filter, reverb/drive macro drift) |
| PANIC (button) | Instantly zeroes the Larsen feedback gain if it runs away |
| SPIN | Rotor rate — how fast the Cayley cross-mix rotates |
| CROSS | Depth of the octonion-style nonlinear cross term |
| LARSEN FEEDBACK | Feedback gain in the delay/resonator loop; above ~1.0 it self-oscillates independent of the Cayley input |
| DISTORTION DRIVE / MIX | Pre-gain into the tanh waveshaper, and dry/wet blend |
| REVERB MIX | Dry/wet blend of the Schroeder reverb |
| OUTPUT GAIN | Final level before the safety limiter |

The three clocks each drive an independent `drunk` random walk (voice pitch,
delay time + filter frequency, and left for future macro parameters), smoothed
through `line~` ramps — so the patch keeps wandering into new territory on its
own rather than looping or settling into a fixed drone.

## Usage

Open `larsen-cayley.maxpat` in Max 9, turn on audio (or the transport bar's
speaker icon), switch `ENGINE` on, and raise `LARSEN FEEDBACK` past the
midpoint while nudging `SPIN`. Keep `PANIC` within reach — feedback gains
above unity are the point, but they can get loud fast.

This lives alongside `original-html/shift.html` and `original-html/cayley-ii.html`
as a self-contained prototype; it was not ported into the JUCE `Quaternion`
plugin in `Source/`, which remains SHIFT + CAYLEY II only.

## If Max crashes on load or on enabling audio

The patch itself has been validated (every patch cord references a real,
in-range inlet/outlet, and no two boxes' rectangles overlap — see the
generator script's self-checks), so a crash is unlikely to be caused by the
patch content itself. Two things worth checking on the Max side first:

- **License state.** If Max's title bar shows "saving disabled" or a
  "start trial or authorize" banner, it's running unlicensed. Crash reports
  whose stack trace lives entirely in Max's own package/menu bookkeeping
  (`packages_cloneinfo`, `dictionary_clone`, `jmenu_updatecommand` — nothing
  from this patch) point at that trial/license-check code path, which seems
  to get exercised whenever a window gains focus or audio is turned on.
  Authorize or properly start the trial, then retry.
- **Isolate from the patch.** Open a brand-new blank patcher, click around
  between windows, and toggle audio on. If Max still crashes with the same
  signature, it confirms the instability is in the Max install/license state,
  not in `larsen-cayley.maxpat`.

The patch was built and statically verified in a non-Max environment (no
Max 9 install available to test-load it directly), so if something behaves
unexpectedly beyond the above, note which object/section and it can be
adjusted.
