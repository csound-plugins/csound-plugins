#!/usr/bin/env python3

import argparse
import pathlib
import sys
import fnmatch
import json

parser = argparse.ArgumentParser()

parser.add_argument("--destdir", default=None,
                    help="The dir to put the created mkdocs site")
parser.add_argument("--mkdocsdir", default=None,
                    help="The dir where the mkdocs resources are")
args = parser.parse_args()

owndir = pathlib.Path(sys.argv[0]).absolute().parent
rootdir = owndir.parent

if args.destdir:
    destdir = pathlib.Path(args.destdir)
else:
    destdir = rootdir.parent / "csound-plugins.mkdocs"
if not destdir.exists():
    destdir.mkdir(exist_ok=True)

mkdocsdir = pathlib.Path(args.mkdocsdir) if args.mkdocsdir else rootdir/"mkdocs"


def list_all_opcodes(exclude_pattern="*template*"):
    manifests = list((rootdir/"src").glob("**/manifest.json"))
    manifests = [m for m in manifests if fnmatch.fnmatch(m, exclude_pattern)]
    opcodes = []
    for manifest in manifests:
        try:
            d = json.load(manifest)
            opcodeshere = d['opcodes']
            opcodes.extend(opcodeshere)
        except json.decoder.JSONDecodeError:
            print(f"Could not parse {manifest}")
    return opcodes

opcodes = list_all_opcodes()
print(opcodes)
