# linexp

## Abstract

linear to exponentail interpolation

## Description

Maps a range of values to another range of values following an exponential curve

## Syntax

```csound

iy  linexp  ix, iexp, ix0, ix1, iy0, iy1
ky  linexp  kx, kexp, ix0, ix1, iy0, iy1

```

### Arguments

* `kx`: input value
* `kx0`: lower bound of x
* `kx1`: upper bound of x
* `ky0`: lower bound of y
* `ky1`: upper bound of y

### Output

* `ky`: the target value

### Execution Time

* Input
* Performance

## Examples

```csound


<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

; Example file for linexp.csd

/*

linexp

linear to exponential interpolation between two ranges

ky    linexp kx, kexp, ky0, ky1, kx0=0, kx1=1

*/

; Map a value within the range 1-3 to the range 0-10 with an exponent of 2.
instr 1
  kx line 1, p3, 3
  ky linexp kx, 2, 1, 3, 0, 10
  printks "kx: %f   ky: %f \n", 1/kr, kx, ky
endin


</CsInstruments>
<CsScore>
i 1 0   0.2

</CsScore>
</CsoundSynthesizer>



```


## See also

* [linlin](https://csound.com/docs/manual/linlin.html)
* [lincos](https://csound.com/docs/manual/lincos.html)
* [bpf](https://csound.com/docs/manual/bpf.html)

## Credits

Eduardo Moguillansky, 2019

(idea based on pd/else's `envgen` and supercollider's `envgen`)
