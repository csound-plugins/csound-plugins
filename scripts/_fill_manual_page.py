#!/usr/bin/env python3

import argparse
import os
import sys
    
parser = argparse.ArgumentParser(description="fill a markdown template file with an example csd")
parser.add_argument("templatefile")
parser.add_argument("--example", default=None,
                    help="Path to example file (defaults to a .csd file with the "
                    "same name in the same directory or in an 'examples' folder parallel to the template files)")
parser.add_argument("--verbose", action="store_true")
parser.add_argument("-o", "--outfile", default=None)
args = parser.parse_args()


def find_example(templatefile):
    folder0 = os.path.split(templatefile)[0]
    examplefile = os.path.splitext(os.path.split(templatefile)[1])[0] + ".csd"
    folders = [folder0, os.path.join(folder0, "..", "examples")]
    for folder in folders:
        f = os.path.join(folder, examplefile)
        if os.path.exists(f):
            return f 
    return None
    
if args.example is not None:
    examplefile = args.example
else:
    examplefile = find_example(args.templatefile)
    if not examplefile:
        sys.exit("Could not find example, please provide the path via --example")
        
templatestr = open(args.templatefile).read()
examplestr = open(examplefile).read()

examplecode = open(examplefile).read()
# examplestr = f"```csound \n\n{examplecode}\n\n```"
examplestr = "\n".join([
    "```csound",
    "",
    "",
    examplecode,
    "",
    "",
    "```"
])
filledstr = templatestr.format(example=examplestr)
outfile = args.outfile or os.path.splitext(args.templatefile)[0] + '.md'
open(outfile, "w").write(filledstr)
if args.verbose:
    print(f"Filled: {os.path.abspath(args.templatefile)} + {os.path.abspath(examplefile)} -> {os.path.abspath(outfile)}")
    
