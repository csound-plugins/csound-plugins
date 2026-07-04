# semembedaudiofile

## Abstract

Embed an audio file at init, returning one audio embedding per window.

## Description

`semembedaudiofile` reads a **PCM16 WAV** file, decodes it to mono, resamples it to the
model's rate (32 kHz for PANNs CNN14) and runs it through an end-to-end audio embedding
model, returning a **2D array `[nchunks, ldim]`**. The audio is split into fixed **~10 s
windows** (PANNs' training length); each window is embedded and **L2-normalized** into one
**row per window**. A file shorter than one window yields a single row.
semsys selects the graph output named `embedding`, so a model that also emits `clip_scores`
still reports the embedding dimension (see [semdim](semdim.md)).
The model must be loaded with [semload](semload.md). Models with global time pooling impose
no sequence cap. Load them with `maxlen = -1` ("full"). Embedding runs **once at init**
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
-o dac
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semembedaudiofile.csd
;
; semembedaudiofile embeds a PCM16 WAV at init, returning a 2D array
; [nchunks, ldim]: the file is split into ~10 s windows, each embedded and
; L2-normalized into one row. Runs in the init pass, off the audio thread.
; Load an end-to-end audio model (e.g. PANNs CNN14) with maxlen = -1 ("full").
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define AUDIO_EMB_DIR # "path/to/audio_model_dir" #
#define AUDIO_TEST_FILE # "sound.wav" #

; audio embedder: -1 = run full, no sequence cap (PANNs CNN14 pools over time)
h_aemb@global:i = semload(-1, $AUDIO_EMB_DIR)

; PCM16 WAV only (convert mp3/flac/ogg first, or load into an ftable -> semembedaudioft)
instr FILE
    ldim:i = semdim(h_aemb)
    prints("audio latent dim: %d\n", ldim)

    emb:i[][] = semembedaudiofile(h_aemb, $AUDIO_TEST_FILE)
    prints("file embedded: %d window(s) x %d dims\n", lenarray(emb), ldim)
    printarray(emb)
    turnoff
endin

</CsInstruments>
<CsScore>
i "FILE" 0 1
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

Pasquale Mainolfi, 2026
