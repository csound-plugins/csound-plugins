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

`semload` takes a **model directory** containing `model.onnx` (the end-to-end
graph). If the graph uses ONNX external data, `model.onnx.data` must be in the same
directory. The
graph tokenizes internally with custom ops from **onnxruntime-extensions**, so its
native shared library (`libortextensions`) must be available at runtime — bundled next
to the plugin, or via the `SEMSYS_ORT_EXTENSIONS` env var. See
[doc/semload.md](doc/semload.md) for the expected tensor I/O, the export recipe, and how
to obtain/bundle the extensions library.

> Status: experimental / research. APIs and the on-disk cache format may change.

## Two layers: `semembed*` vs `semspace`

semsys has two distinct sides. Keep them separate in your head.

### `semembedtxt` — stateless, real-time embedding

`semembedtxt` takes one text and produces its vectors *now*, at k-rate. Nothing is
stored. Use it for live latent-space control: map a phrase to a 384-d vector and
patch that into your synthesis (interpolate between phrases, modulate parameters
from semantic axes, etc.). It re-runs the model only when the input text changes.

The **i-rate** form embeds once at init and returns a **2D array `i[][]` of shape
`[nchunks, ldim]`**: text longer than the model window is split into `≤`-window token
chunks (same chunker as `semspacebuild`), one embedding row per chunk — short text
yields a single row. `semembedtxtfile` is the same but reads the text from a file on disk.

### `semembedaudio*` — audio embedding (no classification)

The same idea for **audio**: feed raw sound to an end-to-end ONNX audio model (e.g.
**PANNs CNN14**) and get a semantic vector back — the model's pooled embedding, not its
class scores. Load it with `semload`; if the model has global time pooling and imposes no
sequence cap (PANNs does), pass **`maxlen = -1`** ("full"). semsys resamples and downmixes
to the model's rate (32 kHz for PANNs) in C before inference, and picks the graph output
named `embedding` (so a model that also emits `clip_scores` still reports the right dim).

- **`semembedaudiofile`** / **`semembedaudioft`** (i-rate): embed a whole file (PCM16 WAV)
  or a function table at init, returning a **2D `[nchunks, ldim]`** array — the audio is
  split into ~10 s windows, one L2-normalized row per window. Runs in the init pass, off
  the audio thread.
- **`semembedaudio`** (a-rate): accumulate live `asig` into windows (`iwindow` seconds,
  default 10, min 1) and embed each on a **background worker thread** — like STT, inference
  never blocks the audio thread. Returns a 1D `[ldim]` vector plus a gate that pulses `1`
  on the pass a fresh embedding is published (embeddings arrive one window behind). Near-
  silent windows are skipped. Latest-wins: under overload the freshest window is kept
  rather than a growing backlog, so live control stays current.

Audio and text embeddings are **not comparable across models** — a PANNs vector says
nothing about a MiniLM text vector. Use audio embeddings for audio↔audio similarity; bridge
to text via STT, or use a joint audio-text model, for cross-modal search. (A `semspace` can
still *hold* both text and audio vectors when their dimensions match — it just compares them
blindly; see the space section.)

### `semspace` — an in-memory vector store

`semspace` is a *collection* of embeddings held **in RAM**. You build it (add items,
or bulk-build from a folder), query it for nearest neighbours, and persist it with
`semspacesave`. Use it for similarity search and corpus navigation.

The space is a **pure vector store: it does not embed.** The embedding model is passed
by *you* to each add/query call, so **text and audio vectors can live in one space** as
long as they share a dimension. The `handle` given to `semspace` only anchors the
dimension. This is why add/query come in `txt` and `audio` variants, while `semspacebuild`
is a single **universal** opcode that dispatches on the model kind (detected at `semload`).

Persistence is explicit and decoupled:

