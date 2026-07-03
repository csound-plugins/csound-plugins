# semsttsubmitarray

## Abstract

Submit a buffer of audio samples for transcription (asynchronous, non-blocking).

## Description

`semsttsubmitarray` transcribes a 1-D array of audio samples. The samples are wrapped in an
in-RAM WAV (engine `sr`, mono) and handed to the background worker started by
[semsttload](semsttload.md); the graph resamples to the model rate internally.

The array length is whatever you filled, it is **not** limited by `ksmps`. To transcribe a
live signal captured sample-by-sample, use [semsttsubmitlive](semsttsubmitlive.md) instead,
which accumulates an a-rate signal into a window.

Audio longer than the model window (~30 s for Whisper) is **segmented automatically**: the
buffer is split into max 30 s chunks, every chunk is transcribed, and the texts are joined into a single
result. One submit yields one combined transcription regardless of length.

## Syntax

```csound
semsttsubmitarray(handle:i, samples:i[])
```

## Arguments

* `handle:i`: handle returned by [semsttload](semsttload.md).
* `samples:i[]`: mono audio samples (engine `sr`, range -1..1).

## Output

## Execution Time

* Init

## Examples

```csound
h@global:i = semsttload("path/to/model_e2e", 448, 8)

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
