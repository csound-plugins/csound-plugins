# semsttsubmitft

## Abstract

Submit a function table of audio samples for transcription (asynchronous, non-blocking).

## Description

`semsttsubmitft` transcribes the samples held in a function table. The table contents are
wrapped in an in-RAM WAV (engine `sr`, mono) and handed to the background worker started by
[semsttload](semsttload.md); the graph resamples to the model rate internally. The whole
table (`flen` samples) is used.

Like all `semstt*` submits it is **asynchronous**: it only **enqueues** the audio and
returns immediately. Read the result with [semsttready](semsttready.md) /
[semsttresult](semsttresult.md). If the queue is full, the newest submit is dropped with a
warning and already queued jobs stay queued.

The samples are assumed to be at **engine `sr`**. A table loaded from a soundfile (GEN01) is
stored at the file's sample rate, not necessarily the engine rate — if they differ the
pitch/speed will be wrong; load such tables at engine `sr` or use
[semsttsubmitfile](semsttsubmitfile.md) (which passes the file to the model's own decoder).

## Syntax

```csound
semsttsubmitft(handle:i, ftable:i)
```

## Arguments

* `handle:i`: handle returned by [semsttload](semsttload.md).
* `ftable:i`: function table number holding mono audio samples (engine `sr`).

## Output

* none. The transcription is retrieved with [semsttresult](semsttresult.md).

## Execution Time

* Init

## Examples

```csound
h@global:i = semsttload("path/to/whisper_e2e", 448, 8)
audio:i = ftgen(0, 0, 0, 1, "speech.wav", 0, 0, 1)   ; mono table

instr transcribe
    semsttsubmitft(h, audio)
endin
```

## See also

* [semsttload](semsttload.md)
* [semsttsubmitfile](semsttsubmitfile.md)
* [semsttsubmitarray](semsttsubmitarray.md)
* [semsttsubmitlive](semsttsubmitlive.md)
* [semsttready](semsttready.md)
* [semsttresult](semsttresult.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
