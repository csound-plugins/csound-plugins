# cachepop

## Abstract

Get a cached string and remove it from the cache


## Description

The cache- opcodes implement a global string cache. This can be useful to
pass multiple strings between instruments using the `event` opcode. This is
similar to the `strset` and `strget` opcodes but automatically asigns an idx
to each distints string inside the cache (we guarantee that passing twice 
the same string will return the same index). `cachepop` is similar to [cacheget](cacheget.md)
but after retrievin the value from the cache it is removed. This can be useful 
for cases where one-of strings are passed between instruments and do not need
to survive the current note

## Syntax

    Sstr cachepop idx


### Arguments

* `idx`: the numeric id representing the string, as returned by `cacheset`

### Output

* `Sstr`: the string inside the cache, corresponding to `idx`


### Execution Time

* Init 

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>

<CsInstruments>

/*
  Example file for cacheset / cacheget / cachepop

  cacheset and cacheget implement a method to internalize strings,
  similar to strset and strget, but without having to take care about
  setting the indices. cachepop is similar to cacheget but removes the
  entry from the cache.

  cacheset puts a strin into the cache and returns an idx identifying 
  this string. If a string is put into the cache which already exists,
  we guarantee that the index returned is the same. 

 */

; Use cacheset/pop to pass multiple strings between instruments
instr 1  
  event_i "i", 2, 0, -1, cacheset:i("foo"), cacheset:i("bar")
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

```


## See also

* [cacheset](cacheset.md)
* [cacheget](cacheget.md)


## Credits

Eduardo Moguillansky, 2019