- `semspace(handle)` starts an empty space; `semspace(handle, path)` loads a `.espc`
  file **or** a whole directory of them into RAM; `semspace(handle, paths[])` merges
  an array of `.espc` files. Loaded files are read once, then closed. When files are
  merged, vectors already present in the space are skipped.
- `semspaceaddtxt(space, model, text)` / `semspaceaddaudio(space, model, wavfile)` append
  to the space **in RAM only** — no file is touched. Each brings its own model, and
  duplicate vectors are not reinserted.
- `semspacebuild(model, dest, source)` is a separate offline builder: it reads a source
  (`.txt` for a text model, `.wav` for an audio model) and writes a `.espc` to disk, ready
  to load with `semspace`.
- `semspacequerytxt(space, model, text, k)` / `semspacequeryaudio(space, model, wavfile, k)`
  return the top-k nearest vectors (a long text/audio query is chunked and **mean-pooled**
  into one centroid query). `semspacequeryaudioft(space, model, ftable, k [, ktrig][, iminsec])`
  is the **real-time** audio query: it reads a live capture **ftable** instead of a file and
  can fire on a trigger, with a minimum-duration gate. The k-rate query forms run
  inference/search on per-instance worker threads with latest-wins scheduling; their
  third output, `kgate`, pulses to 1 when a fresh result has been published.
- `semspacesave(space, file)` writes a full snapshot of the in-memory space to a
  fresh file (loaded files are never overwritten). A k-rate form saves on a trigger
  edge.

A `.espc` stores only dimension + vectors — no source text or labels, and no record of
which model produced each row — so text- and audio-built files with the same dimension
merge freely into one search space.

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
- **No text is stored.** The space holds vectors only. The query opcodes return the
  matching *vectors* and their scores — **not** the source text/audio. semsys is not a
  retrieval/RAG index; mapping a result back to its original item is out of
  scope (for now).
- **Approximate chunking.** For a text model, `semspacebuild` splits the source text into
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
| `semload` | `handle:i = semload(maxlen:i, modeldir:S)` | Load the end-to-end `model.onnx` from a directory; if `model.onnx.data` exists, keep it alongside the graph |
| `semdim` | `dim:i = semdim(handle:i)` | Embedding dimension of the loaded model |
| `semembedtxt` | `pool:k[], changed:k = semembedtxt:k(handle:i, text:S)` (k-rate); `chunks:i[][] = semembedtxt:i(handle:i, text:S)` (i-rate, one row per chunk) | Embed text — real-time (gated) or once at init (long text auto-chunked) |
| `semembedtxtfile` | `chunks:i[][] = semembedtxtfile(handle:i, path:S)` | Embed a text file at init; one row per chunk (long text auto-chunked) |
| `semembedaudiofile` | `chunks:i[][] = semembedaudiofile(handle:i, path:S)` | Embed an audio file (PCM16 WAV) at init; one row per ~10 s window |
| `semembedaudioft` | `chunks:i[][] = semembedaudioft(handle:i, ftable:i)` | Embed a function table of samples at init; one row per ~10 s window |
| `semembedaudio` | `emb:k[], gate:k = semembedaudio(handle:i, asig:a [, window:i])` | Embed live a-rate audio on a background worker; `gate` pulses when a fresh vector arrives |
| `semspace` | `space:i = semspace(handle:i)`; `… = semspace(handle:i, path:S)`; `… = semspace(handle:i, paths:S[])` | Create an in-memory vector store (empty, or load a `.espc` file / directory / array). `handle` only anchors the dimension |
| `semspaceclear` | `semspaceclear(space:i)`; `semspaceclear(space:i, trig:k)` | Clear all vectors from an in-memory space; k-rate form clears on a rising edge |
| `semspacebuild` | `semspacebuild(handle:i, dest:S, source:S)` | Universal builder: `.espc` from a text source (text model) or an audio source (audio model), dispatched on the model kind |
| `semspaceaddtxt` | `semspaceaddtxt(space:i, handle:i, sentence:S [, trig:k])` | Embed text with `handle` and append it (in RAM); long text added as one entry per chunk; duplicate vectors are skipped |
| `semspaceaddaudio` | `semspaceaddaudio(space:i, handle:i, path:S [, trig:k])` | Embed a PCM16 WAV with `handle` and append it (in RAM); one entry per ~10 s window; duplicate vectors are skipped |
| `semspacequerytxt` | `neighs:i[][], scores:i[] = semspacequerytxt(...)`; `neighs:k[][], scores:k[], kgate:k = semspacequerytxt(...)` | Top-k nearest to a text query (long query auto-chunked + mean-pooled); k-rate runs async latest-wins |
| `semspacequeryaudio` | `neighs:i[][], scores:i[] = semspacequeryaudio(...)`; `neighs:k[][], scores:k[], kgate:k = semspacequeryaudio(...)` | Top-k nearest to an audio query (file windows mean-pooled into one query); k-rate runs async latest-wins |
| `semspacequeryaudioft` | `neighs:i[][], scores:i[] = semspacequeryaudioft(...)`; `neighs:k[][], scores:k[], kgate:k = semspacequeryaudioft(...)` | Real-time audio query from an ftable; k-rate snapshots the table on trigger and runs async latest-wins |
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
directory must hold `model.onnx`; if `model.onnx.data` exists, it must be alongside it. `maxlen` comes
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
(`sem_embedding.csd`, `sem_embed_audio.csd`, `sem_space.csd`, `sem_space_build.csd`,
`sem_space_audio.csd`) plus two small **semantic synthesis** demos that turn a query into
sound:

