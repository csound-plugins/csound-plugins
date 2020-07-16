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
If any k-variables are passed to the scheduled instr these will reflect the 
changes at the end of the instr.

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

atstop instrnum   [, idelay=0, idur=-1, p4, p5, ...]
atstop Sinstrname [, idelay=0, idur=-1, p4, p5, ...]

```          

> `atstop` executes only at **init time**. 
    
## Arguments

* `instrnum` / `Sintrname`: the number or the name of the instr to be scheduled
* `idelay`: the time offset **after** the stop time of this note to start this instrument
* `idur`: the duration of the event
* `p4`, `p5`, ...: any other p-arguments, as used with similar opcodes like `schedule`, `event`, etc.
    They can be any i-, k- or S- variable. The scheduled instr will access them, as p-args.

## Execution Time

* Init 

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
  imidi = imidi < 96 ? imidi : 48
  
  a0 myvco 0.1, mtof:i(imidi)
  a0 *= linsegr(0, 0.05, 1, 0.05, 0)
  outs a0, a0
  
  atstop 3, 0, p3*0.97, imidi + 1
endin

instr 3
  imidi = p4
  imidi = imidi < 88 ? imidi : 48
  a0 oscili 0.8, mtof:i(imidi)
  a0 *= linsegr(0, 0.05, 1, 0.050, 0)
  outs a0, a0
  atstop 2, 0, p3*0.95, imidi + 1
endin


; test calling a named instr at stop
instr 10
  a0 oscili 0.1, 440
  outs a0, a0
  atstop "foo", 0.5, 1, 1000
endin

instr foo 
  ifreq = p4
  a0 oscili 0.1, ifreq
  outs a0, a0
endin

; test simple case with optional pargs
instr first
  atstop "second", 1, -1, 0.5
  atstop "second", 0.5
  atstop 200
endin

instr second
  printf "second!  p4 =%f \n", 1, p4
  turnoff
endin

instr 200
  printf "200! \n", 1
  turnoff
endin

; test atstop with k args
instr _printCounter
  icounter = p4
  prints "counter: %d\n", icounter
  turnoff
endin

instr kargs
  kcounter init 0
  kcounter += 1
  atstop "_printCounter", 0, -1, kcounter
endin

instr StopPerformance
  exitnow
endin


</CsInstruments>
<CsScore>
; i 1 0 0.25 36

; i 2 0 0.25 48

; i 10 0 1
; i "StopPerformance" 10 1
; i "first" 1 0.5
i "kargs" 0 1
f 0 5
</CsScore>
</CsoundSynthesizer>


```


## See also

* [defer](defer.md)
* [schedule](http://www.csounds.com/manual/html/schedule.html)
* [event](http://www.csounds.com/manual/html/event.html)
* [release](http://www.csounds.com/manual/html/release.html)
* [xtratim](http://www.csounds.com/manual/html/xtratim.html)

## Credits

Eduardo Moguillansky, 2019