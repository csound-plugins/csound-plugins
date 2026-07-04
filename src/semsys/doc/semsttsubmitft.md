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
<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semsttsubmitft.csd
;
; semsttsubmitft transcribes the samples held in a function table (engine sr,
; mono). The whole table (flen samples) is used and segmented automatically if it
; is longer than the model window. Here: load a WAV into a mono table and submit it.
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
    semsttsubmitft(h, faudio)
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
* [semsttsubmitarray](semsttsubmitarray.md)
* [semsttsubmitlive](semsttsubmitlive.md)
* [semsttready](semsttready.md)
* [semsttresult](semsttresult.md)

## Credits

Pasquale Mainolfi, 2026
