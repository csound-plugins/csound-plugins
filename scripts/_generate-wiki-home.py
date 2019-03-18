#!/usr/bin/env python3

import argparse
import pathlib
import sys
import generate_plugins_list

parser = argparse.ArgumentParser()
parser.add_argument("--wikifolder", default=None)
parser.add_argument("--destfolder", default=None)
args = parser.parse_args()
ownfolder = pathlib.Path(sys.argv[0]).parent.absolute()
rootfolder = ownfolder.parent

if args.wikifolder:
    wikifolder = pathlib.Path(args.wikifolder)
else:
    wikifolder = rootfolder / "wiki"
destfolder = pathlib.Path(args.destfolder or wikifolder)
outfile = destfolder / "Home.md"
template = wikifolder / "Home.md_"
if not template.exists():
    print(f"Template file not found, searched in {template}")
    sys.exit(-1)

def generate_plugins_str(manifest):
    """ lib: fol of the library """
    try:
        s = generate_plugins_list.generate(manifest, linkopcodes=True)
    except Exception as e:
        raise Exception(f"error with manifest {manifest}:\n {e}")
    return s

def find_recursive(root, pattern):
    return list(pathlib.Path(root).glob(f"**/{pattern}"))

manifests = list(find_recursive(rootfolder/"src", "manifest.json"))
sep = "\n"
collectedplugins = sep.join(generate_plugins_str(manifest) for manifest in manifests)
filledtempl = open(template).read().format(plugins=collectedplugins)
open(outfile, "w").write(filledtempl)
print(filledtempl)
