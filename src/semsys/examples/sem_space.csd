<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

// load embedding and tokenizer model (.onnx)
e_handle@global:i = semload(256, "path/to/model_dir")

// empty in-memory space (RAM-only). Use semspace(e_handle, "space.espc") to load an existing one.
s_handle@global:i = semspace(e_handle)

instr 1 // add one line per k-cycle from a text file (semspaceadd embeds internally)
    text:S, line:k = readf("space_text.txt")
    printf("%s\n", line, text)

    if (line == -1) then
        turnoff
    endif

    semspaceadd(s_handle, text) // add to latent space (self-gated, consider token truncation config)
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
