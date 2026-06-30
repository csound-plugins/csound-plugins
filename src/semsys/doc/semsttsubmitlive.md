# semsttsubmitlive

## Abstract

Capture a live a-rate signal and submit usable speech windows for transcription
(asynchronous, non-blocking).

## Description

`semsttsubmitlive` accumulates an a-rate mono signal across control blocks and submits
speech windows to the background worker created by [semsttload](semsttload.md).

`maxdur` is a **preferred** window length, not a hard periodic cut. After `maxdur`, the
backend analyzes the buffer with a lightweight energy gate:

* silent windows are dropped;
* windows with too little detected speech are held and merged with following audio when
  possible;
* speech in progress is not cut just because `maxdur` elapsed;
* a window is submitted when there is enough detected speech and a useful trailing silence
  boundary;
* submitted audio is trimmed around detected speech with a small pre/post pad;
* a hard safety cap prevents unbounded live accumulation (`max(maxdur, ~30 s)`).

The optional `trig` input forces a boundary on a rising edge. Even then, the backend still
checks whether there is enough speech before submitting. Use `trig` for explicit phrase
ends, manual gates, or an external VAD; omit it to use only the built-in gate.

Like all `semstt*` submits, the opcode only enqueues work and returns. Read results with
[semsttready](semsttready.md) and [semsttresult](semsttresult.md). If the fixed queue is
full, the newest window is dropped with a warning while already queued jobs stay queued.
When the live instrument instance deinitializes, any remaining buffered speech is flushed if
it is functional; insufficient speech is dropped. When the STT handle itself is destroyed,
pending jobs/results are discarded rather than drained.

With `SEMSYS_STT_DEBUG=1`, the opcode prints window decisions such as submitted speech
duration, dropped insufficient-speech windows, queue drops, and `job#` enqueue/dequeue/result
tracking.

`semsttsubmitlive` is not equivalent to [semsttsubmitfile](semsttsubmitfile.md) unless the
live window covers the same audio span. For file-style tests through the live path, use a
`maxdur` that is long enough to contain a useful phrase and let the gate close on silence.

## Syntax

```csound
semsttsubmitlive(handle:i, asig:a, maxdur:i)
semsttsubmitlive(handle:i, asig:a, maxdur:i, trig:k)
```

## Arguments

* `handle:i`: handle returned by [semsttload](semsttload.md).
* `asig:a`: mono audio signal to capture at engine `sr`.
* `maxdur:i`: preferred speech-window length in seconds. After this duration, the backend
  starts checking for a usable boundary; speech can continue accumulating up to the hard
  safety cap.
* `trig:k` (optional, default `0`): rising edge requests a forced boundary.

## Output

* none. The transcription is retrieved with [semsttresult](semsttresult.md).

## Execution Time

* Init (buffer allocation)
* Performance (capture + gated submit)

## Examples

```csound
sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

h_stt@global:i = semsttload("path/to/model_e2e", 448, 256)

instr LISTEN
    sig:a = inch(1)
    semsttsubmitlive(h_stt, sig, 2)
endin

instr POLL
    ready:k = semsttready(h_stt)
    if (ready == 1) then
        text:S, len:k = semsttresult(h_stt)
        if (len > 0) then
            println("[STT]: %s", text)
        endif
    endif
endin
```

## See also

* [semsttload](semsttload.md)
* [semsttsubmitfile](semsttsubmitfile.md)
* [semsttsubmitarray](semsttsubmitarray.md)
* [semsttsubmitft](semsttsubmitft.md)
* [semsttready](semsttready.md)
* [semsttresult](semsttresult.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
