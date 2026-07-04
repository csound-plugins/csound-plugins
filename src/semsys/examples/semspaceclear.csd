<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semspaceclear.csd
;
; semspaceclear empties a space (vector count -> 0) while keeping its dimension and
; allocated capacity, so it can be repopulated. Here: add + query, clear, then
; query again -- the match score collapses after the clear.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define MODEL_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/all-MiniLM-L6-v2/all-MiniLM-L6-v2-e2e" #

e_handle@global:i = semload(256, $MODEL_DIR)
s_handle@global:i = semspace(e_handle)

instr ADD
    semspaceaddtxt(s_handle, e_handle, "a warm analog texture")
    semspaceaddtxt(s_handle, e_handle, "a bright metallic hit")
    prints("space populated\n")
    turnoff
endin

instr QUERY_BEFORE
    neighs:i[][], scores:i[] = semspacequerytxt(s_handle, e_handle, "warm analog sound", 1)
    prints("before clear score = %.6f\n", scores[0])
    turnoff
endin

instr CLEAR
    semspaceclear(s_handle)
    prints("space cleared\n")
    turnoff
endin

instr QUERY_AFTER
    neighs:i[][], scores:i[] = semspacequerytxt(s_handle, e_handle, "warm analog sound", 1)
    prints("after clear score = %.6f\n", scores[0])
    turnoff
endin

</CsInstruments>
<CsScore>
i "ADD"          0 0.1
i "QUERY_BEFORE" 1 0.1
i "CLEAR"        2 0.1
i "QUERY_AFTER"  3 0.1
</CsScore>
</CsoundSynthesizer>
