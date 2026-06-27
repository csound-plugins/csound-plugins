# semsys

**semsys** is an experimental *semantic synthesis* framework for Csound. It turns
text into embeddings — points in the latent space of a sentence-embedding model —
and lets you treat *meaning* as a control signal: compare it, search it, navigate
it, and drive synthesis, sound design, or algorithmic composition from natural
language.

Under the hood it runs an ONNX sentence-embedding model (e.g.
`sentence-transformers/all-MiniLM-L6-v2`) plus its tokenizer through the ONNX
Runtime C API. A piece of text is tokenized, run through the model, and
mean-pooled into a single fixed-size vector (the *embedding*). Texts with similar
meaning land close together in that space, so cosine similarity becomes a measure
of semantic distance.

> Status: experimental / research. APIs and the on-disk cache format may change.

## Two layers: `semembed` vs `semspace`

semsys has two distinct sides. Keep them separate in your head.

### `semembed` — stateless, real-time embedding

`semembed` takes one text and produces its vectors *now*, at k-rate. Nothing is
stored. Use it for live latent-space control: map a phrase to a 384-d vector and
patch that into your synthesis (interpolate between phrases, modulate parameters
from semantic axes, etc.). It re-runs the model only when the input text changes.

### `semspace` — an in-memory vector store

`semspace` is a *collection* of embeddings held **in RAM**. You build it (add
sentences, or bulk-build from a text file), query it for nearest neighbours, and
persist it with `semspacesave`. Use it for similarity search and corpus navigation.

Persistence is explicit and decoupled:

- `semspace(handle)` starts an empty space; `semspace(handle, path)` loads a `.espc`
  file **or** a whole directory of them into RAM; `semspace(handle, paths[])` merges
  an array of `.espc` files. Loaded files are read once, then closed.
- `semspaceadd` appends to the space **in RAM only** — no file is touched.
- `semspacebuild` is a separate offline builder: it reads text (a file, or a
  directory of `.txt`) and writes a `.espc` file to disk, ready to be loaded with
  `semspace`.
- `semspacesave(space, file)` writes a full snapshot of the in-memory space to a
  fresh file (loaded files are never overwritten). A k-rate form saves on a trigger
  edge.

### Building large search spaces

`semspacebuild` can ingest a whole **directory of `.txt` documents** in one pass —
drop many files in a folder, point `semspacebuild` at it, and every document is
chunked and embedded into a single `.espc`. Load it back with `semspace` and query
it with natural language.

This is, in effect, the **retrieval stage of a RAG pipeline — without the
generation**: nearest-neighbour search over document chunks, driven by meaning
rather than keywords. By building from a corpus you can assemble **very large
semantic search spaces** (many thousands of chunks) and navigate them from Csound.
What you get back today is the matching vectors and similarity scores (not the
source text — see the roadmap on source-text storage), which is exactly what you
need to drive synthesis and selection from semantic proximity.

### `semspace` does **not** index

This is a deliberate limitation, not a TODO:

- **Brute-force search.** A query is embedded, normalized, then compared against
  *every* stored vector by cosine similarity — a linear scan in RAM, with a bounded
  min-heap keeping the top-k. There is no ANN / approximate index.
- **No text is stored.** The space holds vectors only. `semspacequery` returns the
  matching *vectors* and their scores — **not** the source text. semsys is not a
  retrieval/RAG index; mapping a result back to its original sentence is out of
  scope (for now).
- **Approximate chunking.** `semspacebuild` splits the source text into
  overlapping word-windows (~`maxlen`-sized, ~15% overlap), one vector per window.
  Boundaries are approximate (word-based, not token-exact). Blank lines are hard
  boundaries; a window never crosses one.

## Model and token limits — read before choosing `maxlen`

`maxlen` (the sequence length you pass to `semload`) is **not** free. It is bound
by your model and tokenizer, and semsys does **not** auto-detect or validate it —
it is your responsibility to pass a sane value.

Three separate caps interact:

| Limit | Where it lives | Meaning |
|-------|----------------|---------|
| `max_position_embeddings` | model `config.json` | Hard architectural ceiling. Exceeding it produces garbage or errors. (MiniLM-L6-v2: **512**) |
| `max_seq_length` (training) | sentence-transformers config | Context the model was trained on. Beyond it, embeddings degrade. (MiniLM-L6-v2: **256**) |
| `truncation.max_length` | tokenizer config | Where the tokenizer cuts the input. (often **128** by default) |

Practical rules:

- Keep `maxlen` **≤ `max_position_embeddings`** (hard), ideally **≤ training
  `max_seq_length`** (quality).
