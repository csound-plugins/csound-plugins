# semsttresult

## Abstract

Read the next finished transcription (and its length) from a speech-to-text context.

## Description

`semsttresult` returns the oldest completed transcription (FIFO order) from the handle's
result queue and removes it, along with the transcription's **length** in characters. If no
result is waiting it returns an empty string with length `0`, so gate it with
[semsttready](semsttready.md).

The `length` output is handy to tell whether a window actually contained speech: a silent or
speechless window transcribes to an empty string (`length == 0`), which you can skip.

Each call **consumes** one result. To drain several pending results, call it once per
control pass while [semsttready](semsttready.md) is `1`. The results come out in the same
order the audio was submitted.

The queue is **shared by the whole handle**: a result is not tied to the submit (or the
instrument) that produced it — `semsttresult` just returns the next finished one. If several
instruments submit to the same handle, the texts interleave in FIFO order. For independent
flows, use a separate handle per flow (see [semsttload](semsttload.md)).

## Syntax

```csound
text:S, length:k = semsttresult(handle:i)
```

## Arguments

* `handle:i`: handle returned by [semsttload](semsttload.md).

## Output

* `text:S`: the next transcription, or `""` if none is ready.
* `length:k`: its length in characters (`0` for an empty / no-speech result).

## Execution Time

* Performance

## Examples

```csound
instr poll
    ready:k = semsttready(h)
    if (ready == 1) then
        text:S, len:k = semsttresult(h)   ; pops the oldest result
        if (len > 0) then                 ; skip empty (silent) windows
            printf("%s\n", ready, text)
        endif
    endif
endin
```

## See also

* [semsttload](semsttload.md)
* [semsttsubmitfile](semsttsubmitfile.md)
* [semsttsubmitlive](semsttsubmitlive.md)
* [semsttready](semsttready.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
