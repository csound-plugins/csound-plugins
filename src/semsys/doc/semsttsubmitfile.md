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
<CsoundSynthesizer>
<CsOptions>
-o dac2
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semsttsubmitfile.csd
;
; semsttsubmitfile reads a PCM16 WAV and hands it to the STT worker (non-blocking).
; Audio longer than the model window is segmented automatically. Offline flow:
; submit -> poll semsttready -> read semsttresult. The commented lines show the
; array / function-table submit paths that reach the same worker.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 128
nchnls = 1
0dbfs = 1

; end-to-end STT model dir: must contain model.onnx; keep model.onnx.data there too if present
#define STT_DIR # "path/to/model_e2e" #
#define AUDIO   # "path/to/spoken.wav" #

faudio@global:i = ftgen(0, 0, 0, 1, $AUDIO, 0, 0, 0)

h@global:i = semsttload($STT_DIR, 448, 64)

; submit the file once at init; transcription runs on the worker thread
instr SUBMIT
    semsttsubmitfile(h, $AUDIO)
    ; alternative submit paths (same async result):
    ; audio_arr:i[] = init(ftlen(faudio))
    ; copyf2array(audio_arr, faudio)
    ; semsttsubmitarray(h, audio_arr)
    ; semsttsubmitft(h, faudio)
endin

; poll until the transcription is ready, print it, stop
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
* [semsttsubmitarray](semsttsubmitarray.md)
* [semsttsubmitft](semsttsubmitft.md)
* [semsttsubmitlive](semsttsubmitlive.md)
* [semsttready](semsttready.md)
* [semsttresult](semsttresult.md)

## Credits

Pasquale Mainolfi, 2026
