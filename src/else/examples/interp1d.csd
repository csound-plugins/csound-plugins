<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>

<CsInstruments>

/*

Abstract
========

Interpolate between elements of an array/table

Syntax
======

    kout   interp1d kidx, xarr[], Smode="linear"
    aout   interp1d aidx, xarr[], Smode="linear"
    iout   interp1d iidx, iarr[], Smode="linear"
    kout[] interp1d kidx[], xarr[], Smode="linear"
    iout[] interp1d iidx[], iarr[], Smode="linear"

    kout   interp1d kidx, ktab, Smode="linear", kstep=1, koffset=0
    aout   interp1d aidx, ktab, Smode="linear", kstep=1, koffset=0
    iout   interp1d iidx, itab, Smode="linear", kstep=1, koffset=0
    kout[] interp1d kidx[], ktab, Smode="linear", kstep=1, koffset=0
    iout[] interp1d iidx[], ktab, Smode="linear", kstep=1, koffset=0
        
    **NB**: mode can also be given as a number (see below)
    **NB2**: interp1d performs the opossite operation of `bisect`

Args
====

    idx: the index into the array/table. For example, using linear interpolation (see mode)
        an index of 1.5 will interpolate halfway between arr[1] and arr[2] 
    arr: the array (1D) holding the data. 
    tab: the table holding the data
    mode: the interpolation mode. "linear", "cos", "floor", "cubic" or "exp=XX" where XX 
        stands for the exponential desired. The mode can also be given as k-value, where
        0:linear, -1:cosing, -2:floor, -3: cubic, mode>0: exponential with
        the value as exponent. All modes except cubic perform piecewise interpolation. cubic 
        interpolation needs at least 4 elements. 
        
Output
======

    out: the result of interpolating the array/table at the given index.  

See Also
========

bisect, bpf, linlin, getrowlin, linenv
*/

ksmps = 64
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
    kmidi1 interp1d kidx1, itime2midi1, -1, 2, 1
    kmidi2 interp1d kidx2, itime2midi2, -1, 2, 1
        
    a0 squinewave a(mtof(kmidi1)), a(0.1), a(0.1)
    a1 squinewave a(mtof(kmidi2)), a(0.2), a(0.5)
    igain = 0.1
    ifade = 0.2
    aenv = linseg:a(0, ifade, igain, p3-ifade*2-0.1, igain, ifade, 0)
    outch 1, a0*aenv, 2, a1*aenv
endin

</CsInstruments>

<CsScore>

; Uncomment to perform each example

; i "example1" 0 1
i "example2" 0 10
;i "example3"  0 7


</CsScore>
</CsoundSynthesizer>
