# semsys

**semsys** is an experimental *semantic synthesis* framework for Csound. It turns
text into embeddings — points in the latent space of a sentence-embedding model —
and lets you treat *meaning* as a control signal: compare it, search it, navigate
it, and drive synthesis, sound design, or algorithmic composition from natural
language.

Under the hood it runs an end-to-end ONNX sentence-embedding model (e.g.
`sentence-transformers/all-MiniLM-L6-v2`) through the ONNX Runtime C API. A piece of
text is tokenized, run through the encoder, and mean-pooled into a single fixed-size
vector (the *embedding*) — all inside one ONNX graph. Texts with similar meaning land
close together in that space, so cosine similarity becomes a measure of semantic
distance.

`semload` takes a **model directory** containing two files with fixed names —
`model.onnx` (the end-to-end graph) and `model.onnx.data` (its external weights). The
graph tokenizes internally with custom ops from **onnxruntime-extensions**, so its
native shared library (`libortextensions`) must be available at runtime — bundled next
to the plugin, or via the `SEMSYS_ORT_EXTENSIONS` env var. See
[doc/semload.md](doc/semload.md) for the expected tensor I/O, the export recipe, and how
to obtain/bundle the extensions library.

> Status: experimental / research. APIs and the on-disk cache format may change.

## Two layers: `semembed` vs `semspace`

semsys has two distinct sides. Keep them separate in your head.

### `semembed` — stateless, real-time embedding

`semembed` takes one text and produces its vectors *now*, at k-rate. Nothing is
stored. Use it for live latent-space control: map a phrase to a 384-d vector and
patch that into your synthesis (interpolate between phrases, modulate parameters
from semantic axes, etc.). It re-runs the model only when the input text changes.

The **i-rate** form embeds once at init and returns a **2D array `i[][]` of shape
`[nchunks, ldim]`**: text longer than the model window is split into `≤`-window token
chunks (same chunker as `semspacebuild`), one embedding row per chunk — short text
yields a single row. `semembedfile` is the same but reads the text from a file on disk.

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
  min-heap keeping the top-k. There is no ANN / approximate index. A query longer than
  the model window is chunked and its chunk embeddings are **mean-pooled** into one
  centroid vector, so the whole query is represented rather than truncated.
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
| `truncation.max_length` | tokenizer (baked into `model.onnx`) | Where the internal tokenizer cuts the input. (often **128** by default) |

Practical rules:

- Keep `maxlen` **≤ `max_position_embeddings`** (hard), ideally **≤ training
  `max_seq_length`** (quality).
- For `semspacebuild` to actually cover long documents, the model's **built-in
  tokenizer must not truncate below `maxlen`**. If the tokenizer baked into your
  `model.onnx` has `truncation.max_length = 128` but you pass `maxlen = 256`, every
  window is silently cut at 128 and the rest is lost. Re-export the end-to-end model
  with tokenizer truncation set to your `maxlen` (or disabled).
- Smaller chunks give sharper, more specific vectors; larger chunks dilute meaning
  through mean-pooling. For navigation/similarity, smaller is often better even
  when the model allows more.

## Opcodes

| Opcode | Form | Purpose |
|--------|------|---------|
| `semload` | `handle:i = semload(maxlen:i, modeldir:S)` | Load the end-to-end `model.onnx` (+ `model.onnx.data`) from a directory; returns a handle |
| `semdim` | `dim:i = semdim(handle:i)` | Embedding dimension of the loaded model |
| `semembed` | `pool:k[], changed:k = semembed(handle:i, text:S)` (k-rate); `chunks:i[][] = semembed(handle:i, text:S)` (i-rate, one row per chunk) | Embed text — real-time (gated) or once at init (long text auto-chunked) |
| `semembedfile` | `chunks:i[][] = semembedfile(handle:i, path:S)` | Embed a text file at init; one row per chunk (long text auto-chunked) |
| `semspace` | `space:i = semspace(handle:i)`; `… = semspace(handle:i, path:S)`; `… = semspace(handle:i, paths:S[])` | Create an in-memory space (empty, or load a `.espc` file / directory / array) |
| `semspacebuild` | `semspacebuild(handle:i, dest:S, source:S)` | Build a `.espc` from a text file or a directory of `.txt` |
| `semspaceadd` | `semspaceadd(space:i, sentence:S)` (once at init); `semspaceadd(space:i, sentence:S, trig:k)` (on rising edge) | Embed a sentence and append it (in RAM); long text added as one entry per chunk |
| `semspacequery` | `neighs:k[][], scores:k[] = semspacequery(space:i, query:S, topk:i)` (k-rate); `neighs:i[][], scores:i[] = semspacequery(space:i, query:S, topk:i)` (i-rate) | Top-k nearest neighbours — real-time (gated) or once at init (long query auto-chunked + mean-pooled) |
| `semspacesave` | `semspacesave(space:i, file:S)`; `semspacesave(space:i, file:S, trig:k)` | Write the in-memory space to a `.espc` (once, or on trigger edge) |

