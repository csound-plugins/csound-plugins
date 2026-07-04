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
<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semsttsubmitarray.csd
;
; semsttsubmitarray transcribes a 1-D array of mono samples (engine sr, -1..1).
; The array length is whatever you filled, NOT limited by ksmps. Here: load a WAV
; into a table, copy it into an i-array, and submit that array to the worker.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 128
nchnls = 1
0dbfs = 1

#define STT_DIR # "path/to/model_e2e" #
#define AUDIO   # "spoken.wav" #

faudio@global:i = ftgen(0, 0, 0, 1, $AUDIO, 0, 0, 1)   ; mono table (GEN01, channel 1)
h@global:i = semsttload($STT_DIR, 448, 64)

instr SUBMIT
    samps:i[] = init(ftlen(faudio))
    copyf2array(samps, faudio)
    semsttsubmitarray(h, samps)
endin

instr POLL
    r:k = semsttready(h)
    if (r == 1) then
        text:S, len:k = semsttresult(h)
        println("TRANSCRIPTION: %s\n", text)
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i "SUBMIT" 0 0.1
i "POLL"   0 120
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semsttload](semsttload.md)
* [semsttsubmitfile](semsttsubmitfile.md)
* [semsttsubmitft](semsttsubmitft.md)
* [semsttsubmitlive](semsttsubmitlive.md)
* [semsttready](semsttready.md)
* [semsttresult](semsttresult.md)

## Credits

Pasquale Mainolfi, 2026
