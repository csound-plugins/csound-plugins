# semsttsubmitarray

## Abstract

Submit a buffer of audio samples for transcription (asynchronous, non-blocking).

## Description

`semsttsubmitarray` transcribes a 1-D array of audio samples. The samples are wrapped in an
in-RAM WAV (engine `sr`, mono) and handed to the background worker started by
[semsttload](semsttload.md); the graph resamples to the model rate internally.

The array length is whatever you filled — it is **not** limited by `ksmps`. To transcribe a
live signal captured sample-by-sample, use [semsttsubmitlive](semsttsubmitlive.md) instead,
which accumulates an a-rate signal into a window.

Like all `semstt*` submits it is **asynchronous**: it only **enqueues** the audio and
returns immediately. Read the result with [semsttready](semsttready.md) /
[semsttresult](semsttresult.md). If the queue is full, the newest submit is dropped with a
warning and already queued jobs stay queued.

The samples must be at **engine `sr`** (the rate at which Csound arrays/signals are
produced); the WAV header records that rate so the model's decoder resamples correctly.

## Syntax

```csound
semsttsubmitarray(handle:i, samples:i[])
```

## Arguments

* `handle:i`: handle returned by [semsttload](semsttload.md).
* `samples:i[]`: mono audio samples (engine `sr`, range -1..1).

## Output

* none. The transcription is retrieved with [semsttresult](semsttresult.md).

## Execution Time

* Init

## Examples

```csound
h@global:i = semsttload("path/to/whisper_e2e", 448, 8)

instr transcribe
    ; samps:i[] filled elsewhere with mono audio at engine sr
    semsttsubmitarray(h, samps)
endin
```

## See also

* [semsttload](semsttload.md)
* [semsttsubmitfile](semsttsubmitfile.md)
* [semsttsubmitft](semsttsubmitft.md)
* [semsttsubmitlive](semsttsubmitlive.md)
* [semsttready](semsttready.md)
* [semsttresult](semsttresult.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
