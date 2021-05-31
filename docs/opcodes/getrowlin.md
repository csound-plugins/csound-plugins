# getrowlin

## Abstract

opy a row from a 2D array or table, with interpolation between rows


## Description

Given a 2D array (i- or k- array), or a table representing a 2D
matrix, get a row of this matrix (possibly a slice). If krow is not an
integer, the values are the result of the interpolation between the
two adjacent rows. Assuming such a 2D matrix containing multiple rows
of sampled streams (for instance, the amplitudes of a set of
oscillators, sampled at a regular interval), this opcode extracts one
row of that data with linear interpolation between adjacent rows (if
the row is not a round number) and places the result in a 1D array

## Syntax


```csound

kOut[] getrowlin kMtx[], krow, kstart=0, kend=0, kstep=1 
kOut[] getrowlin krow, ifn, inumcols, iskip=0, istart=0, iend=0, istep=1

```
    
## Arguments

* **kMtx[]**: a 2D array
* **krow**: the row to read (can be a fractional number, in which case
  interpolation with the next row is performed)
* **kstart**: start index to read from the row (default = 0) 
* **kend**: end index to read from the row (not inclusive) 
* **kstep**: step used to read the along the row (default = 1) 
* **iskip**: in the case of using a table as input, iskip indicates
  the start of the sampled data (skipping a possible header in the
  data) (default = 0)
* **inumcols**: in the case of using a table as input, inumcols
  indicates the number of columns of the 2D matrix.

## Output

* **kOut[]**: the interpolated row


## Execution Time

* Performance

------

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
   
  iflags = 0    ; uniform noise, no interpolation
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

* [tabrowlin](tabrowlin.md)
* [slicearray](http://www.csound.com/docs/manual/adsynt2.html)
* [beadsynt](beadsynt.md)

## Credits

Eduardo Moguillansky, 2019
