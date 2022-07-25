# initerror 

## Abstract

Throws an error message at init

## Description

Use initerror to stop the current event with an init error

## Syntax

    initerror Smessage
    
### Arguments

* `Smessage`: the text message to show. Use sprintf to construct a message if needed

### Output

### Execution Time

* Init

## Examples

```csound

if imidi > 128 then
    initerror sprintf("Received an invalid midi value (%d)", imidi)
endif

```

## See also

* [throwerror](throwerror.md)

## Credits

Eduardo Moguillansky, 2020
