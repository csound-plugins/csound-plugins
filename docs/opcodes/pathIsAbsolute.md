# pathIsAbsolute

## Abstract

Returns 1 if the path of a file is absolute


## Description

Returns 1 if the path of a file is absolute

## Syntax

    i_isabsolute pathIsAbsolute Spath 
    k_isabsolute pathIsAbsolute Spath
        
## Arguments

* `Spath': The path to a file or directory

## Output

* `i_isabsolute`: 1 if the path is absolute, 0 otherwise

## Execution Time

* Init (if the result is i-type)
* Perf (if the result is k-type)

## Examples

```csound

i_isabsolute pathAbsolute "relative/path/to/file.txt"
prints "Path is absolute: %s \n", i_isabsolute == 1 ? "yes" : "no"

```

## See also

* [pathAbsolute](pathAbsolute.md)
* [scriptDir](scriptDir.md)

## Credits

Eduardo Moguillansky, 2020
