<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

// load the end-to-end embedding model (dir must contain model.onnx + model.onnx.data)
e_handle@global:i = semload(256, "path/to/model_dir")

// empty in-memory space (RAM-only). Use semspace(e_handle, "space.espc") to load an existing one.
s_handle@global:i = semspace(e_handle)

instr 1 // add sentences to the space (i-rate form: each call embeds + appends once at init)
    semspaceadd(s_handle, "deep blue ocean under a calm evening sky")
    semspaceadd(s_handle, "a red sunset burns over the marine horizon")
    semspaceadd(s_handle, "rockets roar as they climb into orbit")
    semspaceadd(s_handle, "green forests breathe in the cold morning mist")
    semspaceadd(s_handle, "a lone violin sings a slow melancholic tune")
    turnoff
endin

instr 2 // persist the in-memory space to disk (scheduled after instr 1, so it sees all adds)
    semspacesave(s_handle, "space.espc")
    turnoff
endin

instr 3 // query the space
    top_k:i = 3
    query:S = "blue marine in red sky"
    neighs:k[][], scores:k[] = semspacequery(s_handle, query, top_k)
endin

</CsInstruments>
<CsScore>

i 1 0 4
i 2 4 0.1
i 3 5 1

</CsScore>
</CsoundSynthesizer>
