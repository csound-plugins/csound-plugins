# semsttready

## Abstract

Poll whether a finished transcription is waiting to be read.

## Description

`semsttready` returns `1` when at least one transcription has completed and is waiting in
the result queue, else `0`. It is a cheap, non-blocking k-rate poll used to gate
[semsttresult](semsttresult.md): submit audio with a `semsttsubmit*` opcode, poll
`semsttready`, and read the text when it turns `1`.

Because results are queued FIFO on the handle configured by [semsttload](semsttload.md),
`semsttready` stays `1` while more than one result is pending — read one per pass with
[semsttresult](semsttresult.md) until it returns `0`.

## Syntax

```csound
ready:k = semsttready(handle:i)
```

## Arguments

* `handle:i`: handle returned by [semsttload](semsttload.md).

## Output

* `ready:k`: `1` if a transcription is ready to read, else `0`.

## Execution Time

* Performance

## Examples

```csound
instr poll
    ready:k = semsttready(h)
    if (ready == 1) then
        text:S, len:k = semsttresult(h)
        printf("%s\n", ready, text)
    endif
endin
```

## See also

* [semsttload](semsttload.md)
* [semsttsubmitfile](semsttsubmitfile.md)
* [semsttsubmitlive](semsttsubmitlive.md)
* [semsttresult](semsttresult.md)

## Credits

Author: Pasquale Mainolfi<br>
Italy<br>
June 2026.
