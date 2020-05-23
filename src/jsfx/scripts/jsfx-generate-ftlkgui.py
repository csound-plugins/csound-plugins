#!/usr/bin/env python3

import argparse
import json
import subprocess
import sys
import os
from dataclasses import dataclass

parser = argparse.ArgumentParser()
parser.add_argument("--sliderwidth", default=300, type=int,
                    help="width of a slider, in pixels")
parser.add_argument("--height", default=30, type=int)
parser.add_argument("--linefactor", default=2, type=float)
parser.add_argument("-o", "--outfile", default="", type=str)
parser.add_argument("--color", choices=["lila"], default="lila")
parser.add_argument("--full", action="store_true")
parser.add_argument("script")
args = parser.parse_args()

# get own path
ownfolder = os.path.split(os.path.abspath(sys.argv[0]))[0]
jsfxparseui = os.path.join(ownfolder, "jsfx-parse-ui.py")

if not os.path.exists(jsfxparseui):
    print(f"Could not find jsfx-parse-ui.py, searched in {jsfxparseui}")
    sys.exit(-1)

jsontxt = subprocess.check_output([jsfxparseui, args.script]).decode('utf-8')
j = json.loads(jsontxt)

lines = []
_ = lines.append

slider_value_margin = 3
value_width = 50
margin_left = 30
margin_right = margin_left * 2
margin_up = margin_down = 30
line_margin = args.height * args.linefactor
total_line_height = line_margin

numsliders = len(j['sliders'])

colordefs = {
    'lila': (150, 100, 150, 200, 100, 250)
}

panelwidth = (margin_left +
              args.sliderwidth +
              slider_value_margin +
              value_width +
              margin_right)
panelheight = (margin_up + numsliders * total_line_height + margin_down)
value_x = margin_left + args.sliderwidth + slider_value_margin
title = j.get('desc', 'title')

if args.full:
    template = """
<CsoundSynthesizer>

<CsOptions>
-odac
-d
</CsOptions>

<CsInstruments>

sr     = 44100
ksmps  = 64
nchnls = 2
0dbfs  = 1

    """
    lines.extend(template.splitlines())

_(f"FLpanel \"{title}\", {panelwidth}, {panelheight}, 50, 50")
colors = colordefs[args.color]
colorstr = ", ".join(str(c) for c in colors)
_(f"FLcolor {colorstr}")
_(f"i__w, i__h, i__line = {args.sliderwidth}, {args.height}, {line_margin}")
_(f"iy, i__marginx = {margin_up}, {margin_left}")

def generate_name(slider):
    descr = slider['descr']
    descr = descr.split("(")[0]
    descr = "".join(part.capitalize() for part in descr.split())
    name = descr.replace("-", "").replace("&&", "").replace("&", "").replace("_", "")
    #parts = name.split("_")
    #name = "".join(part.capitalize() for part in parts)
    return "gk" + name # .capitalize()

slidertype = 3
sliders = j['sliders']
for slider in sliders:
    varname = generate_name(slider)
    slidernum = int(slider['sliderid'][6:])
    descr = slider['descr']
    minval = slider['minval']
    maxval = slider['maxval']
    valuename = f"i_v{slidernum}"
    slider['slidernum'] = slidernum
    slider['valuename'] = valuename
    sliderhandle = f"i_s{slidernum}"
    slider['handle'] = sliderhandle
    _(f"{valuename} FLvalue \"\", {value_width}, {args.height}, {value_x}, iy")
    _(f"{varname}, {sliderhandle} FLslider \"{descr}\", {minval}, {maxval}, 0, {slidertype}, {valuename}, i__w, i__h, i__marginx, iy")
    _("iy += i__line")
lines.pop()

_("FLpanelEnd")
_("FLrun")

for slider in sliders:
    default = slider['defaultval']
    handle = slider['handle']
    descr = slider['descr']
    _(f"FLsetVal_i {default}, {handle}\t\t ; {descr}")

if args.full:
    template = \
"""

instr 1
  ; xxx
endin

</CsInstruments>

<CsScore>

i1 0 3600

</CsScore>
</CsoundSynthesizer>
"""
    lines.extend(template.splitlines())

    
if args.outfile:
    with open(args.outfile, "f") as f:
        for line in lines:
            f.write(line)
else:
    for line in lines:
        print(line)