- For `semspacebuild` to actually cover long documents, the **tokenizer must
  not truncate below `maxlen`**. If your exported tokenizer has
  `truncation.max_length = 128` but you pass `maxlen = 256`, every window is
  silently cut at 128 and the rest is lost. Re-export the tokenizer with
  truncation set to your `maxlen` (or disabled).
- Smaller chunks give sharper, more specific vectors; larger chunks dilute meaning
  through mean-pooling. For navigation/similarity, smaller is often better even
  when the model allows more.

## Opcodes

| Opcode | Form | Purpose |
|--------|------|---------|
| `semload` | `ihandle semload imaxlen, Smodel[, Stokenizer]` | Load model (+ optional tokenizer); returns a handle |
| `semdim`  | `idim semdim ihandle` | Embedding dimension of the loaded model |
| `semembed` | `kpool[], ktokens[][], kchanged semembed ihandle, Stext` | Embed text in real time |
| `semspace` | `ispace semspace ihandle[, Spath | Spaths[]]` | Create an in-memory space (optionally load a `.espc` file, directory, or array) |
| `semspacebuild` | `semspacebuild ihandle, Sdest, Ssource` | Build a `.espc` file from a text file or a directory of `.txt` |
| `semspaceadd` | `semspaceadd ispace, Ssentence` | Embed a sentence and append it (in RAM) |
| `semspacequery` | `kneighs[][], kscores[] semspacequery ispace, Squery, itopk` | Top-k nearest neighbours |
| `semspacesave` | `semspacesave ispace, Sfile[, ktrig]` | Write the in-memory space to a `.espc` file |

See `doc/` for per-opcode reference and `examples/` for working `.csd` files.

## Building

Configure and build from the **repository root** (`csound-plugins/`), not from this
directory — only the root `CMakeLists.txt` defines the project and pulls `semsys`
in as a sub-directory.

You must pass `-DAPIVERSION=7.0`. The default is `6.0`, which does not define
`CSOUNDAPI7`, and `semsys` fails to compile without it.

### With an ONNX Runtime binary package

```sh
cmake -S . -B build -DAPIVERSION=7.0 -DONNXRUNTIME_ROOT=/path/to/onnxruntime
cmake --build build --target semsys
cmake --install build
```

### With ONNX Runtime as a CMake config package (Homebrew, vcpkg, …)

```sh
cmake -S . -B build -DAPIVERSION=7.0
cmake --build build --target semsys
cmake --install build
```

## ONNX Runtime dependency

How the dependency is resolved (see `src/semsys/CMakeLists.txt`):

- If `ONNXRUNTIME_ROOT` is set (cache var or environment), the build uses the
  `MODULE` finder `Findonnxruntime.cmake`.
- Otherwise it tries `find_package(onnxruntime CONFIG)`, then falls back to a
  `REQUIRED` lookup.

When using `ONNXRUNTIME_ROOT`, point it at an extracted/installed ONNX Runtime
C/C++ binary package. The root directory must contain:

```text
include/onnxruntime_c_api.h
lib/libonnxruntime.dylib  # macOS
lib/libonnxruntime.so     # Linux
lib/onnxruntime.lib       # Windows import library
bin/onnxruntime.dll       # Windows runtime library
```

During install, the ONNX Runtime shared library is installed next to the Csound
plugin so the loader can resolve it from the plugin directory.

## Roadmap

- **Faster search for large spaces.** The scan is now in RAM with a bounded
  min-heap for top-k. Still open: vectorize the dot products (SIMD / blocked) and,
  for large spaces, an approximate-nearest-neighbour index.
- **Source-text storage.** Optionally keep each vector's originating text so
  `semspacequery` can return it — turning the space into a real retrieval index.
- **Token-accurate chunking** in `semspacebuild` (current chunking is
  word-based / approximate).
- **Multimodal embeddings.** Embed audio and images, not just text, and extend the
  search space across modalities — so a single space can hold and cross-search
  sound, image, and text together.

## Version history

### 1.0.0

- Initial release.
- Opcodes: `semload`, `semdim`, `semembed`, `semspace`, `semspacebuild`,
  `semspaceadd`, `semspacequery`, `semspacesave`.
- ONNX Runtime C API backend; mean-pooled sentence embeddings.
- In-memory vector space with explicit persistence: `semspace` loads a `.espc`
  file, a directory of them, or an array into RAM; `semspaceadd` appends in memory;
  `semspacesave` writes a snapshot (i-time or k-rate trigger).
- `semspacebuild` builds a `.espc` from a text file or a directory of `.txt`
  (word-window chunking with overlap).
- Brute-force cosine nearest-neighbour search in RAM, top-k via min-heap.
- Requires Csound API 7.
