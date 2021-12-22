# ftnew

## Abstract

creates a new table of a given size

## Description

`ftnew` is a shortcut opcode to create a new table of a given size. It
is possible to set the elements to an initial value (default=0). 
It explicitely disallows the user to set the table-number manually. 

## Syntax


```csound

itabnum ftnew isize, [idefault=0]


```
    
## Arguments

* **isize**: The size of the table.
* **idefault**: The initial value for all items in the table (default=0)


## Output

* **itabnum**: the number of the generated f-table

## Execution Time

* Init 

## Examples


```csound

; create a table from an array of any size
ixs[] fillarray 0, 1, 2, 3, 4, 5
itab ftnew lenarray(ixs)
copya2ftab ixs,itab
```


## See also

* [ftgen](https://csound.com/docs/manual/ftgen.html)
* [fillarray](http://www.csound.com/docs/manual/fillarray.html)
* [ftfill](ftfill.md)
* [ftfree](http://www.csound.com/docs/manual/ftfree.html)

## Credits

Eduardo Moguillansky, 2021
