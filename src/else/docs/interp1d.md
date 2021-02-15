# interp1d

## Abstract

Interpolate between elements of an array/table

## Description

Given a fractional index into an arra/table, interpolate
between adjacent items. In the case of a table, a specific
column of the table can be selected for performing interpolation.
In this case, the index indicates the "row", the step value determines
the size of each row, and the offset determines which column is used 
when interpolating. Possible interpolation modes are: linear, cos, 
floor, exponential and cubic.
Together with `bisect` it can be used to generate any possible 
breakpoint-function configuration.

**NB**: interp1d performs the opossite operation of `bisect`

**NB2**: when used with a table the `param` value can be given within the string, 
as "exp=1.5" or "smooth=0.7". For example, `kout = interp1d(kidx, itab, "exp=1.5")`

!!! Note

    At the moment the interpolation mode is set at init time and can't be modified

## Syntax

```csound
kout   interp1d kidx, xarr[], Smode="linear", kparam=0
aout   interp1d aidx, xarr[], Smode="linear", kparam=0
iout   interp1d iidx, iarr[], Smode="linear", kparam=0
kout[] interp1d kidx[], xarr[], Smode="linear", kparam=0
iout[] interp1d iidx[], iarr[], Smode="linear", kparam=0

kout   interp1d kidx, ktab, Smode="linear", kstep=1, koffset=0
aout   interp1d aidx, ktab, Smode="linear", kstep=1, koffset=0
iout   interp1d iidx, itab, Smode="linear", kstep=1, koffset=0
kout[] interp1d kidx[], ktab, Smode="linear", kstep=1, koffset=0
iout[] interp1d iidx[], ktab, Smode="linear", kstep=1, koffset=0
```
    
## Arguments

* `idx`: the index into the array/table. For example, using linear interpolation (see mode)
    an index of 1.5 will interpolate halfway between arr[1] and arr[2] 
