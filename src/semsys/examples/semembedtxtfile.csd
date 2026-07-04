<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semembedtxtfile.csd
;
; semembedtxtfile reads a whole text file and embeds it once at init, returning a
; 2D array [nchunks, ldim]: text longer than the model window is split into token
; chunks, one mean-pooled embedding row per chunk. It is the file counterpart of
; the i-rate semembedtxt.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define MODEL_DIR # "/Users/pm/AcaHub/Coding/csound-repo/semsys-models/all-MiniLM-L6-v2/all-MiniLM-L6-v2-e2e" #

handle@global:i = semload(256, $MODEL_DIR)

instr 1
    emb:i[][] = semembedtxtfile(handle, "corpus.txt")
    prints("chunks = %d | dim = %d\n", lenarray(emb), lenarray(emb, 2))
    printarray(emb)
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
