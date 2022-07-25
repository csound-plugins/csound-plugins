# beadsynt

## Abstract

Band-Enhanced Oscillator-Bank


## Description

Band-Enhanced oscillator-bank, a port of Loris' oscillator (based on
Supercollider's Beadsynt). Can work both with arrays and tables. The
band-enhanced family of opcodes (beosc, beadsynt) implement sound
modeling and synthesis that preserves the elegance and malleability of
a sinusoidal model, while accommodating sounds with noisy
(non-sinusoidal) components. Analysis is done offline, with an
enhanced McAulay-Quatieri (MQ) style analysis that extracts bandwidth
information in addition to the sinusoidal parameters for each
partial. To produce noisy components, we synthesize with sine wave
oscillators that have been modified to allow the introduction of
variable bandwidth.

The synthesis can be controlled via a set of flags (see iflags),
allowing to switch between unifrom or gaussian noise for the noise
components, wavetable interpolation (switch off to save cpu), and
freq. interpolation between k-cycles (switch off to save cpu)

!!! note

    `kFreqs[]`, `kAmps[]` and `kBws[]` must all be the same size 
    (this also holds true for `ifreqft`, `iampft` and `ibwft`) 

### Flags

The **iflags** value controls a series of behaviours. It controls the
noise shape used for the residual spectrum (uniform or gaussian
noise); whether to use linear interpolation in the oscillator; and if
the frequency value of an oscillator is interpolated within a
performance pass (relevant if frequencies are changing fast and ksmps
is high)



| iflags    | Description                                                |
|:--------- |:---------------------------------------------------------- |
| +1        | Uniform noise / Gaussian noise                             |
| +2        | Fast (no interpolation) oscillator / Linear interpolation  |
| +4        | No frequency interpolation / Frequency interpolation       |


------


## Syntax


```csound

aout beadsynt kFreqs[], kAmps[], kBws[], inumosc, iflags=1, kfreq=1, kbw=1, ifn=-1, iphs=-1
aout beadsynt ifreqft, iampft, ibwft, inumosc, iflags=1, kfreq=1, kbw=1, ifn=-1, iphs=-1


```
    
## Arguments

* **kFreqs[]**: An array holding the frequencies of each partial 
* **kAmps[]**: An array holding the amplitudes of each partial 
* **kBws[]**: An array holding the bandwidths of each partial 
* **kfreq**: Freq. scaling, all frequencies are multiplied by this (default = 1) 
* **kbw**: Bandwidth scaling, all bandwidths are multiplied by this (default = 1)
* **inumosc**: the number of partials to resynthesize. In the array case, it can be left unset.
* **iflags**: flags controlling oscillator quality, noise type and frequency interpolation. See table
* **ibwft**: A table holding the bandwidths for each partial
* **iampft**: A table holding the amplitudes for each partial
* **ifreqft**: A table holding the frequencies for each partial
* **ifn**: A table holding a sine wave (or -1 to use the builtin table) (default = -1)
* **iphs**: Initial phase. -1: randomized, 0-1: initial phase, >1: table number holding the phases (default = -1)

## Output

* **aout**: The generated sound


## Execution Time

* Performance

## Examples


```csound

<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

/*

This example uses the analysis file fox.mtx which was produced with 
loristrck_pack, see https://github.com/gesellkammer/loristrck
The file is in fact a wav file, with the difference that the wav format is used 
as a binary exchange format

*/

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

gispectrum ftgen 0, 0, 0, -1, "fox.mtx", 0, 0, 0

instr 1
  ifn = gispectrum
  iskip      tab_i 0, ifn
  inumrows   tab_i 1, ifn
  inumcols   tab_i 2, ifn
  it0 = tab_i(iskip, ifn)
  it1 = tab_i(iskip+inumcols, ifn)
  idt = it1 - it0
  inumpartials = (inumcols-1) / 3 
  imaxrow = inumrows - 2
  it = ksmps / sr
  igain init 1
  ispeed init 0.3
  idur = imaxrow * idt / ispeed
  kGains[] init inumpartials
  kfilter init 0
  ifreqscale init 1
  
  kt timeinsts
  kplayhead = phasor:k(ispeed/idur)*idur
  krow = kplayhead / idt
  ; each row has the format frametime, freq0, amp0, bandwidth0, freq1, amp1, bandwidth1, ...
  kF[] getrowlin krow, ifn, inumcols, iskip, 1, 0, 3
  kA[] getrowlin krow, ifn, inumcols, iskip, 2, 0, 3
  kB[] getrowlin krow, ifn, inumcols, iskip, 3, 0, 3

  if(kt > idur*0.6) then
    if metro(0) == 1 then
      println "Applying filter: bandpass between 1000-1500 Hz"
    endif
    kfilter = 1
  endif
  
  ifilterGain = 3    
  if (kfilter == 1) then
    kGains bpf kF, 990, 0.001, 1000, ifilterGain, 1500, ifilterGain, 1510, 0.01
    kA *= kGains
  endif 
   
  iflags = 7  ; max. quality
  aout beadsynt kF, kA, kB, -1, iflags, ifreqscale
   
  if(kt > idur) then
    event "e", 0, 0, 0
  endif
  aenv cosseg 0, 0.02, igain, idur-0.02-0.1, igain, 0.1, 0
  aout *= aenv
  outs aout, aout
endin

schedule 1, 0, -1

</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>


```


## See also

* [beosc](beosc.md)
* [adsynt2](http://www.csound.com/docs/manual/adsynt2.html)
* [oscili](http://www.csound.com/docs/manual/oscili.html)

## Credits

Eduardo Moguillansky, 2019
