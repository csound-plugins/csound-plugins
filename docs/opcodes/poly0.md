# poly0

## Abstract

`poly0` creates and controls multiple parallel version of an opcode with no outputs

## Description

`poly0` creates a user given number of instances of an opcode, each with its own state,
inputs and outputs. The resulting output is an array where each element holds the
output of the corresponding instance.

In general, an opcode has a signature, given by the number and types of its output and
input arguments. For example, the opcode `oscili` as used like `aout oscili kamp, kfreq`
has a signature `a / kk` (`a` as output, `kk` as input).

To follow this example, to create 10 parallel versions of this opcode (an oscillator bank)
it is possible to use `poly0` like this:

```csound

kFreqs[] fillarray 100, 110, 200, 220, 300, 330, 400, 440, 500, 550
iPans[]   fillarray 0,   1,   0,   1,   0.5, 0.3, 0.7, 0.2, 0.9, 0.8
aSigs[] poly 10, "oscili", 0.1, kFreqs
aLs[], aRs[] poly 10, "pan2", aSigs, iPans
poly0 10, "outch", 1, aLs
poly0 10, "outch", 2, aRs

```

Notice that it is possible to set one value for each instance, as given by `kFreqs`, or
one value to be shared by all instances, as given by the amplitude `0.1`. By changing
the array `kFreqs` it is possible to modify the frequency of each oscillator.

It is of course possible to chain multiple `poly0`s to generate complex effect chains,
and `poly0` can also be used with k-values.

!!! warning

    At the moment `poly0` works **only** with **builtin opcodes**. This might change
    in the future


## Syntax


    poly0 inuminstances, Sopcode, xarg0, [xarg1, ...]


## Arguments

* `inuminstances`: the number of instances of `Sopcode` to instantiate
* `Sopcode`: the name of the opcode
* `xargs`: any number of arguments, either i-, k- or a-rate, either scalar or arrays,
           or strings, as needed by the given opcode. String arrays are not yet supported

The number and type of the input arguments depend on the arguments passed to the
given opcode. The same applies for the output arguments

**NB**: output arguments are always arrays, input arguments can be arrays, in which
case they must be at least the size of `inuminstances`, or scalars, in which case
the same value is shared by multiple instances

## Examples

See [poly](poly.md) for examples

## See also

* [poly](poly.md)
* [maparray](http://www.csounds.com/manual/html/maparray.html)
* [polyseq](polyseq.md)

## Credits

Eduardo Moguillansky, 2019
