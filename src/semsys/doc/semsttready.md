# semsttready

## Abstract

Poll whether a finished transcription is waiting to be read.

## Description

`semsttready` returns `1` when at least one transcription has completed and is waiting in
the result queue, else `0`. It is a cheap, non-blocking k-rate poll used to gate
[semsttresult](semsttresult.md): submit audio with a `semsttsubmit*` opcode, poll
`semsttready`, and read the text when it turns `1`.
Because results are queued FIFO on the handle configured by [semsttload](semsttload.md),
`semsttready` stays `1` while more than one result is pending, read one per pass with [semsttresult](semsttresult.md) until it returns `0`.

## Syntax

```csound
ready:k = semsttready(handle:i)
```

## Arguments

* `handle:i`: handle returned by [semsttload](semsttload.md).

## Output

* `ready:k`: `1` if a transcription is ready to read, else `0`.

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
; semsttready.csd
;
; semsttready is a cheap non-blocking k-rate poll: 1 when a finished transcription
; is waiting in the result queue. Use it to gate semsttresult. Here: submit a file,
; poll every k-pass, read the text when it turns 1.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 128
nchnls = 1
0dbfs = 1

; end-to-end STT model dir: must contain model.onnx; keep model.onnx.data there too if present
#define STT_DIR # "path/to/model_e2e" #
#define AUDIO   # "path/to/spoken.wav" #

h@global:i = semsttload($STT_DIR, 448, 64)

instr SUBMIT
    semsttsubmitfile(h, $AUDIO)
endin

instr POLL
    r:k = semsttready(h)              ; non-blocking poll
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
* [semsttsubmitlive](semsttsubmitlive.md)
* [semsttresult](semsttresult.md)

## Credits

Pasquale Mainolfi, 2026
