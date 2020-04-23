<CsoundSynthesizer>
<CsOptions>
--nosound 
</CsOptions>
<CsInstruments>

/*

  Example file for dict_clear

  dict_clear idict
  dict_clear kdict

  Clear the contents of dict, resets it to 0 key/value pairs
    
*/

instr 1
  idict dict_new "int:float", 0, 0, 1, 10, 2, 20, 3, 30
  dict_print idict
  dict_clear idict
  dict_print idict
  turnoff
endin


</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
