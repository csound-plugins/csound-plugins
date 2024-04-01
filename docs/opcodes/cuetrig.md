# cuetrig

## Abstract

Generate a trigger at given time values

## Description

Given a time signal and a list of time stamps, `cuetrig` generates a 
trigger whenever this signal crosses over one of the given timestamps.
The output value is the index (starting at 1) of the timesamp crossed.


## Syntax


```csound
ktrig cuetrig ktime, itime0, ..., itimen

```
    
## Arguments

* **kstep**: ktime. A value acting as the time signal. It must be monotonically ascending.
  If the value is ever lower than a previous value, the sequence is reset
* **itimen**: time stamps

## Output

* **ktrig**: index of the timestamp crossed, or 0 if no timestamp crossed at this cycle

## Execution Time

* Performance

## Examples


```csound


<CsoundSynthesizer>
<CsOptions>

</CsOptions>

<CsInstruments>

0dbfs = 1

instr 1
  kt = eventtime() % 2
  ktrig cuetrig kt, 0.0, 0.1, 0.12, 0.4
  knotes[] fillarray 60, 64, 67, 67.5
  if ktrig != 0 then 
    println "ktrig: %d, kt: %f, note: %f", ktrig, kt, knotes[ktrig-1]
    schedulek 2, 0, 0.5, knotes[ktrig-1]
  endif
endin

instr 2
  ifreq = mtof:i(p4)
  outch 1, vco2:a(0.1, ifreq) * linsegr:a(0, 0.01, 1, 0.05, 0.1, 0.2, 0)
endin

</CsInstruments>
<CsScore>

i1 0 8

</CsScore>
</CsoundSynthesizer>



```


## See also

* [accum](accum.md)
* [metro](http://www.csound.com/docs/manual/metro.html)
* [changed](http://www.csound.com/docs/manual/changed.html)
* [trighold](http://www.csound.com/docs/manual/trighold.html)

## Credits

Eduardo Moguillansky, 2024
