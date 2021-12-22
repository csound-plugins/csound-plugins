# sysPlatform

## Abstract

Get a string description of the current system platform

## Description

Possible values depending on the platform:

    * windows
    * linux
    * macos
    * android
    * unix

For unknown platforms, this opcode returns an empty string


## Syntax

    Splatform sysPlatform

## Arguments

## Output

* `Splatform`: a string describing the current platform

## Execution Time

* Init

## Examples

```csound

Splatform sysPlatform
prints "Csound is runnign on platform: %s \n", Splatform

```

## See also

* [pathNative](pathNative.md)

## Credits

Eduardo Moguillansky, 2020
