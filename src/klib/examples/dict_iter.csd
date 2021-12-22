<CsoundSynthesizer>
<CsOptions>
--nosound 
</CsOptions>
<CsInstruments>

/*

  # Example file for dict_iter opcode 

*/
  
instr 1
  /*
  
  dict_iter
  
  xkey, xvalue, kidx dict_iter ihandle [,kreset=1]
  
  kidx: holds the number of pairs yielded since last reset. It 
        is set to -1 when iteration has stopped 
        (in this case, xkey and xvalue are invalid and should not
        be used)
  kreset = 0  -> after iterating over all pairs iteration stops
                 In this mode, iteration happens at most once
           1  -> iteration starts over every k-cycle
           2  -> iteration restarts after stopping   
  */
  
  idict dict_new "str:float", 0, "foo", 1, "bar", 2, "baz", 15, "bee", 9

  kt timeinstk
  ; iterate with a while loop
  kidx = 0
  while kidx < dict_size(idict) - 1 do 
    Skey, kvalue, kidx dict_iter idict 
    printf "while) %s -> %f \n", kidx+kt*1000, Skey, kvalue
  od   

  ; the same but with goto
loop:
  Skey, kvalue, kidx dict_iter idict
  if kidx == -1 goto break
  printf "loop) %s -> %f \n", kidx+kt*1000, Skey, kvalue
  kgoto loop
break:

endin


</CsInstruments>
<CsScore>
i 1 0 0.05

</CsScore>
</CsoundSynthesizer>
