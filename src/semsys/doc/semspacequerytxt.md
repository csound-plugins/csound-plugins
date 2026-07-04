# semspacequerytxt

## Abstract

Find the nearest stored vectors to a query text (k-nearest neighbours).

## Description

`semspacequerytxt` embeds `query` with the **text model** passed as `handle`, normalizes it,
and compares it against every vector in the space (opened with [semspace](semspace.md)) by
**cosine similarity**. It returns the `top-k` closest entries, sorted from most to least
similar.
The model is chosen **per call**. The space holds no model, only vectors. `handle` must be
a **text** model whose dim equals the space's dim. To query the same space with audio, use
[semspacequeryaudio](semspacequeryaudio.md).

A query longer than the model window is not truncated: it is split into `<=` window token
chunks, each chunk is embedded, and the chunk embeddings are **mean-pooled** into one
centroid query vector, so the whole query text contributes. (Short queries embed to a
single vector.)

## Syntax

```csound
neighs:i[][], scores:i[] = semspacequerytxt(space:i, handle:i, query:S, topk:i)   ; i-rate
neighs:k[][], scores:k[], kgate:k = semspacequerytxt(space:i, handle:i, query:S, topk:i)   ; k-rate
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `handle:i`: a **text** embedding model from [semload](semload.md) (dim must match the space).
* `query:S`: the query text.
* `topk:i`: number of output neighbour slots to return. Must be greater than `0`.

## Output

* `neighs[][]`: nearest vectors (`topk × dim`).
* `scores[]`: their cosine similarity scores (length `topk`, descending). If the space
  contains fewer than `topk` matches, the remaining rows/scores stay zero.
* `kgate:k`: k-rate form only. `1` for the k-pass where a fresh async result is ready,
  otherwise `0`.

## Execution Time

* Init
* Init + Performance

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semspacequerytxt.csd
;
; semspacequerytxt finds the nearest stored vectors to a query text (cosine top-k).
; This example turns the match into SOUND: a text query -> nearest embedding ->
; an additive sine bank, where each embedding coordinate is the amplitude of a
; partial at a fixed center frequency. The query's MEANING shapes the timbre.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 2
nchnls = 2
0dbfs = 1

#define EPS # pow(10, -12) #
#define MODEL_DIR # "path/to/text_model_dir" #

seed(0)

// center frequencies for the partial bank (constant -> i-array)
opcode get_center_freqs(minf:i, maxf:i, nfreqs:i):(i[])
    f:i[] = init(nfreqs)
    step:i = (maxf - minf) / nfreqs
    idx:i = 0
    while (idx < nfreqs) do
        f[idx] = minf + step * idx
        idx += 1
    od
    xout(f)
endop

// random partial mask -> sparse spectra avoid beating
opcode get_partials_mask(npart:i, dim:i):(i[])
    mask:i[] = init(npart)
    i:i = 0
    while (i < npart) do
        ndx:i = random(0, dim - 1)
        mask[i] = ndx
        i += 1
    od
    xout(mask)
endop

opcode osc_bank(amp:k[], freq:k[], dim:i, cnt:o):(a)
    sig:a = poscil(amp[cnt], freq[cnt]) * 0.81
    if (cnt < (dim - 1)) then
        rsig:a = osc_bank(amp, freq, dim, cnt + 1)
        xout(sig + rsig)
    else
        xout(sig)
    endif
endop

// load model dir (must contain model.onnx; keep model.onnx.data there too if present)
e_handle@global:i = semload(256, $MODEL_DIR)
sdim@global:i = semdim(e_handle)

// build a .espc space from a corpus once, then load it
semspacebuild(e_handle, "corpus.espc", "corpus.txt")
s@global:i = semspace(e_handle, "corpus.espc")

instr 1
    neighs:k[][], scores:k[], kgate:k = semspacequerytxt(s, e_handle, "warm analog texture", 3)
    cfreqs:i[] = get_center_freqs(90, 2500, sdim)

    npart:i = 30
    mask:i[] = get_partials_mask(npart, sdim)

    amp:k[] = init(npart)
    frq:k[] = init(npart)
    kidx:k = 0
    while (kidx < npart) do
        endx:k = mask[kidx]
        amp[kidx] = abs(neighs[0][endx])   // amplitude = |embedding coordinate|
        frq[kidx] = cfreqs[endx]
        kidx += 1
    od

    // normalize so the amplitudes sum to 1 -> the summed partials peak at <= 1
    asum:k = sumarray(amp)
    amp = amp / (asum + $EPS)

    add_sig:a = osc_bank(amp, frq, npart) * expseg(0.001, p3 / 2, 1, p3 / 2, 0.001)
    out(add_sig, add_sig)
endin

</CsInstruments>
<CsScore>
i 1 0 5
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspace](semspace.md)
* [semspacequeryaudio](semspacequeryaudio.md)
* [semspaceaddtxt](semspaceaddtxt.md)
* [semspacebuild](semspacebuild.md)
* [semembedtxt](semembedtxt.md)

## Credits

Pasquale Mainolfi, 2026
