# trigLinseg

## Abstract

Trace a series of line segments between specified points.

### Description

Trace a series of line segments between specified points. Unlike the `linseg` opcode, this opcode can be triggered during perf time.

### Syntax
```csound
ares trigLinseg kTrig, ia, idur1, ib [, idur2] [, ic] [...]
kres trigLinseg kTrig, ia, idur1, ib [, idur2] [, ic] [...]
```

### Initialization
* **kTrig** -- trigger signal. When kTrig is greater or equal to one, it will trigger the breakpoint envelope. If you trigger the envelop before it has finished, it will result in clicks. 
* **ia** -- starting value.
* **ib**, **ic**, etc. -- value after dur1 seconds, etc.
* **idur1** -- duration in seconds of first segment. A zero or negative value will cause all initialization to be skipped.
* **idur2**, **idur3**, etc. -- duration in seconds of subsequent segments. A zero or negative value will terminate the initialization process with the preceding point, permitting the last-defined line or curve to be continued indefinitely in performance. The default is zero.

### Performance

These units generate control or audio signals whose values can pass through 2 or more specified points. The sum of dur values may or may not equal the instrument's performance time: a shorter performance will truncate the specified pattern, while a longer one will cause the last value to be repeated until the end of the note.

### Example
Here is an example of the trigLinseg opcode.

```csound
<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac 
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o diskin.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
0dbfs=1

instr 1
    kTrig metro .5
    aEnv trigLinseg kTrig, 0, 1, 1, 1, 0
    a1 oscili aEnv, 400
    outs a1, a1
endin

</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>
```

### See Also

* [trigExpseg](trigExpseg.md)

### Credits
Author: Rory Walsh
2018
