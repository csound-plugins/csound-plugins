# lfnoise

## Abstract

low frequency, band-limited noise

## Description

Generates random values at a rate given by the nearest integer division of the sample rate by the freq argument. If kinterp==0, between generated values 0 is output. Otherwise the output is the result
of interpolating between the generated values. Output is always band limited.


## Syntax


    aout lfnoise krate, kinterp=0

    
### Arguments

* `krate`: the frequency to generate new values
* `kinterp`: if 1, the output is the result of linear interpolation between the
generated values

### Output

* `aout`: if kinterp==0, then this is the output random values at the given frequency, or 0.
If kinterp==1, then output is the result of interpolation between two generated values.  

### Execution Time

* Performance

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
-odac           
   
</CsOptions>

<CsInstruments>

/*
    Example file for lfnoise

    lfnoise generates a random value between 0-1 at the given
    frequency. If kinterp=1, then values are interpolated; otherwise,
    they are held until next value

*/

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

FLpanel "lfnoise", 400, 200, 50, 50
	idisp1 FLvalue "", 50, 30, 322, 20
	FLcolor 150, 100, 150, 200, 100, 250
	gkfreq,   ih1 FLslider "freq",   0, 200, 0, 3, idisp1, 300, 30, 20, 20
	gkinterp, ih2 FLbutton "interpolate", 1, 0,   3, 100, 50, 20, 80, -1
	gkgain,   ih3 FLslider "gain",   0, 1,   0, 3, -1,     300, 30, 20, 140
FLpanelEnd
FLrun
FLsetVal_i 8, ih1
FLsetVal_i 0.1, ih3

instr 1
	aout lfnoise gkfreq, gkinterp
    aout *= interp(gkgain)    
	outs aout, aout
endin

</CsInstruments>

<CsScore>
i1 0 100

</CsScore>
</CsoundSynthesizer>


```


## See also

* [dust2](https://csound.com/docs/manual/dust2.html)
* [crackle](crackle.md)

## Credits

Eduardo Moguillansky, 2019

(port of pd/else's `lfnoise`, which is itself a merge of supercollider's `LFNoise0` and `LFNoise1`)
