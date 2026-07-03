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
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

handle@global:i = semload(-1, "path/to/audio_model_dir")

instr 1
    emb:i[][] = semembedaudioft(handle, 1)
    printarray(emb)
    turnoff
endin

</CsInstruments>
<CsScore>
f 1 0 0 1 "sound.wav" 0 0 1   ; load audio into an ftable (GEN01)
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semload](semload.md)
* [semdim](semdim.md)
* [semembedaudiofile](semembedaudiofile.md)
* [semembedaudio](semembedaudio.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
July 2026.
