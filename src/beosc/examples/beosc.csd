<CsoundSynthesizer>
<CsOptions>
-odac     ;;;realtime audio out
</CsOptions>
<CsInstruments>

/*

This is the example file for beosc

beosc
=====

Band-Enhanced oscillator, a port of Loris's oscillator
(based on Supercollider's BEOsc)

The band-enhanced family of opcodes (beosc, beadsynt) implement
sound modeling and synthesis that preserves the elegance and
malleability of a sinusoidal model, while accommodating sounds
with noisy (non-sinusoidal) components. Analysis is done offline,
with an enhanced McAulay-Quatieri (MQ) style analysis that extracts
bandwidth information in addition to the sinusoidal parameters for
each partial. To produce noisy components, we synthesize with sine
wave oscillators that have been modified to allow the introduction
of variable bandwidth.

aout beosc xfreq, kbw, ifn=-1, iphs=0, inoisetype=0

aout: the generated sound
afreq / kfreq: the frequency of the oscillator
kbw: the bandwidth of the oscillator. 0=pure sinusoidal
ifn: a table holding a sine wave (use -1 for the builtin wave used for oscili)
iphs: the phase of the sine (use unirand:i(6.28) to produce a random phase)
inoisetype: in the original implementation, gaussian (normal) noise is used,
            in supercollider's port, a simple uniform noise is used.
            We implement both. 0=uniform, 1=normal

There is no control for amplitude. The user is supossed to just multiply
the output aout by any factor needed.

NB: watch the output of this example with a freq. scope
*/

sr = 44100
ksmps = 64
nchnls = 1
0dbfs  = 1

instr 1
  ifreq = 440
  kt = timeinsts()
  imaxbw = 0.75
  kfreq bpf kt, 0, ifreq, 5, ifreq,  10, ifreq*4, 15, ifreq
  kbw   bpfcos kt, 0, 0,     5, imaxbw, 10, imaxbw,  15, 0, 18, 0, 20, 1
  ;          freq   bw   fn  phs              noisetype(0=uniform)
  aout  beosc kfreq, kbw^2, -1, 0, 2
  kamp bpf kbw, 0, 1, 0.1, 0.9, 1, 0.25
  aenv  cosseg 0, 0.2, 1, p3-0.4, 1, 0.15, 0
  aenv *= interp(kamp*0.2)
  aout *= aenv
  outch 1, aout
endin

</CsInstruments>
<CsScore>
i 1 0 20
</CsScore>
</CsoundSynthesizer> 
