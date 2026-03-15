# risset

## Abstract

Get information from risset's package manager

## Description

Provides information to interact with the risset package manager, allowing to, for example,
to query the directory where risset places downloaded assets. This allows to use these assets
along different platforms without changing the csound script

## Syntax

    Sout risset Scommand

## Arguments

* **Scommand**: possible commands are:
	* `"root"`: Returns risset's root folder, where assets, documentation and repository clones are
		placed. For linux this is `~/.local/share/risset`, for macOS, `~/Library/Application Support/risset`
		and for windows `%LocalAppData%/risset` (`C:/Users/<User>/AppData/Local/risset`)
	* `"assets"`: The assets folder (a subfolder of risset's root)

## Output

* **Sout**: the result of the query

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
    Sout risset Scommand

    Query the risset package manager

    Possible commands:
    * root -- risset's root folder
    * assets -- where risset places extra assets

*/

instr 1
    Sroot risset "root"
	prints "risset root: %s\n", Sroot
    
    Sassets risset "assets"
    prints "risset assets: %s\n", Sassets
    turnoff
endin

</CsInstruments>

<CsScore>

i1 0 0.1
; f0 3600

</CsScore>
</CsoundSynthesizer>



```

## See also

* [sysPlatform](sysPlatform.md)

## Credits

Eduardo Moguillansky, 2021
