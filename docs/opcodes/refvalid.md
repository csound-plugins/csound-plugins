# refvalid

## Abstract

Queries if a reference is valid

## Description


`ref` and `deref` implement a mechanism to pass a reference to any object,
allowing to share a variable across instruments, with opcodes, etc. A reference
is a proxy to an axisting variable / array. A reference is reference counted and
deallocates itself when it falls out of scope without being referenced by any
object. Since a reference is just an integer, `refvalid` can be used to check if
the given reference index corresponds to a valid reference

## Syntax

```csound
iout refvalid iref
kout refvalid kref
```   

## Arguments

* `iref` / `kout`: an integer identifying the reference handle, as passed via [ref](ref.md)


## Output

* `ìout` / `kout`: 1 if the reference is valid, 0 otherwise

## Execution Time

* Init
* Performance

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
-m0
-d
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
  kfreq linseg 0, p3, 1
  schedule 2, 0, p3, ref(kfreq)
endin

instr 2
  if refvalid(p4) == 1 then
    kfreq = deref(p4)
  else
    kfreq = 1000
  endif
  asig vco2 0.2, kfreq
  schedule 3, 0, p3, ref(asig)
endin

instr 3
  if refvalid(p4) == 1 then
    ain deref p4
  else
    ain = 0
  endif
  aout lpf18 ain, 2000, 0.9, 0.2
  outs aout, aout
endin


```

## See also

* [deref](deref.md)
* [ref](ref.md)
* [defer](defer.md)
* [schedule](http://www.csounds.com/manual/html/schedule.html)
* [event](http://www.csounds.com/manual/html/event.html)
* [release](http://www.csounds.com/manual/html/release.html)

## Credits

Eduardo Moguillansky, 2019
