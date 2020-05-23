#!/usr/bin/env python3

import argparse
import json

from dataclasses import dataclass

parser = argparse.ArgumentParser()
parser.add_argument("-o", "--outfile", default="", type=str)
parser.add_argument("script")
args = parser.parse_args()


@dataclass
class SliderDef:
    sliderid: str
    defaultval: float
    minval: float
    maxval: float
    incr: str
    descr: str

    def slidernum(self):
        return int(self.sliderid[6:])

    def asdict(self):
        return {
            'sliderid': self.sliderid,
            'defaultval': self.defaultval,
            'minval': self.minval,
            'maxval': self.maxval,
            'incr': self.incr,
            'descr': self.descr
        }
    
def parse_slider(line):
    sliderid, *rest = line.split(":")
    rest = ":".join(rest)
    # slider3: 0.5 <0.1, -1, 0.001> Text
    defaultval, rest = rest.split("<")
    rangedef, slidertext = rest.split(">")
    try:
        minval, maxval, *incr = rangedef.split(",")
        incr = ",".join(incr)
    except ValueError:
        raise ValueError(f"Could not parse range definition: {rangedef}")
    return SliderDef(sliderid=sliderid,
                     defaultval=float(defaultval),
                     minval=float(minval),
                     maxval=float(maxval),
                     incr=incr,
                     descr=slidertext.strip())
    
    
desc = None
sliders = []
for line in open(args.script):
    line = line.strip()
    if line.startswith("@"):
        break
    if line.startswith("desc:"):
        desc = line.split(":")[1].strip()
        continue
    elif line.startswith("slider"):
        sliderdef = parse_slider(line)
        sliders.append(sliderdef)

sliderdicts = [slider.asdict() for slider in sliders]
d = {'sliders': sliderdicts}
d['desc'] = desc if desc else ""

if args.outfile:
    with open(args.outfile, "w") as f:
        json.dump(d, f, indent=True)
else:
    print(json.dumps(d, indent=True))