### Speech-to-text (experimental)

An **asynchronous** speech-to-text front-end: load an end-to-end ONNX model, submit audio,
poll for the transcription. Inference runs on a background worker thread started on demand,
so it never blocks the audio thread; jobs and results pass through a bounded FIFO queue.

| Opcode | Form | Purpose |
|--------|------|---------|
| `semsttload` | `handle:i = semsttload(model_dir:S, maxlen:i [, queue:i])` | Load an end-to-end STT model |
| `semsttsubmitfile` | `semsttsubmitfile(handle:i, path:S)` | Submit a PCM16 WAV file (long audio auto-segmented) |
| `semsttsubmitarray` | `semsttsubmitarray(handle:i, samples:i[])` | Submit a sample buffer (long audio auto-segmented) |
| `semsttsubmitft` | `semsttsubmitft(handle:i, ftable:i)` | Submit a function table of samples (long audio auto-segmented) |
| `semsttsubmitlive` | `semsttsubmitlive(handle:i, asig:a, maxdur:i [, trig:k])` | Capture live a-rate speech; `maxdur` is a preferred window length and the backend closes usable speech windows |
| `semsttready` | `ready:k = semsttready(handle:i)` | `1` when a transcription is ready |
| `semsttresult` | `text:S, length:k = semsttresult(handle:i)` | Read the next transcription + its length (FIFO) |

The model is **not tied to Whisper** — any end-to-end graph matching the I/O contract
(`audio_stream` bytes in → `str` text out, plus the generation scalars) works. The model
directory must hold `model.onnx` and `model.onnx.data` (external weights). `maxlen` comes
from the model's config, not chosen freely. The model transcribes a **single audio window
whose length is fixed by the model** (~30 s for Whisper); the offline opcodes
(`semsttsubmitfile`, `semsttsubmitarray`, `semsttsubmitft`) **split longer audio
automatically** into ≤30 s segments — cutting at the quietest point near each boundary —
transcribe each, and join the texts into one result. `semsttsubmitlive` instead uses a
built-in energy gate to close usable speech windows. See
[doc/semsttload.md](doc/semsttload.md) for the contract, limits and the Whisper export recipe.

See `doc/` for the per-opcode reference.

## Examples

The `examples/` directory holds working `.csd` files: usage examples for the opcodes
(`sem_embedding.csd`, `sem_space.csd`, `sem_space_build.csd`) plus two small
**semantic synthesis** demos that turn a query into sound:

- `sem_synthesis.csd` — builds a filter **impulse response** from the query embedding
  and convolves a source with it (`semspacequery` → IR → `ftconv`).
- `sem_synthesis_additive.csd` — **additive synthesis**: the embedding coordinates
  become the amplitudes of a bank of sine partials.

and two **speech-to-text** demos:

- `sem_stt_file.csd` — transcribe an audio file (`semsttsubmitfile` → `semsttready` →
  `semsttresult`).
- `sem_stt_live.csd` — **voice-controlled latent space**: speak into the mic, each usable
  speech window is transcribed and used as a query into a semantic space, and the match
  drives sound.

## Building

Configure and build from the **repository root** (`csound-plugins/`), not from this
directory — only the root `CMakeLists.txt` defines the project and pulls `semsys`
in as a sub-directory.

You must pass `-DAPIVERSION=7.0`. The default is `6.0`, which does not define
`CSOUNDAPI7`, and `semsys` fails to compile without it.

To **bundle** the onnxruntime-extensions library next to the plugin, point
the build at the **native** `libortextensions` (see [doc/semload.md](doc/semload.md)),
either explicitly with `-DORT_EXTENSIONS_LIB=/path/to/libortextensions.dylib`, or by a
root hint `-DORTEXTENSIONS_ROOT=/root` (or the `ORTEXTENSIONS_ROOT` env var), which is
searched with `find_library`. Without it the plugin still builds, but you must supply
`libortextensions` at runtime via the `SEMSYS_ORT_EXTENSIONS` env var or inside the
model directory.

