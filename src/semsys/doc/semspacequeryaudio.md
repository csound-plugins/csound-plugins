# semspacequeryaudio

## Abstract

Find the nearest stored vectors to a query audio file (k-nearest neighbours).

## Description

`semspacequeryaudio` decodes a **PCM16 WAV** file, embeds it with the **audio model** passed
as `handle`, and compares the result against every vector in the space (opened with
[semspace](semspace.md)) by **cosine similarity**, returning the `top-k` closest entries.

The model is chosen **per call**. The space holds no model, only vectors. `handle` must be
an **audio** model whose dim equals the space's dim.

## Syntax

```csound
neighs:i[][], scores:i[] = semspacequeryaudio(space:i, handle:i, path:S, topk:i)   ; i-rate
neighs:k[][], scores:k[], kgate:k = semspacequeryaudio(space:i, handle:i, path:S, topk:i)   ; k-rate
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `handle:i`: an **audio** embedding model from [semload](semload.md) (dim must match the space).
* `path:S`: path to a PCM16 WAV file to use as the query.
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
; semspacequeryaudio.csd
;
; semspacequeryaudio decodes a PCM16 WAV, embeds it with the audio model and
; returns the top-k nearest stored vectors by cosine similarity. Here: build an
; audio space from a folder of .wav, load it, then query it with a sound file.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define AUDIO_MODEL # "path/to/audio_model_dir" #
#define AUDIO_DIR # "path/to/wav_dir" #

a_handle@global:i = semload(-1, $AUDIO_MODEL)
; build an .espc from every .wav in a directory (audio model -> audio embedding)
semspacebuild(a_handle, "sounds.espc", $AUDIO_DIR)

; load it back into RAM (handle only fixes the dimension)
s_handle@global:i = semspace(a_handle, "sounds.espc")

instr QUERY
    ; 3 stored sounds most similar to a query sound (file split into ~10s windows,
    ; embedded, mean-pooled into one query vector, then top-k cosine search)
    neighs:i[][], scores:i[] = semspacequeryaudio(s_handle, a_handle, "query.wav", 3)
    printarray(scores)
    turnoff
endin

</CsInstruments>
<CsScore>
i "QUERY" 0 0.1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspace](semspace.md)
* [semspacequerytxt](semspacequerytxt.md)
* [semspacequeryaudioft](semspacequeryaudioft.md)
* [semspaceaddaudio](semspaceaddaudio.md)
* [semembedaudio](semembedaudio.md)

## Credits

Pasquale Mainolfi, 2026
