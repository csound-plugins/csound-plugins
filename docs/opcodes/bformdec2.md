# bformdec2

## Abstract

Ambisonics decoding with dual-band decoding and near-field compensation


## Description

This opcode performs ambisonics decoding with near–field compensation
and uses a different decoding matrix for low and high frequencies. It
also adds some features, such as additional loudspeaker array con-
figurations (rectangle, hexagon) and a binaural output for headphones.

### Ambisonics

Unlike traditional multichannel audio for spatialization in which each
chan- nel corresponds to a given loudspeaker, the Ambisonics sound
format (called B-format) contains a speaker-independent representation
of a sound field. By means of an appropriate decoder, i.e. matched to
the geometry of the loudspeaker array, this sound file can be played
back in different speaker layouts.

#### Dual-Band decoding

Given that no decoding approach is adequate for both high and low
frequencies, many Ambisonics decoders split the B-format signals into
(at least) two bands and use independent solutions for low and high
frequencies. Then the output of each band is recombined to produce
the audio signals for the loudspeakers. When
combining the output of each band, different criteria can be used to
deal with the signal’s level difference between the low and high
frequencies, such as preserving the amplitude, the root–mean–square
RMS level or the total energy.

#### Near–field compensation

Another important aspect of an Ambisonics decoder is to provide near–field
compensation. The recreation of the sound field at the central position holds
under the hypothesis that the wavefronts are planar. Given the finite distance to
the loudspeakers, the sound wavefronts at the listening position present instead
a curvature, which produces a bass–boosting effect that has to be compensated.
The compensation is essentially a high–pass filter, which depends on the order of
reproduction and on the distance of the loudspeakers to the center of the array.

!!! Note

    To use HRTF files, as in the examplee, make sure that the HRTF files
    are in the same directory, or that the SADIR env variable is set and
    the HRTF files are places inside it. 

### Supported setups

There are eight supported setups:

* **1** - *Stereo*: L(90), R(-90); this is an M+S style stereo decode.
* **2** - *Quad*: FL(45), BL(135), BR(-135), FR(-45). This is a first-order decode.
* **3** - *5.0*: L(30), R(-30), C(0), BL(110), BR(-110). Note that many people do not actually use the angles above for their speaker arrays and a good decode for DVD etc can be achieved using the Quad configuration to feed L, R, BL and BR (leaving C silent).
* **4** - *Octagon*: FFL(22.5), FLL(67.5), BLL(112.5), BBL(157.5), BBR(-157.5), BRR(-112.5), FRR(-67.5), FFR(-22.5). This is a first-, second- or third-order decode, depending on the number of input channels.
* **5** - *Cube*: FLD(45,-35.26), FLU(45,35.26), BLD(135,-35.26), BLU(135,35.26), BRD(-135,-35.26), BRU(-135,35.26), FRD(-45,-35.26), FRU(-45,35.26). This is a first-order decode.
* **6** - *Hexagon*: FL(30), L(90) BL(150), BR(-150), R(-90), FR(-30). This is a first- or second- order decode.
* **21** - *2D binaural configuration*. This first decodes to a octagon configuration and then applies HRTF filters.
* **31** - *3D binaural configuration*. This first decodes to a dodecahedron configuration and then applies HRTF filters.


## Syntax


```csound
aout[] bformdec2 isetup, abform[], [idecoder=0, idistance=1, ifreq=400, imix=0, ifilel, ifiler]
```
    
## Arguments

* **isetup**: loudspeaker setup. One o 1 (Stereo), 2 (Quad), 3 (5.0),
  4 (Octagon), 5 (Cube), 6 (Hexagon), 21 (2D binaural), 31 (3D
  binaural)
* **abform[]**: input signal array in B-format
* **idecoder**: decoder type. 0: Dual decoder (velocity and energy decoders using dual-band splitting), 1: Velocity decoder, 2: Energy decoder
* **idistance**: select the distance (in meters) to the loudspeakers (radius in regular configuration). Default=1m
* **ifreq**: cut frequenc of the band splitting filter (only has an effect if `idecoder` is 0), default=400Hz 
* **imix**: type of mix o the velocity and energy decoders' outputs (0: energy, 1: RMS, 2: Amplitude)
* **ifilel**: left HRTF spectral data file
* **ifiler**: right HRTF spectral data file

!!! note

    Spectral datailes (based on the MIT HRTF database) shoud be in the current 
    directory, in the SADIR (see 
    [hrtfstat documentation](http://www.csounds.com/manual/html/hrtfstat.html)

## Output

* **aout[]**: loudspeaker specific output signals

## Execution Time

* Performance

## Examples

To run the example, make sure that the HRTF files are in the same directory, or that
the SADIR env variable is set and the HRTF files are places inside it

    csound test_binaural.csd --env:SADIR=<path to hrtf folder>

```csound


<CsoundSynthesizer>

<CsOptions>
-odac
</CsOptions>

<CsInstruments>
sr = 48000
ksmps = 64
0dbfs = 1
nchnls = 2

; Ambisonics order
giOrder	=	1

instr 1	

iSetup	init 21 // binaural 2D

; array Ambisonics
iArraySize	=	(giOrder+1)^2
aAmbi[]	init	iArraySize

; output
iOutSize	init	nchnls
aOut[]	init	iOutSize

aAmbi[0],aAmbi[1],aAmbi[2],aAmbi[3]     diskin "AJH_eight-positions.amb"

aOut bformdec2 iSetup,aAmbi,0,1,400,0,"hrtf-48000-left.dat","hrtf-48000-right.dat" //order1

outs aOut[0], aOut[1]

endin 


</CsInstruments>


<CsScore>

i1	0	20

</CsScore>

</CsoundSynthesizer>




```

## See also

* [bformenc](http://www.csound.com/docs/manual/bformenc.html)
* [bformdec](http://www.csound.com/docs/manual/bformdec.html)
* [bformenc1](http://www.csound.com/docs/manual/bformenc1.html)
* [hrtfstat](http://www.csound.com/docs/manual/hrtfstat.html)

## Credits

Pablo Zinemanas, Martín Rocamora and Luis Jure, 2019

## Reference

Pablo Zinemanas, Martín Rocamora and Luis Jure. Improving Csound's Ambisonics decoders. Fifth International Csound Conference – ICSC2019. Italy, 2019

<https://github.com/pzinemanas/bformdec2>
