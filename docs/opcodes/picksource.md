# picksource

## Abstract

Select one of multiple signals basen

## Description

Given a set of signals, this opcode picks one basen on a given index
At the moment no mixing is performed, only the selected signal is output

## Syntax


```csound

asig picksource kindex, asig1, asig2, ...

```

## Arguments

* **asig1**: The first audio signal.
* **asig2**: The second audio signal.
* **...**: Additional audio signals.

## Output

* **asig**: The selected audio signal.

## Execution Time

* Performance

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

/* Example file for picksource

aout picksource kwich, asource1, asource2, ...

  Given a set of signals, this function selects one based on a given index.

  At the moment no mixing is performed, only the selected signal is output.

*/

instr 1
  asig1 = oscili:a(0.1, 500) + oscili:a(0.1, 600)
  asig2 = buzz(0.1, 300, 7, -1)
  asig3 = pinker() * 0.1
  ktick = metro(1)
  Snames[] fillarray "sine", "buzz", "pink"
  ksource init 0
  if ktick == 1 then
    ksource = (ksource + 1) % 3
    println "Source index: %d, signal: %s", ksource, Snames[ksource]
  endif
  asig = picksource(ksource, asig1, asig2, asig3)
  outch 1, asig
endin

</CsInstruments>

<CsScore>
i1 0 10

</CsScore>
</CsoundSynthesizer>



```


## See also

* [panstereo](panstereo.md)


## Metadata

* Author: Eduardo Moguillansky
* Year: 2025
* Plugin: else
* Source: https://github.com/csound-plugins/csound-plugins/blob/master/src/else/src/else.c
