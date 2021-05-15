<CsoundSynthesizer>
<CsOptions>
--nosound

</CsOptions>

<CsInstruments>
/*
    Svalue sfeadmeta Ssndfile, Skey

    Read metadata from sndfile. Skey can be one of "comment", "title",
    "artist", "album", "tracknumber". If the given key is not present
    an empty value is returned. 
    
*/

instr 1
    Scomment sfreadmeta "sfwithmeta.flac", "comment"
    prints "Comment: %s\n", Scomment
    Stitle sfreadmeta "sfwithmeta.flac", "artist"
    prints "Artist: %s\n", Stitle
    turnoff
endin

instr 2
    Skeys[], Svalues[] sfreadmeta "sfwithmeta.flac"
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
