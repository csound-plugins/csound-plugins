# semsttload

## Abstract

Load an end-to-end speech-to-text ONNX model and start its background worker; return a handle.

## Description

`semsttload` initialises a **speech-to-text** context from a **model directory** and returns
an integer handle used by the other `semstt*` opcodes (`semsttsubmit*`, `semsttready`,
`semsttresult`).

The model must be a single **end-to-end** graph that takes **raw audio bytes in and gives
text out**. Any model that matches the I/O contract below works.
Transcription is **asynchronous**.
Jobs and results are held in a **bounded FIFO queue**. `iqueue` sets the capacity; if the
job queue is full, the newest submit is dropped with a warning while already queued jobs
stay queued. If the result queue is full, the newest result is dropped with a warning. Size
the queue as `ceil(transcription_time / submit_interval) + margin`; the default is `256`
and values above `512` are clamped.

A handle is a **single transcription pipeline**: one worker and one shared queue. Every
`semsttsubmit*` that uses the same handle, from any instrument, feeds the **same** queue,
and [semsttresult](semsttresult.md) returns the next finished transcription in **submission
order**. The result is **not** addressed to a particular submit: if two instruments
submit to the same handle, the texts simply interleave in order and you cannot tell which
text came from which submit.

For **independent or parallel flows**, create a **separate handle** (`semsttload`) per flow:
each gets its own worker and queue, fully isolated.

## Limits — audio window and output length

Two limits matter, and **both depend on the model** (the values below are for Whisper, the
reference model, a different end-to-end model may differ):

* **Audio window.** Each model call processes a **single** audio window: the model's
  feature extractor pads or trims the input to a **fixed length set by the model** (Whisper:
  **~30 s**, 480000 samples at 16 kHz). A single call **cannot** fully transcribe audio
  longer than that window. The offline opcodes work around this by segmenting (below); the
  output is one continuous string (the whole recognised text on one line, not split lines).

* **Output tokens — `maxlen`.** `maxlen` caps the number of output tokens per window. Its sensible value comes from the model (Whisper: `448`).

Audio **longer than the model's window** is handled **automatically** by the offline opcodes
[semsttsubmitfile](semsttsubmitfile.md), [semsttsubmitarray](semsttsubmitarray.md) and
[semsttsubmitft](semsttsubmitft.md): they split the audio into `<=` window segments, transcribe each
segment on the worker, and join the texts into one result (one submit, one transcription).
For live capture use [semsttsubmitlive](semsttsubmitlive.md), which submits usable speech
windows automatically using its built-in energy gate, or on an explicit trigger.

## Model directory

The directory passed to `semsttload` must contain:

* `model.onnx` — the end-to-end graph (**required**);
* `model.onnx.data` — external weights, only when the graph uses ONNX external data.
  If this file exists, keep it in the same directory as `model.onnx`.

### I/O contract

The graph must expose exactly these tensors (model-agnostic; Whisper is the reference):

* input `audio_stream` — `uint8 [1, N]`, the raw bytes of an audio file (any format the
  graph's audio decoder supports);
* scalar generation inputs `max_length`, `min_length`, `num_beams`,
  `num_return_sequences`, `length_penalty`, `repetition_penalty`, each shape `[1]` (`int32` / `float`);
* output `str`, a string tensor, the transcription.

## Syntax

```csound
handle:i = semsttload(model_dir:S, maxlen:i [, queue:i])
```

## Arguments

* `model_dir:S`: directory containing the end-to-end `model.onnx` graph. If the graph
  has a `model.onnx.data` external weights file, it must be in the same directory.
* `maxlen:i`: maximum number of output tokens, fed to the graph's `max_length` input. It is
  **not** a free value — take it from the model's configuration (e.g. its max target
  positions / generation `max_length`). For Whisper, `448`.
* `queue:i` (optional, default `256`, max `512`): FIFO capacity (pending jobs + ready
  results). Size it as `ceil(transcription_time / submit_interval) + margin`.

## Output

* `handle:i`: handle to the loaded speech-to-text context.

## Execution Time

* Init

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

; the directory must contain model.onnx; keep model.onnx.data here too if present
h@global:i = semsttload("path/to/model_e2e", 448, 256)

instr transcribe
    semsttsubmitfile(h, "speech.wav")     ; returns immediately
endin

instr poll
    ready:k = semsttready(h)
    if (ready == 1) then
        text:S, len:k = semsttresult(h)
        printf("%s\n", ready, text)
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i "transcribe" 0 0.1
i "poll" 0 30
</CsScore>
</CsoundSynthesizer>
```

## See also

* [semsttsubmitfile](semsttsubmitfile.md)
* [semsttsubmitarray](semsttsubmitarray.md)
* [semsttsubmitft](semsttsubmitft.md)
* [semsttsubmitlive](semsttsubmitlive.md)
* [semsttready](semsttready.md)
* [semsttresult](semsttresult.md)
* [semload](semload.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
