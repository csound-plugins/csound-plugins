# pathNative

## Abstract

Convert a path to its native version

## Description

Convert a path to a native path by replacing path separators to
the native separators ('/' in unix, '\' in windows)

This is only useful when passing paths to external processes which
need an absolute, native path

**NB**: do not use absolute paths when writing cross-platform paths,
since it is not possible to convert an absolute unix path to an absolute
windows path (because of windows use of drives)

**NB2**: windows already converts any forward slash to backwards slash, so
as long as you use relative paths, there is no need to use this

## Syntax

    Snative pathNative Spath

## Arguments

* `Spath`: the path to convert

## Output

* `Snative`: the native path

## Execution Time

* Init

## Examples

```csound

Spath = "foo/bar/baz.txt"
Snative pathNative Spath
prints "Original: %s, Native version: %s \n", Spath, Snative

; this should print the original in unix, "foo\bar\baz.txt" in windows

```

## See also

* [pathNative](pathNative.md)

## Credits

Eduardo Moguillansky, 2020
