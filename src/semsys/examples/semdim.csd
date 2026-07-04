<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semdim.csd
;
; semdim reports the embedding (latent) dimension of a loaded model. Use it to
; size the arrays that hold embeddings, or to verify the model you loaded.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define MODEL_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/all-MiniLM-L6-v2/all-MiniLM-L6-v2-e2e" #

handle@global:i = semload(256, $MODEL_DIR)

instr 1
    ldim:i = semdim(handle)
    prints("latent dimension = %d\n", ldim)
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
