<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semspacebuild.csd
;
; semspacebuild bulk-builds a .espc space from a source in one pass (a text model
; reads .txt, an audio model reads .wav; a file or a whole directory). It is an
; offline/init-time builder. Here: build from a text file, then load the result
; three ways -- single file, directory, explicit array -- and query each.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

// load the end-to-end embedding model (dir must contain model.onnx; keep model.onnx.data there too if present)
e_handle@global:i = semload(256, "path/to/model_dir")

semspacebuild(e_handle, "corpus.espc", "corpus.txt")   // from a single text file
// build from every .txt in a directory (set your own path to enable):
// semspacebuild(e_handle, "all.espc", "path/to/texts")

instr 1 // load a single .espc into RAM and query it
    s:i = semspace(e_handle, "corpus.espc")
    neighs:k[][], scores:k[], kgate:k = semspacequerytxt(s, e_handle, "warm analog texture", 3)
endin

// reference: other loading modes. set the placeholder paths and score them to try.
instr 2 // load a whole directory of .espc, merged into one space
    s:i = semspace(e_handle, "path/to/espc_dir")
    neighs:k[][], scores:k[], kgate:k = semspacequerytxt(s, e_handle, "deep resonant drone", 5)
endin

instr 3 // load and merge an explicit array of .espc files
    paths:S[] = fillarray("corpus.espc", "all.espc")
    s:i = semspace(e_handle, paths)
    neighs:k[][], scores:k[], kgate:k = semspacequerytxt(s, e_handle, "bright metallic hit", 3)
endin

</CsInstruments>
<CsScore>

i 1 0 1
; i 2 1 1   ; enable after setting path/to/espc_dir
; i 3 2 1   ; enable after building all.espc

</CsScore>
</CsoundSynthesizer>
