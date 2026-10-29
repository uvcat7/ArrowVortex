# Adjust Sync: BPM and offset detection

Notes on the "Find BPM" feature in the **Adjust Sync** dialog
(`Dialogs/AdjustSync.cpp` → `Editor/FindTempo.cpp` → `Editor/FindOnsets.cpp`),
covering a set of correctness fixes and two changes that were tried, measured
and rejected.

## TL;DR

**The detector was already accurate.** Against a reference pack of correctly
synced charts it picks the right BPM every time and lands the offset within
about 12 ms. The fixes here are correctness and robustness work — a broken
cancellation path, a window misalignment, an out-of-bounds write, a redundant
allocation, and a misleading percentage in the UI. None of them claim an
accuracy improvement, and measurements confirm they do not move the results:

| | before | after |
|---|---|---|
| BPM correct, reference pack | 5 / 5 | 5 / 5 |
| Offset mean absolute error, reference pack | 11.9 ms | 12.3 ms |
| BPM correct, second corpus | 12 / 12 | 12 / 12 |
| Offset mean absolute error, second corpus | 28.1 ms | 28.2 ms |

Two plausible-looking "fixes" were **rejected** after measurement. They are
documented below and in code comments, because both look obviously correct on
inspection and will otherwise be re-attempted.

## How the feature works

```
AdjustSync.cpp   onFindBPM()  → TempoDetector::New(startTime, length)
                 onTick()     → polls progress, formats the candidate list
                 onApplyBPM() → writes the offset and a BPM change at row 0

FindTempo.cpp    TempoDetectorImp::exec(), on a background thread
                  1. FindOnsets()      – aubio "complex" onset detector
                  2. CalculateBPM()    – for each candidate beat interval, wrap
                                         every onset into that interval and
                                         score how tightly they clump; a fitted
                                         cubic baseline is subtracted so long
                                         intervals are not favoured just for
                                         being long. Keeps the top 3.
                  3. CalculateOffset() – for the winning intervals, pick the
                                         beat phase with the most support, then
                                         decide on-beat vs off-beat using the
                                         waveform's amplitude slope.
```

---

## How to validate a change to these files

**Measure against material with a known-exact grid.** The reference pack used
here is a set of correctly synced ITG charts including a 120 BPM metronome whose
clicks sit on exact 0.25 s boundaries, which makes it possible to check the
onset detector directly with no charting convention in the way.

**Do not use hand-synced charts as ground truth.** Charts synced by feel carry
their own systematic bias — easily tens of milliseconds, and consistent across a
whole pack, because it comes from the charter's setup and habits rather than
from any individual song. Measuring the detector against them will make it look
wrong in whichever direction that pack leans. A 12-chart corpus was used that
way early in this work and produced a confident, precisely quantified, and
entirely wrong conclusion; see "Rejected: clearing aubio's onset delay" below.

Note also that ITG-convention charts carry a deliberate **+0.009 s** bias, so
even a correct detection reads about 9 ms "off" against them.

---

## Fix 1 — Cancellation never worked

`System/Thread.h`, `System/Thread.cpp`, `Editor/FindTempo.cpp`

`BackgroundThread::getStopToken()` returned `thread.get_stop_token()`. A
default-constructed `std::jthread` has no stop state at all, and `start()`
replaces the whole `jthread` object. So:

- `TempoDetectorImp` took its token *before* `start()` and got a permanently
  dead token — `stop_possible()` was `false`, and the `MarkProgress`
  cancellation check could never fire.
- `Sound::Thread` took its token from *inside* the worker, which races with the
  assignment to `thread` in `start()`.

`BackgroundThread` now owns a `std::stop_source` created in its constructor, so
a token is valid before, during and after `start()`.

**Impact.** Closing the Adjust Sync dialog, opening a different file, or
pressing "Find BPM" again mid-run previously blocked the UI thread in `join()`
until the whole analysis finished — roughly a second for a three-minute song, so
a stutter rather than a hang. Also removes the data race in the audio loader.
`OggConversionThread` does not poll the token and is unaffected. No effect on
detection results.

## Fix 2 — Window misalignment in `GapConfidence`

`Editor/FindTempo.cpp`

`GapConfidence` slides a Hamming window over the wrapped onset histogram to
score a candidate beat position. When the window wrapped past position 0, the
code clamped the loop start to 0 and then restarted the window at *its own*
index 0 instead of continuing from where the wrapped part left off. Low window
weights were counted twice and the high ones were never reached.

A single onset sitting exactly on the candidate position must always score the
window's peak weight of 1.0. It did not:

| gap position | before | after |
|---|---|---|
| 0 | 0.08 | 1.00 |
| 100 | 0.10 | 1.00 |
| 512 | 0.54 | 1.00 |
| 1023 and above | 1.00 | 1.00 |

**Impact.** Any beat phase landing in the first half-window of an interval was
scored far below its true support and so could never win — a blind spot covering
roughly the first 2–3% of every candidate interval. The fix can only raise a
score that was being suppressed, never lower a correct one. Effect on results is
small and mixed: a fraction of a millisecond on most songs, and on the reference
pack's sparse `sync test` it replaced a spurious third candidate of 160.111 BPM
with a sensible 128.160.

