#!/usr/bin/env python3

# Given a manifest.json, generate a README.md file
import json
import argparse
import sys
import os
import io
from pathlib import Path
import re


header_template = \
"""
## {libname}

{description}

"""

def remove_newlines(s):
    return " ".join(s.replace("\n", " ").split())

def md_read_abstract(mdfile: str, plaintext=True) -> str:
    abstract = False
    abstractlines = []
    for line in open(mdfile):
        if abstract:
            if line.strip().startswith("#"):
                break
            else:
                abstractlines.append(line)
        elif re.search(r"#\s+[aA]bstract\b", line):
            abstract = True
    if not abstractlines:
        return None
    lines = [line.strip() for line in abstractlines]
    lines = [line for line in lines if line]
    out = " ".join(lines)
    if plaintext:
        out = out.replace("**", "").replace("`", "")
    return out

def read_description(opcode: str, docsfolder: Path) -> str:
    mdfile = docsfolder / f"{opcode}.md"
    if not mdfile.exists():
        print(f"Could not find opcode file {mdfile}")
        return "?"
    return md_read_abstract(str(mdfile))

def manifest_ok(manifest: dict):
    neededkeys = {"name", "short_description", "opcodes"}
    return set(manifest.keys()).issuperset(neededkeys)


def generate(manifestpath, docsfolder=None, overwrite=False, linkopcodes=False,
             linkprefix="", linksuffix=""):
    pluginroot = Path(manifestpath).parent.absolute()
    if not os.path.exists(manifestpath):
        raise Exception(f"Coud not find manifest: {manifestpath}")

    try:
        manifest = json.load(open(manifestpath))
    except json.decoder.JSONDecodeError as e:
        print("\n----------------------------\n")
        print("    Could not parse ", manifestpath)
        print("\n----------------------------\n")
        for i, line in enumerate(open(manifestpath)):
            print(f"{i+1:02d}: {line.rstrip()}")
        raise

    if not manifest_ok(manifest):
        print("A manifest should have following keys: "
              "\n  name: name of the library"
              "\n  opcodes: a list of opcodes defined"
              "\n  short_description: an abstract (one line) of what this library does")
        print(manifest)
        raise Exception(f"manifest malformed")

    libname = manifest["name"]
    description = manifest["short_description"]
    opcodes = manifest["opcodes"]
    headerstr = header_template.format(libname=libname, description=description)
    
    docsfolder = Path(docsfolder) if docsfolder else pluginroot/"doc"
    if not (docsfolder.exists() and docsfolder.is_dir()):
        raise Exception(f"documentation folder not found (searched: {docsfolder})"
                        ", can't generate README")

    if linkopcodes:
        templ = "* [{opcode}]({linkprefix}{opcode}{linksuffix}): {descr} \n"
    else:
        templ = "* **{opcode}**: {descr} \n"
    
    f = io.StringIO()
    f.write(headerstr)
    for opcode in opcodes:
        descr = read_description(opcode, docsfolder)
        s = templ.format(opcode=opcode, descr=descr, 
                         linkprefix=linkprefix, linksuffix=linksuffix)
        f.write(s)
    return f.getvalue()

   
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--docsfolder", default=None)
    parser.add_argument("-o", "--overwrite", action="store_true")
    parser.add_argument("--outfile", default=None)
    parser.add_argument("--linkopcodes", action="store_true")
    parser.add_argument("--linkprefix", default="")
    parser.add_argument("--linksuffix", default="")
    parser.add_argument("manifest")
    args = parser.parse_args()
    manifestpath = os.path.split(args.manifest)[0]
    link = args.linkopcodes or (args.linkprefix or args.linksuffix)
    try:
        s = generate(manifestpath, 
                     docsfolder=args.docsfolder, 
                     overwrite=args.overwrite, 
                     linkopcodes=link, 
                     linkprefix=args.linkprefix, 
                     linksuffix=args.linksuffix)
    except Exception as e:
        print(str(e))
        sys.exit(-1)

    if args.outfile is None:
        print(s)
    else:
        outfile = args.outfile
        if outfile and os.path.exists(outfile) and not args.overwrite:
            raise Exception(f"Outfile {outfile} exists, aborting"
                            "(use --overwrite to overwrite)\n")
        open(args.outfile, "w").write(s)
    
    

