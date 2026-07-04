<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semspacesave.csd
;
; semspacesave writes a full snapshot of the in-memory space to a .espc file
; (overwrites). Here: fill the space during instr "build", then persist it from
; instr "save" scheduled AFTER it, so the i-time save sees every add.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

#define MODEL_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/all-MiniLM-L6-v2/all-MiniLM-L6-v2-e2e" #

e_handle@global:i = semload(256, $MODEL_DIR)
s_handle@global:i = semspace(e_handle)          ; RAM-only space

instr build
    semspaceaddtxt(s_handle, e_handle, "deep blue ocean under a calm evening sky")
    semspaceaddtxt(s_handle, e_handle, "rockets roar as they climb into orbit")
    semspaceaddtxt(s_handle, e_handle, "a lone violin sings a slow melancholic tune")
    turnoff
endin

instr save
    semspacesave(s_handle, "space.espc")
    prints("space saved to space.espc\n")
    turnoff
endin

</CsInstruments>
<CsScore>
i "build" 0 0.1
i "save"  1 0.1
</CsScore>
</CsoundSynthesizer>
