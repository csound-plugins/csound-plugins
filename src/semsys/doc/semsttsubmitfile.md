# semsttsubmitfile

## Abstract

Submit an audio file for transcription (asynchronous, non-blocking).

## Description

`semsttsubmitfile` reads `path` from disk as raw bytes and hands it to the background worker
created by [semsttload](semsttload.md). The model graph's audio decoder handles the file
format and resampling internally, so WAV/MP3/etc. support depends on the exported graph.

The opcode only enqueues the job and returns. Poll with [semsttready](semsttready.md), then
read the transcription with [semsttresult](semsttresult.md).

The job goes into the handle's bounded FIFO. If the queue is full, the newest submit is
dropped with a warning and already queued jobs stay queued.

One submit is one model call. For Whisper-like models, audio longer than the model window
(~30 s) is not fully transcribed by a single call; split long files into model-sized
segments or use [semsttsubmitlive](semsttsubmitlive.md) for gated stream capture.

## Syntax

```csound
semsttsubmitfile(handle:i, path:S)
```

## Arguments

* `handle:i`: handle returned by [semsttload](semsttload.md).
* `path:S`: path to an audio file.

## Output

* none. The transcription is retrieved with [semsttresult](semsttresult.md).

## Execution Time

* Init

## Examples

```csound
sr = 44100
ksmps = 128
nchnls = 1
0dbfs = 1

h@global:i = semsttload("path/to/model_e2e", 448, 256)

instr SUBMIT
    semsttsubmitfile(h, "speech.wav")
endin

instr POLL
    ready:k = semsttready(h)
    if (ready == 1) then
        text:S, len:k = semsttresult(h)
        println("TRANSCRIPTION: %s", text)
        turnoff
    endif
endin
```

## See also

* [semsttload](semsttload.md)
* [semsttsubmitarray](semsttsubmitarray.md)
* [semsttsubmitft](semsttsubmitft.md)
* [semsttsubmitlive](semsttsubmitlive.md)
* [semsttready](semsttready.md)
* [semsttresult](semsttresult.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
