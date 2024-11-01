# gaintovel

## Abstract

Map amplitude 0-1 to velocity (0-127)


## Description

Given a linear amplitude in the range 0-1, returns a corresponding
velocity following an exponential curve defined by an exponent,
a minimum gain and a minimum velocity. A gain of 0 will always
result in a velocity of 0, any gain between 0 and the min. gain
will be mapped to the min. velocity, and in the range between
min. gain and 1., it will be mapped to min. velocity - 127 following
the given exponential curve. Normally the exponent would be a number
between 0 and 1, but the value depends highly upon the actual
context in which the resulting velocity will be used. In general,
an exponent higher than 1 will result in more sensitivity to higher
gains, an exponent between (0, 1) will result in higher sensitivity to
lower gains.

For reference, the formula used is:

```
relgain = (gain - mingain) / (1. - mingain)  ;; This will be a value between 0 and 1
vel = (relgain ^ exp) * (127 - minvel) + minvel
```


## Syntax

MYFLT *vel;
    MYFLT *gain;
    MYFLT *mingain;
    MYFLT *exp;
    MYFLT *minvel;
    MYFLT *round;


```csound
ivel gaintovel igain, imingain, iexp, iminvel=1, iround=0
kvel gaintovel kgain, kmingain, kexp, kminvel=1, kround=0
```
    
## Arguments

* **kgain**: the amplitude, in the range 0-1
* **kmingain**: the gain (linear amplitude in the range 0-1) corresponding to min. velocity
* **kexp**: the exponent of the mapping curve.
* **kminvel**: min. velocity, corresponding to min. gain. All values between 0 and
  this value will never be assigned
* **kround**: 0=do not round, 1=round to nearest integer

## Output

* **kvel**: velocity in the range 0-127

## Execution Time

* Init
* Performance

## Examples


```csound


<CsoundSynthesizer>
<CsOptions>
--nosound

</CsOptions>

<CsInstruments>

0dbfs = 1

instr 1
  prints ">> Instr 1\n"
  imindb = -60
  idb = -40
  while idb <= 0 do
  	igain = db(idb)
    ivel = gaintovel(igain, db(imindb), 1/3)
    prints ">> Gain: %d dB, \tamplitude: %f, \tvelocity: %f\n", idb, igain, ivel
    idb += 2
  od
  turnoff
endin


</CsInstruments>
<CsScore>

i1 0 1

</CsScore>
</CsoundSynthesizer>



```


## See also

* [linlin](http://www.csound.com/docs/manual/linlin.html)
* [ampmidicurve](http://www.csound.com/docs/manual/ampmidicurve.html)
* [ampmidid](http://www.csound.com/docs/manual/ampmidid.html)


## Credits

Eduardo Moguillansky, 2024
