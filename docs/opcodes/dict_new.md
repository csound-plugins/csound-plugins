# dict_new

## Abstract

Create a hashtable

## Description

A hashtable is a mapping from a key to a value. The `dict_` family of opcodes
implement a hashtable mapping either strings or integers to strings or floats.

### Local vs Global

A hashtable can be either **local**, in which case its lifespan is fixed to the lifespan
of the note it was created in; or it can be **global**, in which case **it is not
deallocated** when the note is released and is kept alive until either the end
of the performance, or until freed via [dict_free](dict_free.md)


## Syntax

    idict dict_new Stype
    idict dict_new Stype, key0, value0, key1, value1, ...
    
    kdict dict_new Stype, key0, value0, key1, value1, ...

!!! Note

    With the second variant it is possible to create a dict and give it initial values at the same time.

## Types

The types of a dict are fixed at creation time and are specified via the `Stype` argument. To declare a dict as `global`, prefix the type with a `*`.

| type        | short | as global    | short | key    | value  |
|:------------|:------|--------------|:------|:-------|--------|
| `str:float` | `sf`  | `*str:float` | `*sf` | string | float  |
| `str:str`   | `ss`  | `*str:str`   | `*ss` | string | string |
| `int:float` | `if`  | `*int:float` | `*if` | int    | float  |
| `int:str`   | `is`  | `*int:str`   | `*is` | int    | string |
| `str:any`   | `sa`  | `*str:any`   | `*sa` | string | any    |

### The "any type

A dict of the form `str:any` accepts strings as keys and can have both strings and numbers as values. This can be used to pass arguments to an instrument like

```csound

    iargs dict_new "*sa", "name", "foo", "freq", 1000, "amp", 0.5
    schedule "myinstr", 0, -1, iargs

    ; then, inside myinstr
    instr myinstr
      iargs = p4
      Sname dict_get iargs, "name"
      kfreq dict_get iargs, "freq"
      kamp  dict_get iargs, "amp"
      ; ... do something with this
      dict_free iargs
    endin
```

## Arguments

* `Stype`: a string describing the type of the key and the value. See the table above.
           **NB**: to declare the dict as global, prepend the type string with a `*`.

* `key0`, `value0`, `key1`, `value1`, etc: initial pairs can be set at creation time, matching
              the types declared with `Stype`

### Output

* `idict`: identifies this dict with an integer. This integer can be passed around to another instrument and will always resolve to the same dict, as long as this dict is still alive. This can be checked via `dict_query idict, "exists"`

### Execution Time

* Init (the normal case)
* Performance (use this form when using dicts for passing named arguments to an event created at k-time)


## Examples

```csound

<CsoundSynthesizer>
<CsOptions>
; -odac  -iadc    ;;;RT audio out and in

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1
  ; create a local dict, mapping strings to numbers
  idict dict_new "sf"

  ; set key a key:value pair
  dict_set idict, "bar", 123

  ; retrieve the value
  kbar dict_get idict, "bar"

  ; get a non-existent key, will output the default
  kfoo dict_get idict, "foo", -1

  printf ">>>> bar: %f,  foo: %f \n", 1, kbar, kfoo

  ; now create another dict, this one will outlive this note
  idict2 dict_new "*str:str", "baz", "bazvalue", "foo", "foovalue"

  ; schedule another inst, pass this dict
  event "i", 2, 0, 1, idict2

  turnoff

endin

instr 2
  idict = p4
  Sbaz = dict_get(idict, "baz")
  printf "instr 2, kbaz = %s \n", 1, Sbaz

  ; free dict at the end of this note
  dict_free idict, 1
  turnoff
endin

; schedule 1, 0, 1

</CsInstruments>
<CsScore>

i 1 0 2

</CsScore>
</CsoundSynthesizer>
```

## See also

* [dict_free](dict_free.md)
* [dict_set](dict_set.md)
* [dict_newk](dict_newk.md)


## Credits

Eduardo Moguillansky, 2019
