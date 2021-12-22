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
