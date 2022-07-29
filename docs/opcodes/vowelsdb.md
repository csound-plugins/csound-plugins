# vowelsdb

## Abstract

A database of vowel sounds


## Description

`vowelsdb` provides a list of speaker definitions with formant frequencies,
bandwidths and amplitudes for the first 5 formants. It includes all the
speaker definitions found in the csound manual plus speakers from
other sources, such as praat and VocalTractLab (vtl).

The user requests formant data from a given speaker and a selection
of vowels and the opcode returns three 2D arrays: frequencies, bandwidths and amplitudes.
Each row contains the information for the corrsponding vowel and has always a 
size of 5, corresponding to the first 5 formants. The number of rows is the same
as the number of vowels queried.

Even if the generated arrays can be k-arrays, the opcode only works at init time 
(similar to how `fillarray` works for k-arrays)

Speakers available:

	* csound-soprano
	* csound-alto
	* csound-countertenor
	* csound-tenor
	* csound-bass
	* vtl-male
	* vtl-female

Vowels (not all speakers define all vowels): a, e, i, o, u, ae, oe, y

## Syntax

```csound
ifreqs[][], ibandwidths[][], iamps[][] vowelsdb Sspeakername, Svowels
kfreqs[][], kbandwidths[][], kamps[][] vowelsdb Sspeakername, Svowels

```

## Arguments

* **Sspeakername**: one of the available speaker names. At thethe x coordinate of the cursor
* **Svowels**: the vowels to retrieve from the database, as a space delimited list (for example: "a e i o u")
	The order is important, since each row in the returned data arrays corresponds to each vowel given, in
	the order given. In the case of "a e i o u", the first row in the `ifreqs` array would hold the
	frequencies of the first five formants of 'a'.

       
## Output

* **ifreqs** / **kfreqs**: a 2D array, where each row holds the frequencies of the first five 
	formants for the corresponding vowel (there are as many rows as vowels in *Svowels*)
* **ibandwidths** / **kbandwidths**: a 2D array were each row holds the bandwidths of the
	first five formants for the corresponding vowel
* **iamps** / **kamps**: a 2D array were each row holds the amplitudes of the
	first five formants for the corresponding vowel (**NB**: these amplitudes
	are already converted to linear amplitudes and **are not in dB**)

  
## See Also

* [presetinterp](presetinterp.md)
* [weightedsum](weightedsum.md)
* [fof2](http://www.csounds.com/manual/html/fof2.html)
* [Csound Formant tables](http://www.csounds.com/manual/html/MiscFormants.html)

## Examples

```csound


<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
	ifreqs[][], ibws[][], iamps[][] vowelsdb "csound-tenor", "a e i o   u"
	printarray ifreqs, "%d", "ifreqs ="
	printarray ibws, "%d", "ibws ="
	printarray iamps, "", "iamps ="
endin

instr 2
	kfreqs[][], kbws[][], kamps[][] vowelsdb "csound-bass", "a e i o   u"
	println "kfreqs ="
	printarray kfreqs
	println "kbws ="
	printarray kbws
	println "kamps ="
	printarray kamps
	turnoff
endin

</CsInstruments>

<CsScore>

i1 0 0.1
i2 0 0.1
; f0 3600

</CsScore>
</CsoundSynthesizer>



```

```
new alloc for instr 1:
ifreqs =
   0: 650 1080 2650 2900 3250
   1: 400 1700 2600 3200 3580
   2: 290 1870 2800 3250 3540
   3: 400 800 2600 2800 3000
   4: 350 600 2700 2900 3300
ibws =
   0: 80 90 120 130 140
   1: 70 80 100 120 120
   2: 40 90 100 120 120
   3: 70 80 100 130 135
   4: 40 60 100 120 120
iamps =
   0: 1.0001 0.5012 0.4467 0.3981 0.0794
   1: 1.0001 0.1995 0.2512 0.1995 0.1000
   2: 1.0001 0.1778 0.1259 0.1000 0.0316
   3: 1.0001 0.3162 0.2512 0.2512 0.0501
   4: 1.0001 0.1000 0.1413 0.1995 0.0501
new alloc for instr 2:
kfreqs =
   0: 600.0000 1040.0000 2250.0000 2450.0000 2750.0000
   1: 400.0000 1620.0000 2400.0000 2800.0000 3100.0000
   2: 250.0000 1750.0000 2600.0000 3050.0000 3340.0000
   3: 400.0000 750.0000 2400.0000 2600.0000 2900.0000
   4: 350.0000 600.0000 2400.0000 2675.0000 2950.0000
kbws =
   0: 60.0000 70.0000 110.0000 120.0000 130.0000
   1: 40.0000 80.0000 100.0000 120.0000 120.0000
   2: 60.0000 90.0000 100.0000 120.0000 120.0000
   3: 40.0000 80.0000 100.0000 120.0000 120.0000
   4: 40.0000 80.0000 100.0000 120.0000 120.0000
kamps =
   0: 1.0001 0.4467 0.3548 0.3548 0.1000
   1: 1.0001 0.2512 0.3548 0.2512 0.1259
   2: 1.0001 0.0316 0.1585 0.0794 0.0398
   3: 1.0001 0.2818 0.0891 0.1000 0.0100
   4: 1.0001 0.1000 0.0251 0.0398 0.0158
```
