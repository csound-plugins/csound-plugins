# semsttresult

## Abstract

Read the next finished transcription (and its length) from a speech-to-text context.

## Description

`semsttresult` returns the oldest completed transcription (FIFO order) from the handle's
result queue and removes it, along with the transcription's **length** in characters. If no
result is waiting it returns an empty string with length `0`, so gate it with
[semsttready](semsttready.md).

The queue is **shared by the whole handle**: a result is not tied to the submit (or the
instrument) that produced it, `semsttresult` just returns the next finished one. If several
instruments submit to the same handle, the texts interleave in FIFO order. For independent
flows, use a separate handle per flow (see [semsttload](semsttload.md)).

## Syntax

```csound
text:S, length:k = semsttresult(handle:i)
```

## Arguments

* `handle:i`: handle returned by [semsttload](semsttload.md).

## Output

* `text:S`: the next transcription, or `""` if none is ready.
* `length:k`: its length in characters (`0` for an empty / no-speech result).

## Execution Time

* Performance

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
-o dac2
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semsttresult.csd
;
; semsttresult pops the oldest finished transcription (FIFO) from the STT worker,
; plus its length in characters; gate it with semsttready. Here: submit a file,
; poll, then read and print the transcription.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 128
nchnls = 1
0dbfs = 1

; end-to-end STT model dir: must contain model.onnx; keep model.onnx.data there too if present
#define STT_DIR # "path/to/model_e2e" #
#define AUDIO   # "path/to/spoken.wav" #

h@global:i = semsttload($STT_DIR, 448, 64)

; submit the file once at init; transcription runs on the worker thread
instr SUBMIT
    semsttsubmitfile(h, $AUDIO)
endin

; poll until the transcription is ready, read it, stop
instr POLL
    r:k = semsttready(h)
    if (r == 1) then
        text:S, len:k = semsttresult(h)   ; pops the oldest result
        if (len > 0) then                 ; skip empty (no-speech) windows
            println("TRANSCRIPTION: %s\n", text)
        endif
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
* [semsttsubmitlive](semsttsubmitlive.md)
* [semsttready](semsttready.md)

## Credits

Pasquale Mainolfi, 2026
