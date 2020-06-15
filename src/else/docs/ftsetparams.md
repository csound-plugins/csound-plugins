# ftsetparams

## Abstract

Set metadata parameters of a table, as if it was loaded via GEN1

## Description

`ftsetparams` can be used to set the parameters set via when reading a table via GEN1 (samplerate, number of channels, looping, etc). This are necessary by some opcodes (loscil, for example) to play correctly. Together with `ftslice` it can be used to extract a channel of a multichannel table preserving the table metadata.


## Syntax


```csound
ftsetparams ift, isamplerate, inumchannels, iloopstart=0, ibasenote=60
```

## Arguments

* `ift`: the table number to modify
* `isamplerate`: the sample rate of the data saved in the table
* `inumchannels`: the number of channels of the audio sample
* `iloopstart`: if this is a loop, start of the sustain part
* `ibasenote`: pitch of the sample, as midinote

## Output

## Execution Time

* Init

## Examples

```csound
<CsoundSynthesizer>
<CsOptions>
-odac 
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

gifunc0 ftgen 0, 0, 0, -1, "musicbox.flac", 0, 0, 0

gi1 ftgen 0, 0, nsamp(gifunc0), 2, 0
ftslicei gifunc0, gi1, 0, 0, 2
ftsetparams gi1, ftsr:i(gifunc0), 1

gi2 ftgen 0, 0, nsamp(gifunc0), 2, 0
ftslicei gifunc0, gi2, 1, 0, 2
ftsetparams gi2, ftsr:i(gifunc0), 1
    
instr 1
	a0, a1 loscil 1, 1, gifunc0, 1
	outch 1, a0, 2, a1
endin

instr 2
    a0 loscil 1, 1, gi1, 1
	a1 loscil 1, 1, gi2, 1
    outch 1, a0, 2, a1
endin

; schedule(1, 0, -1)
schedule(2, 0, -1)
</CsInstruments>
<CsScore>

</CsScore>
</CsoundSynthesizer>


```

## See also

* [ftslice](http://www.csounds.com/manual/html/ftslice.html)
* [ftslicei](http://www.csounds.com/manual/html/ftslicei.html)
* [loscil](http://www.csounds.com/manual/html/loscil.html)
* [ftsr](http://www.csounds.com/manual/html/ftsr.html)
* [ftchnls](http://www.csounds.com/manual/html/ftchnls.html)

## Credits

Eduardo Moguillansky, 2020
