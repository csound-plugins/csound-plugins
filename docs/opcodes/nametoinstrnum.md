# nametoinstrnum

## Abstract

Returns the number of a named instrument


## Description

Returns the number of a named instrument, or -1 if the instrument does
not exist.
The only difference with `nstrnum` is that `nstrnum` throws an error and
stops the event if the instrument does not exist

**NB**: since this is an init-time opcode the name must be known at init-time


## Syntax


```csound

insno nametoinstrnum Sname

```
    
## Arguments

* **Sname**: the name of the instrument


## Output

* **insno**: the instrument number assigned by csound


## Execution Time

* Init

## Examples


```csound


<CsoundSynthesizer>
<CsOptions>
-odac 
--nosound
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 64


instr foo
  turnoff
endin

instr findname
  ifoo = nametoinstrnum("foo")
  Sfindname = "findname"
  ip1 = nametoinstrnum(Sfindname)
  ibar = nametoinstrnum("bar")
  prints "foo has number: %d\n", ifoo
  prints "findname has number: %d\n", ip1
  prints "bar has number: %d\n", ibar

endin

</CsInstruments>
<CsScore>
i "findname" 0 0.1

</CsScore>
</CsoundSynthesizer>



```


## See also

* [metro](https://csound.com/docs/manual/metro.html)


## Credits

Eduardo Moguillansky, 2021
