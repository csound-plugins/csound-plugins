<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

/*
  Example file for cacheset / cacheget

  cacheset and cacheget implement a method to internalize strings,
  similar to strset and strget, but without having to take care about
  setting the indices

  cacheset puts a strin into the cache and returns an idx identifying 
  this string. If a string is put into the cache which already exists,
  we guarantee that the index returned is the same. 

  idx cacheset Sstr          i-time
  kdx cacheset Sstr          k-time

  cacheget retrieves a str previously put in the cache. If the index
  does not point to an existing string, a performance error is raised

  Sstr cacheget idx          i-time
  Sstr cacheget kdx          k-time

  Both opcodes work at both i- and k-time, depending on the arguments
*/

; Use cacheset/get to pass multiple strings between instruments
instr 1  
  event_i "i", 2, 0, -1, cacheset:i("foo"), cacheset:i("bar")
  turnoff
endin

instr 2
  idx1 = p4
  idx2 = p5
  S1 cacheget idx1
  S2 cacheget idx2
  prints "S1=%s   S2=%s \n", S1, S2
  turnoff
endin

</CsInstruments>

<CsScore>

i 1 0 0.1

f 0 1
</CsScore>
</CsoundSynthesizer>