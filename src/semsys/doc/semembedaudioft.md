# semembedaudioft

## Abstract

Embed a function table of audio samples at init, returning one embedding per window.

## Description

`semembedaudioft` reads the samples of function table `ftable` (interpreted as mono audio
at the **engine sample rate**), resamples them to the model's rate (32 kHz for PANNs CNN14)
and runs them through an end-to-end audio embedding model, returning a **2D array
`[nchunks, ldim]`**. The audio is split into fixed **~10 s windows**; each is embedded and
**L2-normalized** into one **row per window**. A table shorter than one window yields a
single row. semsys selects the graph output named `embedding` (see [semdim](semdim.md)).
The model must be loaded with [semload](semload.md); models with global time pooling take
`maxlen = -1` ("full"). Embedding runs **once at init** (i-rate), off the audio thread.

## Syntax

```csound
chunks:i[][] = semembedaudioft(handle:i, ftable:i)
```

## Arguments

* `handle:i`: handle returned by [semload](semload.md) (an audio embedding model).
* `ftable:i`: function table number holding mono audio samples at the engine sample rate.

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
; semembedaudioft.csd
;
; semembedaudioft embeds the samples of a function table (mono, engine sr) at
; init, returning a 2D array [nchunks, ldim]: split into ~10 s windows, each
; embedded and L2-normalized into one row. Load an end-to-end audio model
; (e.g. PANNs CNN14) with maxlen = -1 ("full").
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define AUDIO_EMB_DIR # "path/to/audio_model_dir" #
#define AUDIO_TEST_FILE # "sound.wav" #

audio_table@global:i = ftgen(0, 0, 0, 1, $AUDIO_TEST_FILE, 0, 0, 0)

; audio embedder: -1 = run full, no sequence cap (PANNs CNN14 pools over time)
h_aemb@global:i = semload(-1, $AUDIO_EMB_DIR)

instr FTABLE
    emb:i[][] = semembedaudioft(h_aemb, audio_table)
    printarray(emb)
    turnoff
endin

</CsInstruments>
<CsScore>
i "FTABLE" 0 1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semload](semload.md)
* [semdim](semdim.md)
* [semembedaudiofile](semembedaudiofile.md)
* [semembedaudio](semembedaudio.md)

## Credits

Pasquale Mainolfi, 2026
