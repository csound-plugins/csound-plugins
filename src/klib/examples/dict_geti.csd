<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>
<CsInstruments>

/*

  # Example file for dict_geti

  ## dict_geti

    Svalue dict_geti idict, Skey

    
  Get the value at a given key at init time. An empty string
  is returned when the key is not found.
    
*/

ksmps = 64
nchnls = 2
0dbfs = 1

instr 1
  ; create a local dict, mapping strings to numbers
  idict dict_new "sa", "foo", "foovalue", "bar", 10

  Sfoo dict_geti idict, "foo"
  prints "Soo: %s \n", Sfoo
  
  turnoff
endin


</CsInstruments>
<CsScore>
i 1 0 0.01
f 0 1
</CsScore>
</CsoundSynthesizer>