# throwerror 

## Abstract

Throws an error message at performance or init

## Description

Use throwerror to stop the current event with a performance error. Alternatively 
throwerror can just issue a warning.

Without a type, a performance error is thrown. 

## Syntax

    throwerror Smessage
    throwerror Stype, Smessage
    
### Arguments

* `Stype`: one of "error", "init", "warning", "info". If absent, type defaults to "error"
* `Smessage`: the text message to show. Use sprintf / sprintfk to construct a message if needed

### Output

### Execution Time

* Init (if Stype == "init")
* Performance (otherwise)

## Examples

```csound

if kmidi > 128 then
    throwerror "Received an invalid midi value, stopping current event"
endif

```

## See also

## Credits

Eduardo Moguillansky, 2019
