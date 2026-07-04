<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semspaceaddaudio.csd
;
; semspaceaddaudio decodes a PCM16 WAV, embeds it with the audio model and appends
; the per-window vector(s) to an in-memory space (RAM-only; use semspacesave to
; persist). Here: add a sound, then query the space with the same sound (its
; self-match score should be close to 1).
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define AUDIO_MODEL # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/PANNs_CNN14_ONNX" #
#define WAV # "mallets_on_piano.wav" #

a_handle@global:i = semload(-1, $AUDIO_MODEL)   ; audio model, no sequence cap
s_handle@global:i = semspace(a_handle)          ; empty in-memory space

instr build
    semspaceaddaudio(s_handle, a_handle, $WAV)
    prints("audio added to space\n")
    turnoff
endin

instr query
    neighs:i[][], scores:i[] = semspacequeryaudio(s_handle, a_handle, $WAV, 1)
    prints("self-match score = %.4f\n", scores[0])
    turnoff
endin

</CsInstruments>
<CsScore>
i "build" 0 0.1
i "query" 1 0.1
</CsScore>
</CsoundSynthesizer>