* `arr`: the array (1D) holding the data. 
* `tab`: the table holding the data
* `mode`: the interpolation mode. Possible interpolations modes are: "linear", "cos", "floor", 
  "cubic", "smooth" (smoothstep, see https://en.wikipedia.org/wiki/Smoothstep), 
  "smoother" (perlin's smootherstep) or "exp" (exponential). "smoother" interpolation is almost
  equal to "smooth" with param=0.7.
* `param`: a parameter used by the interpolation mode. In "exp" mode, param sets the exponent
  (2 will result in a quadratic curve). In "smooth" mode, param sets the number of extra
  smoothsteps (default=0). Fractional smoothsteps are possible (see )
        
## Output

* `out`: the result of interpolating the array/table at the given index.

## Examples

```csound


<CsoundSynthesizer>
<CsOptions>
; -odac
</CsOptions>

<CsInstruments>

/*

Abstract
========

Interpolate between elements of an array/table

Syntax
======

    kout   interp1d kidx, xarr[], Smode="linear", kparam=0
    aout   interp1d aidx, xarr[], Smode="linear", kparam=0
    iout   interp1d iidx, iarr[], Smode="linear", kparam=0
    kout[] interp1d kidx[], xarr[], Smode="linear", kparam=0
    iout[] interp1d iidx[], iarr[], Smode="linear", kparam=0

    kout   interp1d kidx, ktab, Smode="linear", kstep=1, koffset=0
    aout   interp1d aidx, ktab, Smode="linear", kstep=1, koffset=0
    iout   interp1d iidx, itab, Smode="linear", kstep=1, koffset=0
    kout[] interp1d kidx[], ktab, Smode="linear", kstep=1, koffset=0
    iout[] interp1d iidx[], ktab, Smode="linear", kstep=1, koffset=0
        
    **NB**: interp1d performs the opposite operation of `bisect`

See Also
========

bisect, bpf, linlin, getrowlin, linenv
*/

ksmps = 10
nchnls = 2
0dbfs  = 1

instr example1
    ixs[] fillarray 0, 10, 16, 18, 28
    ; interpolate ixs between at index 1.5, interpolating linearly between 
    ; ixs[1] and ixs[2]
    iout interp1d 1.5, ixs
    print iout  ; -> 13.
    
    ; scan ixs at k-rate
    kidx = line:k(0, p3, lenarray:i(ixs)-1)
    kout2 interp1d kidx, ixs
    println "kidx: %f, kout2: %f", kidx, kout2
endin

instr example2
    ; used together with bisect can create multiple piecewise interpolation configurations
    itimes[] fillarray 0,   4,   5,    10
    imidi1[] fillarray 64, 64,   63.5, 64.5
    imidi2[] fillarray 64, 63.4, 63.4, 63
    iamps[]  fillarray 0,   0.8,   0.8,    0

    kidx bisect timeinsts(), itimes
    kamp interp1d kidx, iamps, "cos"
    aamp interp kamp
    a1 oscili aamp, mtof(interp1d(kidx, imidi1, "cubic"))
    a2 oscili aamp, mtof(interp1d(kidx, imidi2))
    println "amp: %f", rms:k(aamp)
    outch 1, a1, 2, a2
endin

instr example3
    ; a table can also be used with interp1d / bisect. A table can hold
    ; both x and y coordinates as pairs
    itime2midi1 ftfill 0, 64, 4, 62, 5, 62, 6, 67
    itime2midi2 ftfill 0, 60, 4, 60, 5, 59, 6, 59
    ftfree itime2midi1, 1
    ftfree itime2midi2, 1
    ; step=2, bisect the column 0.
    kt = timeinsts() 
    kidx1 bisect kt, itime2midi1, 2
    kidx2 bisect kt, itime2midi2, 2
        
    ; -1: cosine interpolation, step size=2, offset=1
    kmidi1 interp1d kidx1, itime2midi1, "cos", 2, 1
    kmidi2 interp1d kidx2, itime2midi2, "cos", 2, 1
        
    a0 squinewave a(mtof(kmidi1)), a(0.1), a(0.1)
    a1 squinewave a(mtof(kmidi2)), a(0.2), a(0.5)
    igain = 0.1
    ifade = 0.2
    aenv = linseg:a(0, ifade, igain, p3-ifade*2-0.1, igain, ifade, 0)
    outch 1, a0*aenv, 2, a1*aenv
endin

instr example4
    ; test all curves, save to csv
    ifile fiopen "interp1d.csv", 0
    fprints ifile, "# kx, kidx, klin, kcos, kfloor, kcub, kexp, ksmooth, ksmooth2, ksmoother\n" 

    ixs[] fillarray 0, 1,  4, 5,  6.4, 8
    iys[] fillarray 0, 10, 2, 20, 3.2, 16
    kx line 0, p3, ixs[lenarray(ixs)-1]
    kidx bisect kx, ixs
    klin = interp1d(kidx, iys, "linear")
    kcos interp1d kidx, iys, "cos"
    kfloor interp1d kidx, iys, "floor"
    kcub = lag(interp1d(kidx, iys, "cubic"), 0.1)
    kexp interp1d kidx, iys, "exp", 2
    ksmooth interp1d kidx, iys, "smooth", 0 
    ksmooth2 interp1d kidx, iys, "smooth", 1
    ksmoother interp1d kidx, iys, "smoother"
        
    ; kx, kidx, klin, kcos, kfloor, kcub, kexp[O[I]]
    fprintks ifile, "%g, %g, %g, %g, %g, %g, %g, %g, %g, %g\n", kx, kidx, klin, kcos, kfloor, kcub, kexp, ksmooth, ksmooth2, ksmoother
endin
</CsInstruments>

<CsScore>

; Uncomment to perform each example

; i "example1" 0 1
; i "example2" 0 10
;i "example3"  0 7
i "example4" 0 2

</CsScore>
</CsoundSynthesizer>



```


## See Also

* [bisect](bisect.md)
* [bpf](http://www.csound.com/docs/manual/html/bpf.html)
* [linlin](http://www.csound.com/docs/manual/html/linlin.html)
* [getrowlin](http://www.csound.com/docs/manual/html/getrowlin.html)
* [linenv](linenv.md)
