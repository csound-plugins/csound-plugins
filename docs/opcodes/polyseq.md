# polyseq

## Abstract

`polyseq` creates and controls multiple **sequential** version of an opcode

## Description

`polyseq` creates a user given number of instances of an opcode, each with its own state.
The output of an instance is routed to the input of the next instance, forming a chain.
Each instance can be controlled individually.

We differentiate between `chained` and `multiplexed` arguments. `chained` arguments are
the ones which are mapped from one opcode instance to the next, so that the output of 
one instance is routed to the input of the next. `multiplexed` are passed after the 
`chained` arguments and work as they do with `poly`: if an array is passed, then each 
instance is delivered one element of this array, and if a scalar value is passed, then 
all instances share the same argument.

### Example

All this is best explained with an example:

```csound

; these two signal chains produce the same result
a0 pinker
kFreqs[] fillarray 100, 150, 300, 330

aeq rbjeq a0, kFreqs[0], 2, 10, 1, 8
aeq rbjeq aeq, kFreqs[1], 2, 10, 1, 8
aeq rbjeq aeq, kFreqs[2], 2, 10, 1, 8
aeq rbjeq aeq, kFreqs[3], 2, 10, 1, 8

aseq polyseq 4, "rbjeq", a0, kFreqs, 2, 10, 1, 8
```

## Syntax

    xouts polyseq numinstances:i, opcodename:s, xins, params ...
    
`polyseq` can have any number of inputs and outputs, as long as the opcode
used has matching inputs and outputs.     

## Arguments

* `inuminstances`: the number of instances of `Sopcode` to instantiate
* `Sopcode`: the name of the opcode
* `xins`: any number of arguments, either k- or a-rate, which should correspond
          to the outputs of the opcode

The number and type of the input arguments depend on the arguments passed to the 
given opcode. The same applies for the output arguments

### Output

`xouts`: any number of arguments of type `k` or `a`, as output by the opcode


## Examples

[LISTEN](https://raw.githubusercontent.com/gesellkammer/csound-plugins/master/src/poly/examples/polyseq.mp3)

```csound 

<CsoundSynthesizer>
<CsOptions>
; -odac           

</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 2
0dbfs  = 1

; Example file for polyseq

instr 1
  ; A simple parametric eq.
  ; On the left channel, the reference signal,
  ; on the right, the polyseq implementation.
  ; These should be the same

  a0 pinker
  a0 *= ampdb(-12)
  
  kFreqs[] fillarray 400, 1400, 3000, 4400, 8000, 12000
  kQs[] fillarray    10,    20,   10,   20,   10,    20
  kV = ampdb(18)
  
  aref = a0
  aref rbjeq aref, kFreqs[0], kV, kQs[0], 1, 8
  aref rbjeq aref, kFreqs[1], kV, kQs[1], 1, 8
  aref rbjeq aref, kFreqs[2], kV, kQs[2], 1, 8
  aref rbjeq aref, kFreqs[3], kV, kQs[3], 1, 8
  aref rbjeq aref, kFreqs[4], kV, kQs[4], 1, 8
  aref rbjeq aref, kFreqs[5], kV, kQs[5], 1, 8

  aseq polyseq lenarray(kFreqs), "rbjeq", a0, kFreqs, kV, kQs, 1, 8

  outs aref, aseq
  
endin

opcode rndarr, i[], iii
  inum, imin, imax xin 
  iOut[] init inum 
  i0 = 0
  idelta = imax - imin
  while i0 < inum do 
    iOut[i0] = imin + unirand(idelta)
    i0 += 1
  od 
  xout iOut
endop


instr 2
  ; two varying eqs
  a0 = pinker() * ampdb(-8)

  kFreqs0[] fillarray 50, 130, 400, 500, 1400, 3000, 4400, 5000, 8000
  inum lenarray kFreqs0

  iFreqslfo[] rndarr inum, 0.05, 0.4
  kLfos[] poly inum, "lfo", 0.1, iFreqslfo

  kFreqsL[] = kFreqs0 * (1+kLfos)
  kFreqsR[] = kFreqs0 * (1+kLfos*1.62)

  kQs[] poly inum, "lfo", 5, iFreqslfo*1.5
  kQs += 7.5

  aeqL polyseq inum, "rbjeq", a0, kFreqsL, ampdb(10), kQs, 1, 8
  aeqR polyseq inum, "rbjeq", a0, kFreqsR, ampdb(12), kQs*1.5, 1, 8

  ; declick
  aenv = linsegr(0, 0.5, 1, 0.5, 0)
  outs aeqL*aenv, aeqR*aenv
endin
      
</CsInstruments>
<CsScore>

i 1 0 5
i 2 5 20

</CsScore>
</CsoundSynthesizer>

```

## See also

* [poly](poly.md)
* `maparray`

## Credits

Eduardo Moguillansky, 2019
