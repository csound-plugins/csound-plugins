# poly

## Abstract

`poly` creates and controls multiple parallel version of an opcode

## Description

`poly` creates a user given number of instances of an opcode, each with its own state,
inputs and outputs. The resulting output is an array where each element holds the 
output of the corresponding instance. 

In general, an opcode has a signature, given by the number and types of its output and
input arguments. For example, the opcode `oscili` as used like `aout oscili kamp, kfreq`
has a signature `a / kk` (`a` as output, `kk` as input). 

To follow this example, to create 10 parallel versions of this opcode (an oscillator bank)
it is possible to use `poly` like this:

```csound

kFreqs[] fillarray 100, 110, 200, 220, 300, 330, 400, 440, 500, 550
aOut[] poly 10, "oscili", 0.1, kFreqs

``` 

Notice that it is possible to set one value for each instance, as given by `kFreqs`, or
one value to be shared by all instances, as given by the amplitude `0.1`. By changing
the array `kFreqs` it is possible to modify the frequency of each oscillator.

It is of course possible to chain multiple `poly`s to generate complex effect chains,
and `poly` can also be used with k-values. 

## Syntax

    out1[], [ out2[], ... ] poly inuminstances, Sopcode, xarg0, [xarg1, ...]

## Arguments

* `inuminstances`: the number of instances of `Sopcode` to instantiate
* `Sopcode`: the name of the opcode
* `xargs`: any number of arguments, either i-, k- or a-rate, either scalar or arrays, 
           as needed by the given opcode

The number and type of the input arguments depend on the arguments passed to the 
given opcode. The same applies for the output arguments

**NB**: output arguments are always arrays, input arguments can be arrays, in which
case they must be at least the size of `inuminstances`, or scalars, in which case
the same value is shared by multiple instances

### Output

The output is one or more arrays of k- or a-type, corresponding to the opcode. For instance,
an opcode like `aout oscili 0.1, kfreq` will output an array of audio channels. An opcode like
`pan2` will output two audio arrays.

## Examples

[LISTEN](https://raw.githubusercontent.com/gesellkammer/csound-plugins/master/src/poly/examples/poly.mp3)

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


; create an array of random values between 0-1
; inum: number of elements in the array
opcode rndarr, i[], i
  inum xin 
  iOut[] init inum 
  i0 = 0
  while i0 < inum do 
    iOut[i0] = unirand(1)
    i0 += 1
  od 
  xout iOut
endop

; multiple oscillators, mixed down to mono
instr 1
  ; number of oscillators
  inum = 100
  
  ; fundamental
  kmidi = line(ntom:i("2G"), p3, ntom:i("1C"))
  kf0 = mtof(kmidi)

  ; each oscillator is an overtone of f0
  kRatios[] genarray_i 1, inum
    
  ; harmonicity over time
  kexp line 1, p3, 1.32

  ; the freq. of each oscillator
  kFreqs[] = (kRatios ^ kexp) * kf0

  ; array of random phases, to avoid synchronous start
  iPhs[] rndarr inum 

  ; generate the oscillators. 
  aOscils[] poly inum, "oscili", 1/inum, kFreqs, -1, iPhs

  amono sumarray aOscils
  amono *= linsegr:a(0, 0.05, 1, 0.05, 0)
  outs amono, amono
endin

; poly instances stacked as a processing pipe
instr 2
  ; amount of polyphony
  inum = 30

  iFreqs[] rndarr inum 
  
  ; the ratios of the overtones
  kRatios[] genarray_i 1, inum
  
  ; a down gliss
  km0 linseg ntom:i("2C"), p3*0.8, ntom:i("1C")
  kf0 = mtof(km0)

  ; harmonicity curve
  kp linsegb 1, p3*0.8, 1.68

  ; calculate actual freqs.
  kFreqs[] = (kRatios ^ kp) * kf0

  ; lfo freqs. used for AM and panning
  kf linsegb 0.1, p3*0.62, 0.8, p3*0.8, 0.8, p3*0.96, 12, p3, 60
  kLfoFreqs[] = iFreqs * kf 
  
  ; multiple noise instances, amplitude modulated
  aA[]    poly inum, "noise", 0.5, 0.3
  aAmps[] poly inum, "oscili", 1, kLfoFreqs*0.6
  aA *= aAmps

  ; filter noise with bandpass 
  kQ linsegb 0.5, p3*0.5, 0.02, p3, 0.0001
  kBands[] = kFreqs * kQ
  aB[] poly inum, "resonr", aA, kFreqs, kBands
  
  ; panning. poly works also with k-rate and with 
  ; opcodes producing multiple outputs, like pan2
  kPanPos[] poly inum, "lfo", 0.5, kLfoFreqs
  kPanPos += 0.5  ; lfo in the range 0-1 for panning
  aL[], aR[] poly inum, "pan2", aB, kPanPos
  
  aleft  sumarray aL 
  aright sumarray aR

  ; compress / fade
  aref init 1 
  asig = 0.707 * (aleft + aright)
  again compress2 aref, asig, -90, -48, -24, 2.5, 0.05, 0.2, 0.05
  aleft *= again 
  aright *= again 

  again2 compress2 aref, (aleft+aright)*0.707, -90, -6, -3, 20, 0.002, 0.010, 0.02
  aenv = again2 * cossegr:a(0, 1, 1, 0.1, 0)
  outs aleft*aenv, aright*aenv
endin

opcode test, a, k
  kfreq xin 
  aout oscili 0.1, kfreq 
  xout aout 
endop 

instr 3
  ; test udo
  kfreqs[] fillarray 440, 443 
  aA[] poly 2, "test", kfreqs 
  a0 sumarray aA
  outs a0, a0 
endin 
      
</CsInstruments>
<CsScore>

; i 1 0 8
; i 2 9 50
i 3 0 1

</CsScore>
</CsoundSynthesizer>

```


## See also

* `maparray`
* [polyseq](polyseq)

## Credits

Eduardo Moguillansky, 2019