## Fix 3 — One-element histogram overflow

`Editor/FindTempo.cpp`

`GapData::wrappedOnsets` was sized `bufferSize`, but callers index it with
`(int)fmod(onsetPos, intervalf)` where `intervalf` is a `double` that can sit a
fraction above the integer `bufferSize`. At the bottom of the BPM range — a
candidate rounded down to exactly 89 BPM inside `RoundBPMValues` — that index
reaches `bufferSize` itself, one past the end. `CalculateOffset` already guarded
this with `+ 1.0`; the path in `CalculateBPM` did not. Out-of-bounds write, no
behaviour change.

## Fix 4 — Redundant slope computation

`Editor/FindTempo.cpp`

`AdjustForOffbeats` built a slope representation of the entire waveform, one
`double` per frame, once per BPM candidate — up to three times, with identical
results. On a ten-minute song at 48 kHz that is a ~230 MB allocation repeated
three times. Now computed once in `CalculateOffset` and shared, with a null
check that was previously missing. Performance only.

## Fix 5 — Misleading percentage in the candidate list

`Dialogs/AdjustSync.cpp`

Each candidate's fitness was divided by the **sum** of all candidates' fitness
and shown as a percentage. That made a perfectly good detection read as "53%"
purely because two other candidates happened to survive — with three candidates
the top one can never exceed 100% and can look as low as 34%. Fitness values are
relative scores with a fitted baseline subtracted; their absolute size means
nothing and they can go negative, so a share-of-sum is not a confidence.

Now scaled against the **best** candidate, so the top pick always reads 100% and
the others say how much weaker they are. Display only.

---

## Rejected: clearing aubio's onset delay

**The idea.** Onset positions come from `aubio_onset_get_last()`, which returns
`last_onset - delay`, where `delay` defaults to `4.6 × hop = 1177 samples`
(~25 ms) for the `"complex"` method. That reads like a latency allowance for
streaming callers, and ArrowVortex analyses a fully decoded buffer offline, so
subtracting it looks like a plain mistake.

**Why it was convincing.** Measured against a 12-chart hand-synced corpus, the
suggested offset was early by a mean of +26.5 ms, and sweeping the delay moved
that bias exactly one-for-one — 1 ms of bias per 1 ms of delay. Clearing it
dropped the mean signed error from +26.5 ms to +1.4 ms and halved the mean
absolute error. Every number pointed the same way.

**Why it is wrong.** The delay compensates for the group delay of aubio's peak
picker, which only confirms a peak several hops after the audio that caused it.
Checked against the metronome, whose clicks sit on exact 0.25 s boundaries:

| | mean error vs the true click attack |
|---|---|
| delay left at its default | **−5.2 ms** |
| delay cleared | **+19.3 ms** |

The detector is right as shipped. The 26 ms "bias" was the hand-synced corpus
being late, not the detector being early — which is exactly the failure mode the
validation note above warns about.

## Rejected: weighting every onset by loudness

**The idea.** `TempoDetectorImp::exec()` weights onsets by the local loudness of
the audio around them, but stops after 100. Every later onset keeps the
placeholder `1.0` from `FindOnsets`, and local loudness runs about 0.03–0.09, so
onsets 100 and up outweigh the first hundred roughly twenty to one. The
weighting is a step function of onset index, which cannot be intentional.

**Why it is not worth changing.** Extending the loop to every onset made no
measurable difference on full-length songs, and it broke the reference pack's
`sync test` — 17 seconds, 113 onsets, 128 BPM — demoting the correct BPM from
first to third and promoting a spurious 160 BPM above it. With few onsets to
work with, amplitude weighting lets a handful of loud hits dominate the
histogram; uniform weighting is more robust on sparse input.

Left as-is, with a comment, until there is a weighting scheme that measures
better on both sparse and dense material.

---

## Known weaknesses, not addressed

- **`AdjustForOffbeats` can tie at zero.** It compares on-beat against off-beat
  support by sampling `ComputeSlopes`, whose output is a ~50 ms plateau ending
  *at* each transient. A sample point landing after a transient reads zero, and
  if both candidates read zero the `sumA >= sumB` comparison silently picks the
  on-beat one. This is what turned the metronome into an off-beat result while
  the rejected delay change was in place. It is latent as things stand, but it
  is fragile.
- **The BPM search is hard-clamped to (89, 205] BPM** (`MinimumBPM` /
  `MaximumBPM`). `IntervalToBPM` cannot return anything outside that range, so
  an 85 or 210 BPM song can only ever come back as a multiple of its true tempo.
  This is the most likely explanation for a "detected the wrong BPM" report.
  Widening it is a real design tradeoff — more candidates, more octave
  confusion, longer runtime — and deserves its own change with measurements.
- **Octave errors inside the range are inherent.** A 100 BPM track with hi-hats
  on eighth notes legitimately reports 200 BPM. This is why three candidates are
  offered.
- **The offset is only ever determined modulo one beat.** Audio alone cannot say
  which beat is beat 0, so the result always lands in `(−oneBeat, 0]` and the
  "Move first beat" buttons still have to do the rest.
- **"Find BPM" analyses only the selected region** if a selection exists,
  otherwise the first 600 seconds. A stale selection produces a detection based
  on that fragment alone.
