<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

/*
  Example file for cacheput / cacheget / cachepop

  cacheput and cacheget implement a method to internalize strings,
  similar to strset and strget, but without having to take care about
  setting the indices. cachepop is similar to cacheget but removes the
  entry from the cache.

  cacheput puts a strin into the cache and returns an idx identifying 
  this string. If a string is put into the cache which already exists,
  we guarantee that the index returned is the same. 

 */

; Use cacheput/pop to pass multiple strings between instruments
instr 1  
  event_i "i", 2, 0, -1, cacheput:i("foo"), cacheput:i("bar")
  turnoff
endin

instr 2
  S1 cachepop p4
  S2 cachepop p5
  ; these strings are no longer in the cache
  prints "S1=%s   S2=%s \n", S1, S2
  turnoff
endin

</CsInstruments>

<CsScore>

i 1 0 0.1

f 0 1
</CsScore>
</CsoundSynthesizer>