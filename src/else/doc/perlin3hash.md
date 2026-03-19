# perlin3hash

## Abstract

gradient noise sound generator, drop-in replacement for perlin3

## Description

Perlin noise is a type of gradient noise devised by Ken Perlin, and commonly used to add texture
to objects rendered in computer graphics. It is deterministic: for a given input co-ordinate it
always returns the same output value. To make audible noise you will typically need to define
some trajectory through the co-ordinate space.

`perlin3` is a fresh implementation using hashes to calculate the noise pattern


## Syntax

```csound

aout  perlin3hash ax, ay, az
kout  perlin3hash kx, ky, kz
```

## Arguments

* `kx`, `ky`, `kz`: the coordinates of a point in 3D space. In the current implementation the space wraps at 255 so any value is actually possible.

## Output

* `aout` / `kout`: the perlin noise corresponding to the given coordinates

## Execution Time

* Performance

## Examples

```csound


<CsoundSynthesizer>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr perlin3
  kspeed = line:k(0.1, 10, 2)
  ax = accum:a(kspeed/sr)
  az = ax*0.5
  aper1 = perlin3hash(ax, a(0), az)
  aper2 = perlin3hash(a(0), ax, az)
  asig = pinker() * 0.4
  
  ; remap to 0-1
  aper1 = (aper1 + 1) * 0.5
  aper2 = (aper2 + 1) * 0.5
  
  ilagtime = 0.1
  a1 = asig*lag(aper1, ilagtime)
  a2 = asig*lag(aper2, ilagtime)
  a1, a2 reverbsc a1, a2, 0.92, 12000
  outch 1, a1, 2, a2
  
endin
  

</CsInstruments>

<CsScore>
i "perlin3" 0 300


</CsScore>
</CsoundSynthesizer>




```


## See also

* [Supercollider's Perlin3](https://doc.sccode.org/Classes/Perlin3.html)
* [perlin3](perlin3.md)

## Credits

Eduardo Moguillansky, 2026
