# uniqinstance

## Abstract

Modify pfield values of an active instrument instance

## Description

Given an integer instrument number, `uniqinstance` a fractional 
instrument number which is not active now and can be used as p1
for `event`, `schedule` or similar opcodes to create a unique 
instance of the given instrument


!!! Note

    This opcode **DOES NOT** create a new instance. It just returns
    an instr number which can be used to create one
    

## Syntax

    instrnum  uniqinstrance integer_instrnum
    
### Arguments

* `integer_instrnum`: the integer instrument number

### Output

* `instrnum`: a fractional instrument number which is guaranteed
  not to be active at the moment

### Execution Time

* Init (if index and value are i-values)
* Performance (if either index or value are k-variables)

## Examples

```csound 

<CsoundSynthesizer>
<CsOptions>

--nosound
-m0

</CsOptions>

<CsInstruments>

/*
    Example file for uniqinstance

    instrnum  uniqinstance intinstr

    Returns a unique fractional instrument number which is not
    in active at the moment.
    
*/

instr 1
	kcounter init 0
	ktrig metro 20
	if ktrig == 1 then
		kcounter += 1
		kinstr = 10 + kcounter/1000
		printk2 kinstr
		event "i", kinstr, 0, 1
	endif
endin

instr 2
	instrnum10 uniqinstance 10
	printf "Unique instance of instr 10: %f\n", 1, instrnum10
	instrnum11 uniqinstance 11
	printf "Unique instance of instr 11: %f\n", 1, instrnum11
	turnoff
endin

instr 10
    print p1
endin

instr 11
    print p1
endin

</CsInstruments>

<CsScore>
i 10.150 0 0.1
i 11 0 2
i 1 0 0.5
i 2 0.5 0.1

</CsScore>
</CsoundSynthesizer>


```


## See also

* [pread](pread.md)
* [pset](https://csound.com/docs/manual/pset.html)
* [p](https://csound.com/docs/manual/p.html)
* [passign](https://csound.com/docs/manual/passign.html)

## Credits

Eduardo Moguillansky, 2019
