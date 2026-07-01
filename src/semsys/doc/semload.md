# semload

## Abstract

Load an end-to-end ONNX embedding model from a directory; return a handle.

## Description

`semload` initialises a **semsys** context from a **model directory** and returns an
integer handle used by all other semsys opcodes (`semdim`, `semembed`, `semspace`, …).

The directory must contain two files, with these exact names:

* `model.onnx` — the end-to-end embedding graph;
* `model.onnx.data` — its external weights.

The model is **end-to-end**: tokenization, the transformer encoder and mean-pooling all
live **inside the ONNX graph**. It takes a raw text string and returns the pooled
sentence embedding directly — there is no separate `tokenizer.onnx` file. Both files are
required.

The embedding dimension is detected automatically from the model output and is
available through [semdim](semdim.md).

`maxlen` is the maximum token sequence length. It is **not** free: it must respect the
model's `max_position_embeddings` (hard ceiling) and ideally the training
`max_seq_length`. semsys does not auto-detect or validate it — see the project README,
section *Model and token limits*, before choosing a value.

The model is released automatically when the instance is deinitialised.

## Preparing the model directory

semsys expects a single end-to-end graph with these fixed tensor names:

* input `text` (string) → output `embedding` (`[batch, dim]` float).

Build it by merging three pieces into one graph: the onnxruntime-extensions tokenizer
(`text` → `input_ids`, `attention_mask`, `token_type_ids`), the encoder
(`sentence-transformers/all-MiniLM-L6-v2` exported with
`optimum.onnxruntime.ORTModelForFeatureExtraction`), and a mean-pooling tail
(`last_hidden_state` masked by `attention_mask` → `embedding`). The tokenizer emits
rank-1 tensors, so a leading batch axis must be added before the encoder. See
`semsys-models/all-MiniLM-L6-v2/to_e2e.py` in this repo for a working merge script.

Save with external data so the weights land in `model.onnx.data`:

```python
onnx.save_model(
    merged, 
    "model_dir/model.onnx",
    save_as_external_data=True, 
    all_tensors_to_one_file=True,
    location="model.onnx.data"
)
```

Put both `model.onnx` and `model.onnx.data` in the same directory and pass that
directory to `semload`.

## Tokenizer runtime dependency: onnxruntime-extensions

The end-to-end graph tokenizes internally with **custom ops** from
[onnxruntime-extensions](https://github.com/microsoft/onnxruntime-extensions) (e.g.
`BertTokenizer` in the `ai.onnx.contrib` domain). Plain ONNX Runtime does not know
them, so `semload` registers the extensions shared library on the embedding session.

You must provide the **native** `libortextensions` shared library (the pip/conda
package only ships a Python-bound build that cannot be loaded standalone). Build it
from source:

```sh
git clone --recursive https://github.com/microsoft/onnxruntime-extensions
cd onnxruntime-extensions
./build.sh -DOCOS_BUILD_SHARED_LIB=ON
find . -name "libortextensions.dylib"   # (.so on Linux, ortextensions.dll on Windows)
```

`semload` looks for it, in order:

1. the path in the `SEMSYS_ORT_EXTENSIONS` environment variable;
2. next to the plugin (`libortextensions.dylib` / `.so` / `ortextensions.dll`) — this
   is where the packaged build bundles it;
3. inside `model_dir`, next to the model files.

If you build semsys yourself, bundle it by configuring with
`-DORT_EXTENSIONS_LIB=/path/to/libortextensions.dylib`.

## Syntax

```csound
handle:i = semload(maxlen:i, model_dir:S)
```

## Arguments

* `maxlen:i`: maximum token sequence length (e.g. `256` for all-MiniLM-L6-v2).
* `model_dir:S`: path to a directory containing `model.onnx` and `model.onnx.data`.

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

; the directory must contain model.onnx and model.onnx.data
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

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