### With an ONNX Runtime binary package

```sh
cmake -S . -B build -DAPIVERSION=7.0 -DONNXRUNTIME_ROOT=/path/to/onnxruntime \
      -DORT_EXTENSIONS_LIB=/path/to/libortextensions.dylib
cmake --build build --target semsys
cmake --install build
```

### With ONNX Runtime as a CMake config package (Homebrew, vcpkg, …)

```sh
cmake -S . -B build -DAPIVERSION=7.0 -DORT_EXTENSIONS_LIB=/path/to/libortextensions.dylib
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
- **Blocking `semstttranscribe` (offline).** A synchronous i-time variant that
  transcribes a file or function table and returns the text directly, for non-real-time /
  batch use where blocking is fine. (Long-audio segmentation is now handled automatically
  by the async offline opcodes.)
- **Configurable live STT gate.** `semsttsubmitlive` currently uses built-in constants for
  energy threshold, minimum speech duration, trailing silence, pre/post pad and hard cap.
  Expose these as optional arguments or a separate setup opcode once the defaults settle.
- **STT `generation_config.json`.** Read the model's generation defaults (e.g.
  `max_length`, special tokens) from the model directory instead of passing them.

## Version history

### 1.0.0

- Initial release.
- Opcodes: `semload`, `semdim`, `semembed`, `semembedfile`, `semspace`,
  `semspacebuild`, `semspaceadd`, `semspacequery`, `semspacesave`.
- ONNX Runtime C API backend; mean-pooled sentence embeddings.
- `semembed` i-rate returns a 2D `[nchunks, ldim]` array: text longer than the model
  window is split into `≤`-window token chunks, one embedding per chunk. `semembedfile`
  is the same for a text file on disk.
- `semspacequery` chunks a query longer than the model window and **mean-pools** the
  chunk embeddings into one centroid query vector (no truncation). `semembed` k-rate
  does the same (single mean-pooled vector) since a k-rate array can't change row count.
- `semspaceadd` chunks long text too, adding **one entry per chunk**; a consecutive
  self-gate skips re-embedding when the text is unchanged from the previous add.
- In-memory vector space with explicit persistence: `semspace` loads a `.espc`
  file, a directory of them, or an array into RAM; `semspaceadd` appends in memory;
  `semspacesave` writes a snapshot (i-time or k-rate trigger).
- `semspacebuild` builds a `.espc` from a text file or a directory of `.txt`
  (word-window chunking with overlap).
- Brute-force cosine nearest-neighbour search in RAM, top-k via min-heap.
- Added experimental asynchronous speech-to-text opcodes: `semsttload`,
  `semsttsubmitfile`, `semsttsubmitarray`, `semsttsubmitft`, `semsttsubmitlive`,
  `semsttready`, `semsttresult`.
- Added STT model documentation for end-to-end ONNX graphs, ONNX Runtime
  Extensions, queue sizing and result polling.
- `semsttsubmitlive` now accumulates a-rate audio into usable speech windows
  instead of blindly cutting fixed-size chunks; it waits for enough speech and a
  trailing silence boundary, with debug diagnostics via `SEMSYS_STT_DEBUG=1`.
- The offline STT opcodes (`semsttsubmitfile`, `semsttsubmitarray`, `semsttsubmitft`)
  auto-segment audio longer than the model window (~30 s) into ≤30 s chunks, snapping
  each cut to the quietest point near the boundary, and join the per-chunk texts into
  one result. `semsttsubmitfile` accepts PCM16 WAV.
- STT queues are bounded FIFO queues. If a submit queue is full the newest submit
  is dropped with a warning; result queue overflow drops the newest result with a
  warning.
- STT worker threads are started lazily and cleaned up when idle or when the STT
  handle is released.
- Hardened semantic-space embedding and query paths against non-finite vectors:
  failed or NaN embeddings are rejected, invalid `.espc` rows are skipped on load,
  and query outputs are cleared on invalid input instead of propagating NaN scores.
- Added STT examples: `sem_stt_file.csd` and `sem_stt_live.csd`.
- Requires Csound API 7.
