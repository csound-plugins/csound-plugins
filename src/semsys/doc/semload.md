# semload

## Abstract

Load an end-to-end ONNX embedding model from a directory; return a handle.

## Description

`semload` initialises a **semsys** context from a **model directory** and returns an
integer handle used by all other semsys opcodes (`semdim`, `semembedtxt`, `semembedaudio`, `semspace`, …).

The directory must contain the model graph, with this exact name:

* `model.onnx`, the end-to-end embedding graph.

If the graph uses ONNX external data, keep the external weights file
`model.onnx.data` in the same directory as `model.onnx`.

The model must be **end-to-end**: tokenization, the transformer encoder and mean-pooling all
live **inside the ONNX graph**. It takes a raw text string and returns the pooled
sentence embedding directly.
The embedding dimension is detected automatically from the model output and is
available through [semdim](semdim.md).

## Syntax

```csound
handle:i = semload(maxlen:i, model_dir:S)
```

## Arguments

* `maxlen:i`: maximum token sequence length (e.g. `256` for all-MiniLM-L6-v2). Use `-1` for models with no sequence cap (e.g. an audio embedder with global time pooling). It is **not** free: it must respect model's `max_position_embeddings` (hard ceiling) and ideally the training `max_seq_length`. semsys does not auto-detect or validate it. Pass **`maxlen = -1`** ("full", no cap) for models that impose no sequence limit (e.g. an audio embedding model with global time pooling such as PANNs CNN14, used by [semembedaudio](semembedaudio.md)).
* `model_dir:S`: path to a directory containing `model.onnx`. If `model.onnx.data`
  exists, it must be in the same directory.

## Output

* `handle:i`: handle to the loaded semsys context.

## Execution Time

* Init

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semload.csd
;
; semload initialises a semsys context from an end-to-end ONNX model directory
; (must contain model.onnx; keep model.onnx.data there too if present) and returns
; a handle used by every other semsys opcode.
;
;   text model  -> maxlen = token cap (e.g. 256 for all-MiniLM-L6-v2)
;   audio model -> maxlen = -1 ("full", no sequence cap; e.g. PANNs CNN14)
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define TEXT_DIR  # "path/to/text_model_dir" #
#define AUDIO_DIR # "path/to/audio_model_dir" #

h_txt@global:i = semload(256, $TEXT_DIR)   ; text model
h_aud@global:i = semload(-1, $AUDIO_DIR)   ; audio model, no sequence cap

instr 1
    prints("text dim = %d | audio dim = %d\n", semdim(h_txt), semdim(h_aud))
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semdim](semdim.md)
* [semembedtxt](semembedtxt.md)
* [semembedaudio](semembedaudio.md)
* [semspace](semspace.md)

## Credits

Pasquale Mainolfi, 2026
