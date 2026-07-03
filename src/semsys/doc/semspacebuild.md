# semspacebuild

## Abstract

Bulk-build a `.espc` semantic space from a text or audio source, using the model you pass.

## Description

`semspacebuild` reads a source, embeds it, and writes the resulting vectors to a new `.espc`
file. Use it to build a space from a corpus in one pass instead of adding items one by one.
Load the result later with [semspace](semspace.md).

`semspacebuild` is intended as an **offline/init-time builder**. In normal use, call it
before creating/loading the runtime space, then load the generated `.espc` with
[semspace](semspace.md). Although it can run at init time inside an instrument, this can
block while the corpus is embedded and is not the recommended pattern for performance-time
updates. To populate an already-created in-memory space during a performance, use
[semspaceaddtxt](semspaceaddtxt.md) or [semspaceaddaudio](semspaceaddaudio.md) instead.

It is **universal**: the embedding model comes from `handle` (from [semload](semload.md)),
and semsys dispatches on the model's **kind**, detected at load time: a **text** model `source` is read as text (a `.txt` file, or a directory of `.txt`), split into overlapping token-window chunks, one vector per chunk (see the chunking notes below); an **audio** model `source` is decoded as audio (a PCM16 WAV file, or a directory of `.wav`), split into fixed ~10 s windows, one L2-normalized vector per window; near-silent windows are skipped.

Because the `.espc` format stores only dim + vectors, `.espc` files built from a **text**
model and from an **audio** model can be **merged into one space**, as long as their
embedding dimensions match (loading validates this).

## Syntax

```csound
semspacebuild(handle:i, dest:S, source:S)
```

## Arguments

* `handle:i`: a text or audio embedding model from [semload](semload.md); its kind selects how `source` is embedded.
* `dest:S`: path to the `.espc` file to create (overwritten if present).
* `source:S`: a file, or a directory, to embed (`.txt` for a text model, `.wav` for audio).

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

e_handle@global:i = semload(256, "path/to/text_model_dir")
a_handle@global:i = semload(-1, "path/to/audio_model_dir")

; build "corpus.espc" from a text corpus (text model)
semspacebuild(e_handle, "corpus.espc", "corpus.txt")

; build "sounds.espc" from a directory of .wav (audio model)
semspacebuild(a_handle, "sounds.espc", "path/to/wavs")

instr 1
    ; load a built space and query it (with a matching-kind model)
    s:i = semspace(e_handle, "corpus.espc")
    neighs:k[][], scores:k[], kgate:k = semspacequerytxt(s, e_handle, "warm analog texture", 3)
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semspace](semspace.md)
* [semspaceaddtxt](semspaceaddtxt.md)
* [semspaceaddaudio](semspaceaddaudio.md)
* [semspacequerytxt](semspacequerytxt.md)
* [semspacequeryaudio](semspacequeryaudio.md)
* [semload](semload.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
