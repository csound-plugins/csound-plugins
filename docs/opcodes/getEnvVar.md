# getEnvVar

## Abstract

Get the value of an environment variable


## Description

getEnvVar first checks csound own environment and if the variable
is not defined checks the global environment. If the variable is not
defined, returns an empty string

!!! Note

    Csound updates its environment when passed flags like `--env:NAME=value` or `--env:NAME+=value`.
    Also csound adds the directory of an orchestra loaded at start time to its own path variables,
    like SSDIR, SADIR, etc. **This opcode will reflect those changes**


## Syntax

    Svalue getEnvVar Svarname 
        
## Arguments

* `Svarname': The name of the env variable, something like "SSDIR" or "USER" 


## Output

* `Svalue`: the value of the variable, if defined (an empty string otherwise)

## Execution Time

* Init 

## Examples

```csound

; Get the value of the HOME env variable
Shome getEnvVar "HOME"

; and the actual value of INCDIR
Sincdir getEnvVar "INCDIR"

prints "HOME: %s, INCDIR: %s \n", Shome, Sincdir

```

## See also

* [system](https://csound.com/docs/manual/system.html)


## Credits

Eduardo Moguillansky, 2020
