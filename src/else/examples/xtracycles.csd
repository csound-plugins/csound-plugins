<CsoundSynthesizer>
<CsOptions>
--nosound
-d
-m0
</CsOptions>

<CsInstruments>

/*

## Example file for xtracycles opcode.

xtracycles returns the number of extra performance cycles 
of an event. An event can extend its scheduled duration via
two mechanisms - either through opcodes like `linsegr`, which
have a release segment, or explicitely through `xtratim`, which
extends the duration of an event by a given time. 
`xtracycles` should be called after all other duration extending
opcodes (like linsegr or xtratim). It works only at init.

NB: to calculate the extra time for an event, divide the number of
cycles by kr (extratime = xtracycles() / kr)

## Syntax
  
icycles xtracycles
  
## Returns
  
icycles - the number of extra performance cycles for this event 

*/
  
sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

instr 1
  aenv linsegr 0, 0.1, 1, 0.5, 0
  iextratime = xtracycles() / kr
  prints "\n >>> extra time = %f (should be higher than 0.5) \n\n", iextratime
endin

</CsInstruments>

<CsScore>

i1 0 3

</CsScore>
</CsoundSynthesizer>
