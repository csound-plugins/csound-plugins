# semload

## Abstract

Load an ONNX embedding model and its tokenizer from a directory; return a handle.

## Description

`semload` initialises a **semsys** context from a **model directory** and returns an
integer handle used by all other semsys opcodes (`semdim`, `semembed`, `semspace`, …).

The directory must contain two files, with these exact names:

* `model.onnx` — the sentence-embedding model;
* `tokenizer.onnx` — its tokenizer.

The tokenizer is **always** loaded together with the model: it is tied to the model
(same vocabulary, special tokens, truncation), so keeping the pair together avoids
mismatches. Both files are required.

The embedding dimension is detected automatically from the model output and is
available through [semdim](semdim.md).

`maxlen` is the maximum token sequence length. It is **not** free: it must respect the
model's `max_position_embeddings` (hard ceiling) and ideally the training
`max_seq_length`. semsys does not auto-detect or validate it — see the project README,
section *Model and token limits*, before choosing a value. For chunking in
[semspacebuild](semspacebuild.md) the tokenizer must not truncate below `maxlen`.

The model is released automatically when the instance is deinitialised.

## Preparing the model directory

semsys expects fixed tensor names in the two ONNX graphs:

* `tokenizer.onnx`: input `text` (string) → outputs `input_ids`, `attention_mask`.
* `model.onnx`: inputs `input_ids`, `attention_mask` → output `last_hidden_state`.

Export them with Python (example for `sentence-transformers/all-MiniLM-L6-v2`):

```python
# 1) the embedding model -> model.onnx
from optimum.onnxruntime import ORTModelForFeatureExtraction
m = ORTModelForFeatureExtraction.from_pretrained(
        "sentence-transformers/all-MiniLM-L6-v2", export=True)
m.save_pretrained("model_dir")          # writes model_dir/model.onnx

# 2) the tokenizer -> tokenizer.onnx (input "text" -> input_ids, attention_mask)
import onnx
from transformers import AutoTokenizer
from onnxruntime_extensions import gen_processing_models

tok = AutoTokenizer.from_pretrained("sentence-transformers/all-MiniLM-L6-v2")
pre, _ = gen_processing_models(tok, pre_kwargs={})
onnx.save(pre, "model_dir/tokenizer.onnx")
```

Put both `model.onnx` and `tokenizer.onnx` in the same directory and pass that
directory to `semload`. To embed long documents losslessly, export the tokenizer with
truncation disabled (or set to your `maxlen`).

## Syntax

```csound
handle:i = semload(maxlen:i, model_dir:S)
```

## Arguments

* `maxlen:i`: maximum token sequence length (e.g. `256` for all-MiniLM-L6-v2).
* `model_dir:S`: path to a directory containing `model.onnx` and `tokenizer.onnx`.

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

; the directory must contain model.onnx and tokenizer.onnx
handle@global:i = semload(256, "path/to/model_dir")

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
