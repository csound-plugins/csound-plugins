# semsttsubmitlive

## Abstract

Capture a live a-rate signal and submit usable speech windows for transcription
(asynchronous, non-blocking).

## Description

`semsttsubmitlive` accumulates an a-rate mono signal across control blocks and submits
speech windows to the background worker created by [semsttload](semsttload.md).

The backend analyzes the buffer with a lightweight energy gate:

* silent windows are dropped;
* windows with too little detected speech are held and merged with following audio when
  possible;
* speech in progress is not cut just because `maxdur` elapsed;
* a window is submitted when there is enough detected speech and a useful trailing silence
  boundary;
* submitted audio is trimmed around detected speech with a small pre/post pad;
* a hard safety cap prevents unbounded live accumulation.

The optional `trig` input forces a boundary on a rising edge. Even then, the backend still
checks whether there is enough speech before submitting. Use `trig` for explicit phrase
ends, manual gates, or an external VAD; omit it to use only the built-in gate.

`semsttsubmitlive` is not equivalent to [semsttsubmitfile](semsttsubmitfile.md) unless the
live window covers the same audio span. For file-style tests through the live path, use a
`maxdur` that is long enough to contain a useful phrase and let the gate close on silence.

## Syntax

```csound
semsttsubmitlive(handle:i, asig:a, maxdur:i)
semsttsubmitlive(handle:i, asig:a, maxdur:i, trig:k)
```

## Arguments

* `handle:i`: handle returned by [semsttload](semsttload.md).
* `asig:a`: mono audio signal to capture at engine `sr`.
* `maxdur:i`: preferred speech-window length in seconds. After this duration, the backend
  starts checking for a usable boundary; speech can continue accumulating up to the hard
  safety cap.
* `trig:k` (optional, default `0`): rising edge requests a forced boundary. Forces a boundary on a rising edge. Even then, the backend still checks whether there is enough speech before submitting. Use `trig` for explicit phrase ends or manual gates; omit it to use only the built-in gate.

## Output

## Execution Time

* Init
* Performance

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
-o dac2
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semsttsubmitlive.csd
;
; semsttsubmitlive captures a live a-rate signal and submits usable speech windows
; to the STT worker (built-in energy gate). This example is a voice-controlled
; latent space: mic -> semsttsubmitlive -> semsttresult (text) -> semspacequerytxt
; (nearest vector + score) -> a synth voice driven by the match score.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

#define STT_DIR # "path/to/model_e2e" #
#define EMB_DIR # "path/to/text_model_dir" #

; speech-to-text + embedding model
h_stt@global:i = semsttload($STT_DIR, 448, 64)
h_emb@global:i = semload(256, $EMB_DIR)

; build a small semantic space from a corpus, then open it
semspacebuild(h_emb, "corpus.espc", "corpus.txt")
h_space@global:i = semspace(h_emb, "corpus.espc")

; NOTE: use a SPEECH source. Whisper transcribes speech reliably; on music/singing it
; often returns empty, so windows come back with len == 0.
instr LISTEN
    sig:a = inch(1)                       ; live mic (or diskin2("path/to/spoken.wav", 1))
    semsttsubmitlive(h_stt, sig, 1)       ; preferred window length; closes on a speech boundary
endin

instr POLL
    r:k = semsttready(h_stt)
    if (r == 1) then
        text:S, tlen:k = semsttresult(h_stt)
        if (tlen > 0) then ; skip empty (no-speech) windows
            ; query the space with the spoken text -> nearest vector + score
            neighs:k[][], score:k[], kgate:k = semspacequerytxt(h_space, h_emb, text, 3)
            println("match score: %f", score[0])
            event("i", "VOICE", 0, 3, score[0]) ; the semantic match score drives a sound
        endif
    endif
endin

; a short tone whose pitch tracks the semantic match score
instr VOICE
    freq:i = 110 + p4 * 660
    env:a = expseg(0.001, 0.001, 1, p3 - 0.001, 0.0001)
    a1:a = poscil(0.5, freq)
    out(a1 * env, a1 * env)
endin

</CsInstruments>
<CsScore>
i "LISTEN" 0 100
i "POLL" 0 100
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semsttload](semsttload.md)
* [semsttsubmitfile](semsttsubmitfile.md)
* [semsttsubmitarray](semsttsubmitarray.md)
* [semsttsubmitft](semsttsubmitft.md)
* [semsttready](semsttready.md)
* [semsttresult](semsttresult.md)

## Credits

Pasquale Mainolfi, 2026
