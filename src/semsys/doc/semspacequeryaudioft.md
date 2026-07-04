# semspacequeryaudioft

## Abstract

Query a semantic space with audio from a **function table**, for real-time audio queries that don't come from a file.

## Description

`semspacequeryaudioft` is the real-time counterpart of [semspacequeryaudio](semspacequeryaudio.md): instead of decoding a WAV file, it embeds the audio currently held in function table `ftable` (mono samples at the **engine sample rate**) with the **audio model** `handle`, mean-pools the per-window vectors into one centroid query, and returns the `top-k` nearest stored vectors by cosine similarity.

Typical use: continuously capture live input into a **circular table**, then query the space
on a trigger. Because the model is passed per call and the space is a pure store, the same
space can also be queried by text or by file.

`iminsec` sets the **minimum audio duration** (in seconds) required to run a query: if the
table holds less than `iminsec` of audio the call is a no-op that leaves the outputs zeroed
(it does **not** error), so a live loop reaching for a partly-filled buffer stays safe. A
near-silent table likewise yields no match. `iminsec = 0` (or omitted) means no minimum.

## Syntax

```csound
neighs:i[][], scores:i[] = semspacequeryaudioft(space:i, handle:i, ftable:i, topk:i [, iminsec:i])
neighs:k[][], scores:k[], kgate:k = semspacequeryaudioft(space:i, handle:i, ftable:i, topk:i, ktrig:k [, iminsec:i])
```

## Arguments

* `space:i`: handle returned by [semspace](semspace.md).
* `handle:i`: an **audio** embedding model from [semload](semload.md) (dim must match the space).
* `ftable:i`: a function table of mono audio samples at the engine sample rate.
* `topk:i`: number of output neighbour slots to return. Must be greater than `0`.
* `ktrig:k`: k-rate form only. A query runs on the rising edge of this signal.
* `iminsec:i` (optional): minimum table duration, in seconds, to run a query. Default `0`. `iminsec = 0` (or omitted) means no minimum.

## Output

* `neighs[][]`: nearest vectors (`topk × dim`), zeroed when the query is skipped.
* `scores[]`: their cosine similarity scores (length `topk`, descending). If the space
  contains fewer than `topk` matches, the remaining rows/scores stay zero.
* `kgate:k`: k-rate form only. `1` for the k-pass where a fresh async result is ready,
  otherwise `0`.

## Execution Time

* Init
* Init + Performance

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

; -----------------------------------------------------------------------------
; semspacequeryaudioft.csd
;
; semspacequeryaudioft is the real-time counterpart of semspacequeryaudio: it
; embeds the audio held in a function table (mono, engine sr), mean-pools it into
; one query vector, and returns the top-k nearest stored vectors. Here: capture
; live mic into a circular table and query the space on a trigger.
; -----------------------------------------------------------------------------

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#define AUDIO_MODEL # "path/to/audio_model_dir" #
#define AUDIO_DIR # "path/to/wav_dir" #
#define TLEN # sr * 3 #

rec@global:i = ftgen(0, 0, $TLEN, 2, 0)

a_handle@global:i = semload(-1, $AUDIO_MODEL)
; build an .espc from every .wav in a directory (audio model -> audio embedding)
semspacebuild(a_handle, "sounds.espc", $AUDIO_DIR)

; load it back into RAM (handle only fixes the dimension)
s_handle@global:i = semspace(a_handle, "sounds.espc")

; RECORD continuously overwrites the circular table `rec` with the mic input.
instr RECORD
    a1:a = inch(1)
    andx:a = phasor(sr / ftlen(rec))     ; 0..1 ramp, wraps once per table length
    tablew(a1, andx, rec, 1)             ; ixmode 1 -> normalized index
endin

; LIVE_QUERY queries the space from the capture table every ~4 s; iminsec = 2 skips the
; query until at least 2 s of audio has been captured. The k-rate form snapshots the table
; on trigger and runs inference/search on a worker thread.
instr LIVE_QUERY
    ktrig:k = metro(0.25) ; ~ every 4 s
    neighs:k[][], scores:k[], kgate:k = semspacequeryaudioft(s_handle, a_handle, rec, 3, ktrig, 2)
    if (kgate == 1) then
        println("nearest match score = %f", scores[0])
    endif
endin

</CsInstruments>
<CsScore>
i "RECORD"     0 30
i "LIVE_QUERY" 0 30
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspacequeryaudio](semspacequeryaudio.md)
* [semspacequerytxt](semspacequerytxt.md)
* [semspace](semspace.md)
* [semembedaudio](semembedaudio.md)

## Credits

Pasquale Mainolfi, 2026
