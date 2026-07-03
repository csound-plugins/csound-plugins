# semsttsubmitft

## Abstract

Submit a function table of audio samples for transcription (asynchronous, non-blocking).

## Description

`semsttsubmitft` transcribes the samples held in a function table. The table contents are
wrapped in an in-RAM WAV (engine `sr`, mono) and handed to the background worker started by
[semsttload](semsttload.md); the graph resamples to the model rate internally. The whole
table (`flen` samples) is used.

Audio longer than the model window (~30 s for Whisper) is **segmented automatically**: the
table is split into max 30 s chunks, every chunk is transcribed, and the texts are joined into a single
result. One submit yields one combined transcription regardless of length.

## Syntax

```csound
semsttsubmitft(handle:i, ftable:i)
```

## Arguments

* `handle:i`: handle returned by [semsttload](semsttload.md).
* `ftable:i`: function table number holding mono audio samples (engine `sr`).

## Output

## Execution Time

* Init

## Examples

```csound
h@global:i = semsttload("path/to/model_e2e", 448, 8)
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
