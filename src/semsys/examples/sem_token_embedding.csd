<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

// load embedding and tokenizer model (.onnx)
handle@global:i = semload(256, "path/to/model_dir")

instr 1
    ldim:i = semdim(handle) // get latent dimension
    prints("Latent dimension size: %d\n", ldim)
    turnoff
endin

instr 2
    sentence:S = "sound synthesis in blue sky"
    // pool = mean-pooled sentence vector, tokens = per-token matrix, changed = 1 on text change
    pool_embed:k[], tokens_embed:k[][], changed:k = semembed(handle, sentence)
endin

</CsInstruments>
<CsScore>

i 1 0 1
i 2 2 1

</CsScore>
</CsoundSynthesizer>
