# semembedaudiofile

## Abstract

Embed an audio file at init, returning one audio embedding per window.

## Description

`semembedaudiofile` reads a **PCM16 WAV** file, decodes it to mono, resamples it to the
model's rate (32 kHz for PANNs CNN14) and runs it through an end-to-end audio embedding
model, returning a **2D array `[nchunks, ldim]`**. The audio is split into fixed **~10 s
windows** (PANNs' training length); each window is embedded and **L2-normalized** into one
**row per window**. A file shorter than one window yields a single row.

Unlike an audio *tagger*, this returns the model's pooled **embedding** (a semantic feature
vector), not class scores — for audio↔audio similarity, retrieval, or driving synthesis.
semsys selects the graph output named `embedding`, so a model that also emits `clip_scores`
still reports the embedding dimension (see [semdim](semdim.md)).

The model must be loaded with [semload](semload.md). Models with global time pooling impose
no sequence cap — load them with `maxlen = -1` ("full"). Embedding runs **once at init**
(i-rate), off the audio thread, so blocking there is harmless.

Only **PCM16 WAV** is accepted. For mp3/flac/ogg, convert to WAV first, or load the audio
into a function table and use [semembedaudioft](semembedaudioft.md). For live a-rate audio,
use [semembedaudio](semembedaudio.md).

## Syntax

```csound
chunks:i[][] = semembedaudiofile(handle:i, path:S)
```

## Arguments

* `handle:i`: handle returned by [semload](semload.md) (an audio embedding model).
* `path:S`: path to a PCM16 WAV file.

## Output

* `chunks:i[][]`: `[nchunks, ldim]` — one L2-normalized embedding per ~10 s window.

## Execution Time

* Init

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; audio embedder: -1 = run full, no sequence cap (e.g. PANNs CNN14)
handle@global:i = semload(-1, "path/to/audio_model_dir")

instr 1
    emb:i[][] = semembedaudiofile(handle, "sound.wav")
    prints("windows=%d  dim=%d\n", lenarray(emb), semdim(handle))
    printarray(emb)
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semload](semload.md)
* [semdim](semdim.md)
* [semembedaudioft](semembedaudioft.md)
* [semembedaudio](semembedaudio.md)
* [semembedtxt](semembedtxt.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
July 2026.
