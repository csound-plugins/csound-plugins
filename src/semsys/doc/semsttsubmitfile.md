# semsttsubmitfile

## Abstract

Submit a PCM16 WAV file for transcription (asynchronous, non-blocking).

## Description

`semsttsubmitfile` reads `path` from disk and hands the audio to the background worker
created by [semsttload](semsttload.md). Only **PCM16 WAV** is accepted; any other format
raises an init error (convert to WAV first, or load it into a function table and use
[semsttsubmitft](semsttsubmitft.md)).

The opcode only enqueues the job and returns. Poll with [semsttready](semsttready.md), then
read the transcription with [semsttresult](semsttresult.md).

Audio longer than the model window (~30 s for Whisper) is **segmented automatically**: the
file is split into max 30 s chunks, every chunk is transcribed, and the texts are joined into a
single result. So one `semsttsubmitfile` call yields one combined transcription regardless
of length.

## Syntax

```csound
semsttsubmitfile(handle:i, path:S)
```

## Arguments

* `handle:i`: handle returned by [semsttload](semsttload.md).
* `path:S`: path to an audio file.

## Output

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
