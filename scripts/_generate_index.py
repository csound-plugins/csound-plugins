#!/usr/bin/env python3

import argparse
from pathlib import Path
import sys
import listopcodes

parser = argparse.ArgumentParser()
parser.add_argument("--outfile", default=None)
parser.add_argument("--template", default=None)
parser.add_argument("--mode", default="mkdocs", choices=["mkdocs", "wiki"])
parser.add_argument("--echo", action="store_true")
args = parser.parse_args()
ownfolder = Path(sys.argv[0]).parent.absolute()
rootdir = ownfolder.parent

template = Path(args.template) if args.template else rootdir/"mkdocs"/"Home.md_"
print("template: ", template)

if not template.exists():
    print(f"Template file not found, searched in {template}")
    sys.exit(-1)

if args.outfile:
    outfile = Path(args.outfile)
else:
    if args.mode == "wiki":
        outfile = template.parent / "Home.md"
    else:
        outfile = template.parent / "index.md"
    
def plugins_str(manifest, mode):
    if mode == "wiki":
        linkprefix = ""
        linksuffix = ""
    elif mode == "mkdocs":
        linkprefix = "opcodes/"
        linksuffix = ".md"
    else:
        raise ValueError("mode must be one of 'wiki', 'mkdocs'")

    try:
        s = listopcodes.generate(manifest, 
                                 linkopcodes=True, 
                                 linksuffix=linksuffix, 
                                 linkprefix=linkprefix)
    except Exception as e:
        raise Exception(f"error with manifest {manifest}:\n {e}")
    return s

pluginfolders = [folder for folder in (rootdir/"src").glob("*")
                 if folder.is_dir()]
manifests = []
for pluginfolder in pluginfolders:
    for possible_manifest in [pluginfolder/"manifest.json", pluginfolder/"risset.json"]:
        if possible_manifest.exists():
            manifests.append(possible_manifest)
            break
    
# manifests = list((rootdir/"src").glob("**/manifest.json"))

plugins_jointstr = "\n".join(plugins_str(m, args.mode) for m in manifests)
filledtempl = open(template).read().format(plugins=plugins_jointstr)

with open(outfile, "w") as f:
    f.write(filledtempl)

if args.echo:
    print(filledtempl)
