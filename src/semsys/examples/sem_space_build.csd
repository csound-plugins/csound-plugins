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

semspacebuild(e_handle, "corpus.espc", "corpus.txt")   // from a single text file
semspacebuild(e_handle, "all.espc", "path/to/texts")   // from every .txt in a directory

instr 2 // load a single .espc into RAM and query it
    s:i = semspace(e_handle, "corpus.espc")
    neighs:k[][], scores:k[] = semspacequery(s, "warm analog texture", 3)
endin

instr 3 // load a whole directory of .espc, merged into one space
    s:i = semspace(e_handle, "path/to/espc_dir")
    neighs:k[][], scores:k[] = semspacequery(s, "deep resonant drone", 5)
endin

instr 4 // load and merge an explicit array of .espc files
    paths:S[] = fillarray("corpus.espc", "all.espc")
    s:i = semspace(e_handle, paths)
    neighs:k[][], scores:k[] = semspacequery(s, "bright metallic hit", 3)
endin

</CsInstruments>
<CsScore>

i 1 0 1
i 2 1 1
i 3 2 1
i 4 3 1

</CsScore>
</CsoundSynthesizer>
