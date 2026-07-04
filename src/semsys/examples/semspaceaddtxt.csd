<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semspaceaddtxt.csd
;
; semspaceaddtxt embeds a sentence with the text model and appends the vector to
; an in-memory space (RAM-only; use semspacesave to persist). Here: fill an empty
; space with a few sentences, then query it to confirm the nearest match.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

#define MODEL_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/all-MiniLM-L6-v2/all-MiniLM-L6-v2-e2e" #

e_handle@global:i = semload(256, $MODEL_DIR)
s_handle@global:i = semspace(e_handle)          ; empty in-memory space

instr build
    semspaceaddtxt(s_handle, e_handle, "a warm analog texture")
    semspaceaddtxt(s_handle, e_handle, "a bright metallic hit")
    semspaceaddtxt(s_handle, e_handle, "a deep resonant drone")
    prints("space populated\n")
    turnoff
endin

instr query
    neighs:i[][], scores:i[] = semspacequerytxt(s_handle, e_handle, "warm analog sound", 1)
    prints("best score = %.4f\n", scores[0])
    turnoff
endin

</CsInstruments>
<CsScore>
i "build" 0 0.1
i "query" 1 0.1
</CsScore>
</CsoundSynthesizer>
