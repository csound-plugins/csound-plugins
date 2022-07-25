# extendarray

## Abstract

Extend one array with the contents of a second array, in place

## Description

`extendarray` concatenates two arrays, placing the results in the first array. It works either at init or performance time, depending on the arrays passed.

## Syntax

```csound

extendarray iArray1[], iArray2
extendarray kArray1[], kArray2
extendarray kArray1[], iArray2
extendarray SArray1[], SArray2

```

## Arguments

* **xArray1**: the first array and the array where the result is placed
* **xArray2**: the second array

## Output

## Execution Time

* Init (if all arguments are of i type)
* Performance (for k or S types)

## Examples

```csound
iArr1[] fillarray 0, 1, 2
iArr2[] fillarray 3, 4, 5
extendarray iArr1, iArr2
; iArr1 is now [0, 1, 2, 3, 4, 5]
```

## See also

* [setslice](setslice.md)
* [setrow](http://www.csound.com/docs/manual/setrow.html)
* [slicearray](http://www.csound.com/docs/manual/slicearray.html))
* [ftset](http://www.csound.com/docs/manual/ftset.html)
* [ftslice](http://www.csound.com/docs/manual/ftslice.html)

## Credits

Eduardo Moguillansky, 2020
