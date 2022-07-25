# filereadmeta

## Abstract

Read metadata from a soundfile

## Description

Reads metadata from a soundfile via libsndfile. Only a subset of keys, standardized
across different fileformats is supported:

* title
* comment
* artist
* album
* tracknumber
* software

The opcode has two forms: 1) a specific key is queried, in which case
the opcode returns the string for that key, or an empty string if that
key is not present; 2) no key is given and all the metadata is
returned in two string arrays, one with all the keys and second with
their corresponding values.

## Syntax

    Svalue filereadmeta Ssndfile, Skey
    Skeys[], Svalues[] filereadmeta Ssndfile

## Arguments

* **Ssndfile**: the path to a soundfile
* **Skey**: a metadata key to query

## Output

* **Svalue**: the string value for the given metadata key. It will be
  empty if the key is not present
* **Skeys**: the keys present in the metadata
* **Svalues**: the string values corresponding to each key in **Skeys**
    
## Execution Time

* Init

## Examples

```csound


<CsoundSynthesizer>
<CsOptions>
--nosound

</CsOptions>

<CsInstruments>
/*
    Svalue filereadmeta Ssndfile, Skey

    Read metadata from sndfile. Skey can be one of "comment", "title",
    "artist", "album", "tracknumber". If the given key is not present
    an empty value is returned. 
    
*/

instr 1
    Scomment filereadmeta "sfwithmeta.flac", "comment"
    prints "Comment: %s\n", Scomment
    Stitle filereadmeta "sfwithmeta.flac", "artist"
    prints "Artist: %s\n", Stitle
    turnoff
endin

instr 2
    Skeys[], Svalues[] filereadmeta "sfwithmeta.flac"
    i0 = 0
    while i0 < lenarray(Skeys) do
      prints "%s = %s\n", Skeys[i0], Svalues[i0]
      i0 += 1
    od
endin

</CsInstruments>

<CsScore>

i1 0 0.1
i2 0 0.1
</CsScore>
</CsoundSynthesizer>



```

## See also

* [filesr](https://csound.com/docs/manual/filesr.html)
* [filenchnls](https://csound.com/docs/manual/filenchnls.html)
* [filenlen](https://csound.com/docs/manual/filelen.html)
* [pathAbsolute](pathAbsolute.md)
* [findFileInPath](findFileInPath.md)

## Credits

Eduardo Moguillansky, 2021
