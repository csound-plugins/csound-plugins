# errormsg

## Abstract

Throws an error message at performance

## Description

Use errormsg to stop the current event with a performance error. Alternatively 
errormsg can just issue a warning of debugging information. This uses
the builtin messaging api so the messages will look as if thrown by an internal
opcode. 

Without a type, a performance error is thrown. 

## Syntax

    errormsg Smessage
    errormsg Stype, Smessage
    
### Arguments

* `Stype`: one of "error", "warning", "info". If absent, type defaults to "error"
* `Smessage`: the text message to show. Use sprintfk to construct a message if needed

### Output

### Execution Time

* Performance

## Examples

```csound

if kmidi > 128 then
    errormsg "Received an invalid midi value, stopping current event"
endif

```

## See also

## Credits

Eduardo Moguillansky, 2019
