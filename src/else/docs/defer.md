# defer

## Abstract

Run an opcode at the end of an event

## Description


`defer` can be used to execute the init pass of an opcode **at the end** of an event.
This can be used to perform some cleanup, schedule an event, print something, etc.

!!! Note "Release time vs deinit time"

    The event is not scheduled at *release* time (see below "Release time vs Deinit time")
    but at the moment the note is freed.

    **Release time** is when the note is within its *release phase*, which will only happen 
    if the note has an envelope with a release segment (like `linsegr`), or if it has 
    setup extra time with `xtratim`. The opcode `release` can be used to query if the 
    current note is being released. The instrument keeps running in release phase as 
    long as the release part of the envelope is finished or the extra time allocated
    via `xtratim` is through. 
    
    **Deinit time** is the moment the note is actually being freed, so the instrument is
    not running anymore at this point.  

## Syntax

```csound
defer Sopcode *args
```
            
## Arguments

* `Sopcode`: the name of the opcode to run at deinit time
* `args`: arguments passed to the opcode itself

## Execution Time

* Deinit time

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
-odac

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
0dbfs = 1
nchnls = 2


instr 1
  kt timeinsts
  prints "Instr 1 started"
  defer "prints", "Instr 1 stopped after %.3f seconds", kt
endin


</CsInstruments>
<CsScore>
i 1 0 1
f 0 4

</CsScore>
</CsoundSynthesizer>


```


## See also

* [defer](defer.md)
* [schedule](http://www.csound.com/docs/manual/schedule.html)
* [event](http://www.csound.com/docs/manual/event.html)
* [release](http://www.csound.com/docs/manual/release.html)
* [xtratim](http://www.csound.com/docs/manual/xtratim.html)

## Credits

Eduardo Moguillansky, 2019