- `sem_synthesis.csd` — builds a filter **impulse response** from the query embedding
  and convolves a source with it (`semspacequerytxt` → IR → `ftconv`).
- `sem_synthesis_additive.csd` — **additive synthesis**: the embedding coordinates
  become the amplitudes of a bank of sine partials.

two **audio** demos:

- `sem_embed_audio.csd` — embed a file into per-window vectors (`semembedaudiofile`,
  `semembedaudioft`) and stream live a-rate audio into embeddings on a worker thread
  (`semembedaudio`, gated).
- `sem_space_audio.csd` — an **audio semantic space**: build a `.espc` from a folder of
  `.wav`, then retrieve the nearest sounds to a query sound (`semspacebuild` →
  `semspaceaddaudio` → `semspacequeryaudio`).

and two **speech-to-text** demos:

- `sem_stt_file.csd` — transcribe an audio file (`semsttsubmitfile` → `semsttready` →
  `semsttresult`).
- `sem_stt_live.csd` — **voice-controlled latent space**: speak into the mic, each usable
  speech window is transcribed and used as a query into a semantic space, and the match
  drives sound.

## Tests

The smoke test score is in `utest/utest.csd`, with its fixtures under `utest/utest_data`.
It exercises the semsys opcodes and prints `[PASS]`, `[FAIL]`, `[SKIP]`, or `[INFO]`
messages for each check.

## Building

### From the release

