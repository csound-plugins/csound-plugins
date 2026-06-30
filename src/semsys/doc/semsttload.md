# semsttload

## Abstract

Load an end-to-end speech-to-text ONNX model and start its background worker; return a handle.

## Description

`semsttload` initialises a **speech-to-text** context from a **model directory** and returns
an integer handle used by the other `semstt*` opcodes (`semsttsubmit*`, `semsttready`,
`semsttresult`).

The model must be a single **end-to-end** graph that takes **raw audio bytes in and gives
text out** — it is **not tied to Whisper**; any model that matches the I/O contract below
works. Everything model-specific (audio decoding and resampling, feature extraction,
encoder, decoder/beam-search, detokenizer) lives **inside the ONNX graph** (built with
[onnxruntime-extensions](https://github.com/microsoft/onnxruntime-extensions)), so the
plugin only feeds bytes and reads a string. Because of this, **swapping models is just
swapping the directory** — no recompiling, no model-specific code. Whisper is the reference
model used to test it.

Transcription is **asynchronous**. Whisper inference takes hundreds of milliseconds to
seconds, far longer than one audio period, so it must not run on the audio thread.
`semsttload` prepares **one background worker** per handle. The thread starts on the first
submit, transcribes off the audio thread, and shuts down again after a short idle period.
The submit opcodes hand a job to the worker and return immediately; you poll
[semsttready](semsttready.md) and read the text with [semsttresult](semsttresult.md).

Jobs and results are held in a **bounded FIFO queue**. `iqueue` sets the capacity; if the
job queue is full, the newest submit is dropped with a warning while already queued jobs
stay queued. If the result queue is full, the newest result is dropped with a warning. Size
the queue as `ceil(transcription_time / submit_interval) + margin`; the default is `256`
and values above `512` are clamped.

### One pipeline per handle

A handle is a **single transcription pipeline**: one worker and one shared queue. Every
`semsttsubmit*` that uses the same handle — from any instrument — feeds the **same** queue,
and [semsttresult](semsttresult.md) returns the next finished transcription in **submission
(FIFO) order**. The result is **not** addressed to a particular submit: if two instruments
submit to the same handle, the texts simply interleave in order and you cannot tell which
text came from which submit. (Order is well defined — the single worker processes serially,
so result order equals submission order — but across concurrent submitters the submission
order itself is nondeterministic.)

For **independent or parallel flows**, create a **separate handle** (`semsttload`) per flow:
each gets its own worker and queue, fully isolated.

Any running or finished worker is joined automatically when the instance is deinitialised.
Pending jobs/results are discarded when the STT handle itself is destroyed; the handle does
not block to drain the queue at shutdown.

## Limits — audio window and output length

Two limits matter, and **both depend on the model** (the values below are for Whisper, the
reference model — a different end-to-end model may differ):

* **Audio window.** Each transcription processes a **single** audio window: the model's
  feature extractor pads or trims the input to a **fixed length set by the model** (Whisper:
  **~30 s**, 480000 samples at 16 kHz). Audio **longer than that window is not fully
  transcribed** — only the first window is. The output is one continuous string (the whole
  recognised text on one line, not split into lines).

* **Output tokens — `maxlen`.** `maxlen` caps the number of output tokens per window — the
  maximum transcription length. Its sensible value comes from the model (Whisper: `448`).

To transcribe audio **longer than the model's window**, **split it into segments** shorter
than the window and submit each (`semsttsubmit*`); the results come back in submission order,
concatenate them yourself. Alternatively use [semsttsubmitlive](semsttsubmitlive.md), which
captures a stream and submits usable speech windows automatically using its built-in energy
gate, or on an explicit trigger.

## Tokenizer / audio runtime dependency: onnxruntime-extensions

The end-to-end graph uses **custom ops** from onnxruntime-extensions (`AudioDecoder`,
`StftNorm`, the BPE decoder, all in the `ai.onnx.contrib` domain). `semsttload` registers
the native extensions shared library on the session. Provide it as for the embedding model
(see [semload](semload.md), section *onnxruntime-extensions*): next to the plugin, in the
model directory, or via the `SEMSYS_ORT_EXTENSIONS` environment variable.

## Model directory

The directory passed to `semsttload` must contain:

* `model.onnx` — the end-to-end graph (**required**);
* `model.onnx.data` — its external weights (**required**: export the model with the
  external-data format so the weights live in this file; the loader checks for it).

### I/O contract

The graph must expose exactly these tensors (model-agnostic; Whisper is the reference):

* input `audio_stream` — `uint8 [1, N]`, the raw bytes of an audio file (any format the
  graph's audio decoder supports);
* scalar generation inputs `max_length`, `min_length`, `num_beams`,
  `num_return_sequences`, `length_penalty`, `repetition_penalty` — each shape `[1]`
  (`int32` / `float`);
* output `str` — a string tensor, the transcription.

### Exporting the model (Whisper reference)

Build the end-to-end graph with onnxruntime + onnxruntime-extensions:

1. export the core beam-search model with
   `onnxruntime.transformers.models.whisper.convert_to_onnx`;
2. generate the audio pre-processing (`audio_stream` → features) and detokenizer
   post-processing (`sequences` → text) graphs with
   `onnxruntime_extensions.gen_processing_models`
   (`pre_kwargs={"USE_AUDIO_DECODER": True, "USE_ONNX_STFT": False}`);
3. stitch `pre → core → post` with `onnx.compose.merge_models` (align IR / opset versions),
   and save with external data as `model.onnx` (+ `model.onnx.data`).

## Syntax

```csound
handle:i = semsttload(model_dir:S, maxlen:i [, queue:i])
```

## Arguments

* `model_dir:S`: directory containing the end-to-end model (see *Model directory* above).
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

; the directory must contain the end-to-end model.onnx (+ its .data)
h@global:i = semsttload("path/to/whisper_e2e", 448, 256)

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
