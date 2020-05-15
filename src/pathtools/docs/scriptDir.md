# scriptDir

## Abstract

Get the directory of the loaded orc/csd file

## Description

Get the directory of the loaded script (orc/csd). This is not necessarily
the same as the current working directory (which can be queried via the opcode
[pwd](https://csound.com/docs/manual/pwd.html)). The `pwd` points at the directory
from which csound was launched, the script directory is always the directory of
the script being run. For example, if csound is launched as:

    $ /home/foo/> csound subdir/myscript.csd

The current working dir is `/home/foo`, whereas the script directory
is `/home/foo/subdir`

This is useful when communicating to another process which is not aware of
csound's environment and needs an absolute path to some file relative to
the script being run

## Syntax

    Spath scriptDir

## Arguments

## Output

* `Spath`: the path of the loaded script

## Execution Time

* Init

## Examples

```csound

Spath scriptDir
prints "The script is being run from this folder: %s \n", Spath

```

## See also

* [pathAbsolute](pathAbsolute.md)
* [findFileInPath](findFileInPath.md)
* [pwd](https://csound.com/docs/manual/pwd.html)

## Credits

Eduardo Moguillansky, 2020
