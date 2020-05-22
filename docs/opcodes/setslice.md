# setslice

## Abstract

Set a slice of an array to a given value

## Description

`setslice` sets the elements of an array or a slice of it to a given value.
This operation is done in place. It is the equivalent of this code:

```python
array[start:end:step] = value
```

## Syntax


```csound
setslice iArray[], ivalue [, istart=0, iend=0, istep=1]
setslice kArray[], kvalue [, kstart=0, kend=0, kstep=1]


```
    
## Arguments

* `value`: the value to set the elements to
* `start`: the start index of the slice. Defaults to 0
* `end`: the end index of the slice. Defaults to 0 which means until the end of the slice
* `step`: the number of steps to jump between elements. Default to 1 (all elements in the slice)

## Output

## Execution Time

* Init (if all arguments are of i type) 
* Performance (if any argument is of k type)

## Examples

```csound

iA[] fillarray 0, 1, 2, 3, 4, 5, 6, 7
setslice iA, 0.5  ; will set all elements of iA to 0.5
printarray iA

kB[] init 10
; if the condition is met setslice will set the even indexes
; between 0 to 6 of kB to -1 
if kvalue = 1 then
    setslice kB, -1, 0, 6, 2
endif
if changed2(kB) == 1 then
    printarray kB
endif
; kB: -1, 0, -1, 0, -1, 0, 0, 0, 0, 0

```


## See also


* [ftset](http://www.csounds.com/manual/html/ftset.html)
* [ftslice](http://www.csounds.com/manual/html/ftslice.html)
* [slicearray](http://www.csounds.com/manual/html/slicearray.html)


## Credits

Eduardo Moguillansky, 2020
