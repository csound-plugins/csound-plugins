# semload

## Abstract

Load an ONNX sentence-embedding model (and optional tokenizer) and return a handle.

## Description

`semload` initialises a **semsys** context: it loads an ONNX sentence-embedding model through the ONNX Runtime C API and returns an integer handle used by all other semsys opcodes.

A tokenizer model is optional in `semload` itself, but it is **required** by every opcode that turns text into vectors (`semembed`, `semspacebuild`, `semspaceadd`, `semspacequery`). Without it those opcodes raise an error. Pass the tokenizer as the third argument when you intend to embed text.

The embedding dimension is detected automatically from the model output and is available through [semdim](semdim.md).

The model is released automatically when the instance is deinitialised.

## Syntax

```csound
handle:i = semload(maxlen:i, model:S [, tokenizer:S])
```

## Arguments

* `maxlen:i`: maximum token sequence length (e.g. `256` for all-MiniLM-L6-v2).
* `model:S`: path to the ONNX embedding model (`.onnx`).
* `tokenizer:S`: path to the ONNX tokenizer model (`.onnx`). Optional, but required by any opcode that embeds text.

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

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

; load embedding + tokenizer model (.onnx)
handle@global:i = semload(256, "path/to/embedding_model.onnx", "path/to/tokenizer_model.onnx")

instr 1
    ldim:i = semdim(handle)
    prints("Latent dimension size: %d\n", ldim)
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semdim](semdim.md)
* [semembed](semembed.md)
* [semspace](semspace.md)

## Credits

Pasquale Mainolfi, 2026