The prebuilt package in the [releases](https://github.com/csound-plugins/csound-plugins/releases)
includes the `semsys` plugin, but **not** its two ONNX runtime libraries. To run it you
supply, yourself:

- `libonnxruntime` and `libortextensions` — get them as described under **Manual build**
  below, then either drop them next to the plugin or point `SEMSYS_ONNXRUNTIME` /
  `SEMSYS_ORT_EXTENSIONS` at them;
- the ONNX **models** (see [`semload`](doc/semload.md)).

### Manual build

Build from the **repository root** (`csound-plugins/`), not this folder.

semsys loads ONNX Runtime dynamically, so it **compiles with no ONNX SDK installed** (the
headers are vendored). You only need two shared libraries at **runtime**:

- **`libonnxruntime`** — download the official [ONNX Runtime](https://github.com/microsoft/onnxruntime/releases) build (auto-found if installed via Homebrew/apt/vcpkg).
- **`libortextensions`** — from the NuGet package `Microsoft.ML.OnnxRuntime.Extensions`
  (a `.nupkg` is a zip; take the lib under `runtimes/<os>/native/`). **Not** the PyPI
  wheel — that one needs Python and won't load standalone.

**Option A — bundle the extensions lib into the build** (self-contained plugin):

```sh
cmake -S . -B build -DAPIVERSION=7.0 -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
      -DORT_EXTENSIONS_LIB=/path/to/libortextensions.dylib
cmake --build build --target semsys --parallel
```

**Option B — don't bundle, give it at runtime:**

```sh
cmake -S . -B build -DAPIVERSION=7.0 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build --target semsys --parallel

# then, before running Csound, point the plugin at whatever was NOT bundled:
export SEMSYS_ORT_EXTENSIONS=/path/to/libortextensions.dylib
# onnxruntime is auto-bundled from a system install (check the configure log). Only if it
# said "semsys: onnxruntime ... not bundled" do you also need:
# export SEMSYS_ONNXRUNTIME=/path/to/libonnxruntime.dylib
```

So: with a system ONNX Runtime (e.g. Homebrew) you typically export **only**
`SEMSYS_ORT_EXTENSIONS`; on a clean machine where neither was bundled, export **both**.

Notes:
- `-DAPIVERSION=7.0` is **required** (semsys is Csound API 7 only).
- `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` is only needed with **CMake ≥ 4** (an old sibling
  plugin aborts the configure otherwise); drop it on CMake 3.x.
- `libonnxruntime` is auto-bundled from a system install; override with `-DONNXRUNTIME_LIB=/path`.

### Runtime — where the libraries are looked up

At `semload` time the plugin resolves each library in order:

- **onnxruntime**: `SEMSYS_ONNXRUNTIME` env → next to the plugin → the system loader's default paths.
- **onnxruntime-extensions**: `SEMSYS_ORT_EXTENSIONS` env → next to the plugin → next to the model directory.

> NOTE: The system-loader step for **onnxruntime** covers only standard paths (`/usr/lib`,
> `DYLD_*` / `LD_LIBRARY_PATH`). **Homebrew's `/opt/homebrew/lib` is *not* among them** —
> so a Homebrew ONNX Runtime is auto-found at **build** time (`find_file`, then bundled
> next to the plugin) but is **not** found at runtime by name. Therefore:
>
> - **bundled** (sitting next to the plugin) → found automatically
> - **not bundled** → set `SEMSYS_ONNXRUNTIME=/path/to/libonnxruntime.dylib`, or drop the
>   library next to the plugin / model directory.

## Roadmap

- **Faster search for large spaces.** The scan is now in RAM with a bounded
  min-heap for top-k. Still open: vectorize the dot products (SIMD / blocked) and,
  for large spaces, an approximate-nearest-neighbour index.
- **Source-text storage.** Optionally keep each vector's originating text/path so the
  query opcodes can return it — turning the space into a real retrieval index.
- **Token-accurate chunking** in `semspacebuild` (current text chunking is
  word-based / approximate).
- **Multimodal embeddings.** Audio embeddings and a modality-agnostic space (text + audio
  in one `.espc`) are done. Still open: images, and a **joint text-audio model** so a single
  space can *cross-search* meaning across modalities (today the space compares vectors
  blindly — cross-modal only makes sense with aligned models).
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
- Opcodes: `semload`, `semdim`, `semembedtxt`, `semembedtxtfile`, `semembedaudiofile`,
  `semembedaudioft`, `semembedaudio`, `semspace`, `semspaceclear`, `semspacebuild`,
  `semspaceaddtxt`, `semspaceaddaudio`, `semspacequerytxt`, `semspacequeryaudio`,
  `semspacesave`.
- ONNX Runtime C API backend; mean-pooled sentence embeddings.
- `semembedtxt` i-rate returns a 2D `[nchunks, ldim]` array: text longer than the model
  window is split into `≤`-window token chunks, one embedding per chunk. `semembedtxtfile`
  is the same for a text file on disk.
- `semspacequerytxt` chunks a query longer than the model window and **mean-pools** the
  chunk embeddings into one centroid query vector (no truncation). `semembedtxt` k-rate
  does the same (single mean-pooled vector) since a k-rate array can't change row count.
- `semspaceaddtxt` chunks long text too, adding **one entry per chunk**; a consecutive
  self-gate skips re-embedding when the text is unchanged from the previous add, and
  duplicate vectors already present in the space are skipped.
- In-memory vector space with explicit persistence: `semspace` loads a `.espc`
  file, a directory of them, or an array into RAM; `semspaceaddtxt` / `semspaceaddaudio`
  append in memory while skipping vectors already present; `semspaceclear` empties
  the in-memory space while keeping its allocation; `semspacesave` writes a
  snapshot (i-time or k-rate trigger).
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
- Renamed the text embedding opcodes to `semembedtxt` / `semembedtxtfile` (was
  `semembed` / `semembedfile`) to make room for audio embedding.
- Added **audio embedding** opcodes for end-to-end audio models (e.g. PANNs CNN14):
  `semembedaudiofile` and `semembedaudioft` (i-rate, 2D `[nchunks, ldim]`, one row per
  ~10 s window) and `semembedaudio` (a-rate live). semsys resamples/downmixes to the
  model's 32 kHz in C and selects the graph output named `embedding` for `ldim`, so a
  model that also emits `clip_scores` reports the embedding dimension. `semload` accepts
  `maxlen = -1` for models with no sequence cap (global time pooling).
- `semembedaudio` runs inference on a per-instance background worker thread (like STT), so
  the audio thread never stalls; it embeds full, non-silent windows and publishes the
  freshest result (latest-wins, bounded latency), gating on new embeddings.
- `semspacequerytxt.k`, `semspacequeryaudio.k`, and `semspacequeryaudioft.k` run
  embedding/search on per-instance background workers. They are latest-wins: faster input
  changes/triggers replace pending work, stale worker results are discarded, and `kgate`
  pulses when the freshest response is published.
- Added audio embedding example: `sem_embed_audio.csd`.
- Made the semantic space **modality-agnostic**: the space is now a pure vector store that
  holds no model, and the embedding model is passed per operation. `semspaceadd` and
  `semspacequery` split into `…txt` (text) and `…audio` (PCM16 WAV, per-window, mean-pooled)
  forms, each taking a model handle; `semspacebuild` became a **single universal** opcode
  that dispatches text vs audio on the model kind (detected at `semload` from the input
  tensor type). Text- and audio-built `.espc` files with the same dimension merge into one
  space; add/query validate the model's dim and kind against the space.
- Added audio-space example: `sem_space_audio.csd`.
- Added `semspacequeryaudioft` — a real-time audio query that reads a live-capture **ftable**
  instead of a WAV file, with an optional trigger (`ktrig`) and a minimum-duration gate
  (`iminsec`) so short/partial buffers are skipped. The k-rate form snapshots the ftable
  on submit, so concurrent recording can overwrite the live table while the worker runs.
- Semantic spaces now skip duplicate vectors when loading/merging `.espc` files and when
  adding text/audio embeddings to an existing in-memory space.
- ONNX Runtime is now **loaded dynamically** (`dlopen` of `OrtGetApiBase`) instead of
  linked; its C API headers are **vendored** in `third_party/onnxruntime/`. The build
  needs no ONNX Runtime SDK — only the runtime shared libraries, bundled next to the
  plugin or supplied via `SEMSYS_ONNXRUNTIME` / `SEMSYS_ORT_EXTENSIONS`. Use the NuGet
  `Microsoft.ML.OnnxRuntime.Extensions` native lib (not the Python wheel) for
  onnxruntime-extensions.
- Requires Csound API 7.
