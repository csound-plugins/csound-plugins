# defer

## Abstract

Run an opcode at the end of current event

## Description


`defer` runs an opcode as the last action of a given event, during the process
of being deallocated. This can be used together with opcodes which release
resources back which are being used during the event. At the moment `defer`
works only with builtin opcodes. For complex release actions, it is nonetheless
possible to bundle the actions in a separate instrument and use `schedule`
together with defer (or simpler, use `atstop` instead).

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

    defer Sopcode, [arg1, arg2, ...]

Any arguments passed after the name of the opcode (which must be within
quotations) are passed directly to the opcode itself. Notice that only opcodes
without output make sense in this context. Useful opcodes are `print`, `prints`,
[pool_push](pool_push.md), [dict_free](dict_free.md), etc


!!! Note

    `defer` evaluates at init time but acts at deacllocation time. The arguments
    passed are evaluated at deallocation time

## Arguments

* `Sopcode`: the name of the opcode to defer
* `args`: any args (i, k, S) are passed to the opcodes. They are evaluated at
  deallocation time.


## Execution Time

* Init
* Deallocation

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
-odac           

</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

;; Example 1
instr 1
  printk2 timeinsts()
  ;; this will be called at the end of this instrument
  defer "event_i", "i", 2, 0, 1, 93
endin

instr 2
  print p4
  turnoff
endin

instr example1
  schedule 1, 0, 0.1
  turnoff
endin

;; -------------------------------------------
;; Example 2

instr 10
  idict dict_new "*sf", "foo", 100, "bar", 200
  ;; delete this key at the end of the note
  defer "dict_set", idict, "foo"
  dict_print idict
  schedule 11, 0.5, 1, idict
  turnoff
endin 

instr 11
printf "Instr 11 \n", 1
  idict = p4
  ;; by now the dict should have deleted the key "foo"
  dict_print idict
  turnoff
endin

instr example2
  schedule 10, 0, 1
endin

schedule "example2", 0, 0.1

</CsInstruments>

<CsScore>
e 3
; f0 3600

</CsScore>
</CsoundSynthesizer>

```


## See also

* [atstop](atstop.md)
* [schedule](http://www.csounds.com/manual/html/schedule.html)
* [event](http://www.csounds.com/manual/html/event.html)
* [release](http://www.csounds.com/manual/html/release.html)
* [xtratim](http://www.csounds.com/manual/html/xtratim.html)

## Credits

Eduardo Moguillansky, 2019
