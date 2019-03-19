# atstop

## Abstract

Schedule an instrument at the end of the current instrument

## Description


`atstop` can be used to schedule an instrument event as the last action
of a given instrument, during the process of being deallocated. This can
be used to notify when the note has actually stopped, or to schedule a 
chain of events, free any table or dict allocated, etc. The advantage
over the `release` opcode is that `atstop` is guaranteed to be run 
after the note has stopped, so there is no danger in deallocating resources
being used by this note, there are no conflicts with release envelopes, etc.

**NB**: the event is not scheduled at *release* time (see below "Release time vs Deinit time")
but at the moment the note is freed.

### Release time vs deinit time

**Release time** is when the note is within its *release phase*, which will only happen 
if the note has an envelope with a release segment (like `linsegr`), or if it has 
setup extra time with `xtratim`. The opcode `release` can be used to query if the 
current note is being released. The instrument keeps running in release phase as 
long as the release part of the envelope is finished or the extra time allocated
via `xtratim` is through. 

**Deinit time** is the moment the note is actually being freed, so the instrument is
not running anymore at this point.  

## Syntax

    atstop instrnum, idelay, idur [, p4, p5, ...]
    atstop Sinstrname, idelay, idur [, p4, p5, ...]
          

`atstop` executes only at **init time**. 
    
## Arguments

* `instrnum` / `Sintrname`: the number or the name of the instr to be scheduled
* `idelay`: the time offset **after** the stop time of this note to start this instrument
* `idur`: the duration of the event
* `p4`, `p5`, ...: any other p-arguments, as used with similar opcodes like `schedule`, `event`, etc.

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
-odac

</CsOptions>
<CsInstruments>

/*

  # This is the example file for the atstop opcode

  atstop schedules an instrument at the end of the note
  This might be useful to notify that a note stopped, or
  to schedule a chain of notes, etc.

  NB: the scheduled event is NOT triggered at release time,
  which might be when this note is still playing, if the note
  has some release envelope, but when the note is being deleted

  ## Syntax

    atstop Sinstrname, idelay, idur, ...
    atstop instrnum, idelay, idur, ...

  * Sinstrname / instrnum: the name or the instrument number to schedule
  * idelay: time offset to this note stop
  * idur: duration of the scheduled event (-1 = forever)

*/

sr = 44100
ksmps = 64
0dbfs = 1
nchnls = 2

opcode myvco,a,ii
  iamp, ifreq xin
  a0 vco2, iamp, ifreq
  a0 += vco2(iamp, ifreq+2)
  a0 += vco2(iamp, ifreq / 2)
  xout a0
endop

; brownian walk
instr 1
  imidi = p4
  a0 myvco 0.1, mtof:i(imidi)
  outs a0, a0
  idelta = round((rnd(4) - 1) * 2)/2
  idelta = idelta != 0 ? idelta : -0.5
  imidi2 = imidi + idelta
  imidi2 = imidi2 < 96 ? imidi2 : 48
  idurnext = round(rnd(0.25)*8) / 8
  atstop 1, 0, idurnext, imidi2
endin

; ping-pong
instr 2
  imidi = p4
  a0 myvco 0.1, mtof:i(imidi)
  a0 *= linsegr(0, 0.05, 1, 0.05, 0)
  outs a0, a0
  imidi = imidi < 96 ? imidi : 48
  
  atstop 3, 0, p3, imidi + 1
endin

instr 3
  imidi = p4
  a0 oscili 0.5, mtof:i(imidi)
  a0 *= linsegr(0, 0.05, 1, 0.050, 0)
  outs a0, a0
  imidi = imidi < 96 ? imidi : 48
  atstop 2, 0, p3*0.75, imidi + 1
endin


instr 100
  exitnow
endin

</CsInstruments>
<CsScore>
; i 1 0 0.25 36
i 2 0 0.25 48
i 100 20 1

</CsScore>
</CsoundSynthesizer>


```


## See also

* `schedule`
* `event`
* `release`
* `xtratim`

## Credits

Eduardo Moguillansky, 2019