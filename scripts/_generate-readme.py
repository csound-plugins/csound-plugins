#!/usr/bin/env python3

# Given a manifest.json, generate a README.md file
import json
import argparse
import sys
import tempfile
import os
import subprocess
import xml.etree.ElementTree as et

parser = argparse.ArgumentParser()
parser.add_argument("--manifest", default="manifest.json")
parser.add_argument("--docsfolder", default=None)
parser.add_argument("--overwrite", action="store_true")
parser.add_argument("--outfile", default=None)
args = parser.parse_args()


header_template = \
"""
# {libraryname}

## Description

{description}

## Opcodes implemented

"""

if not os.path.exists(args.manifest):
    print(f"Coud not find manifest: {args.manifest}")
    sys.exit(-1)

try:
    manifest = json.load(open(args.manifest))
except:
    print(f"Could not parse manifest: {args.manifest}")
    sys.exit(-1)

libname = manifest.get("name")
if libname is None:
    print("Error: manifest should define the key 'name' with the name of the library")
    sys.exit(-1)

description = manifest.get("description")
if description is None:
    print("Error: manifest should define the key 'description' with"
          " a description of what the opcodes in this library do")
    sys.exit(-1)
    
opcodes = manifest.get('opcodes')
if opcodes is None:
    print("Error: manifest should define the key 'opcodes' holding a list of opcodes")
    sys.exit(-1)

headerstr = header_template.format(libraryname=libname, description=description)

def find_mdfile(opcode, docsfolder):
    f = os.path.join(docsfolder, f"opcode.md")
    f = os.path.abspath(f)
    if os.path.exists(f):
        return f
    return None
    

def markdown_to_xml(mdfile: str) -> str:
    """
    returns the abstract from mdfile

    mdfile: a markdown file
    """
    # first, we use pandoc to convert to xml
    xmlfile = tempfile.mktemp(suffix=".xml")
    subprocess.call(["pandoc", "--to", "docbook5", "-o", xmlfile, mdfile])
    # before parsing it, we need to add the xlink namespace, since
    # pandoc leaves that out :-(
    xmlfile2 = tempfile.mktemp(suffix=".xml")
    with open(xmlfile2, "w") as f:
        f.write('<article xmlns:xlink="http://www.w3.org/1999/xlink">')
        f.write(open(xmlfile).read())
        f.write('</article>')
    # now read it
    tree = et.parse(xmlfile2)
    # remove tempfiles
    os.remove(xmlfile)
    os.remove(xmlfile2)
    return tree


def remove_newlines(s):
    """ remove newlines and collapse multiple spaces to 1 """
    return " ".join(s.replace("\n", " ").split())


def xml_to_markdown(xmlfragment):
    xmlfile = tempfile.mktemp(suffix=".xml")
    open(xmlfile, "w").write(xmlfragment)
    markdownfile = tempfile.mktemp(suffix=".md")
    subprocess.call(["pandoc", "--from", "docbook", "--to", "markdown",
                     "-o", markdownfile, xmlfile])
    out = open(markdownfile).read()
    os.remove(xmlfile)
    os.remove(markdownfile)
    return out

def xml_get_abstract(tree, remove_format=True):
    for section in tree.iter("section"): 
        title = section.find("title")
        if title.text != "Abstract":
            continue
        para = section.find("para")
        if para is None:
            return None
        if remove_format:
            md = et.tostring(para, encoding="unicode", method="text")
        else:
            md = et.tostring(para, encoding="unicode")
            md = xml_to_markdown(txt)
        md = remove_newlines(md)
        return md
    return None


def read_description(opcode, docsfolder):
    mdfile = os.path.join(docsfolder, f"{opcode}.md")
    if not os.path.exists(mdfile):
        print(f"Could not find opcode file {mdfile}")
        return "?"
    tree = markdown_to_xml(mdfile)
    abstract = xml_get_abstract(tree)
    return abstract


manifestpath = os.path.split(args.manifest)[0]
docsfolder = args.docsfolder

if docsfolder is None:
    docsfolder = os.path.join(manifestpath, "docs")
docsfolder = os.path.abspath(docsfolder)
if not (os.path.exists(docsfolder) and os.path.isdir(docsfolder)):
    print(f"documentation folder not found (searched: {docsfolder})"
          ", can't generate README")
    sys.exit(-1)
    
if args.outfile:
    outfile = args.outfile
else:
    outfile = os.path.join(manifestpath, "README.md")

if os.path.exists(outfile) and not args.overwrite:
    print(f"\nOutfile {outfile} exists, aborting (use --overwrite to overwrite)\n")
    sys.exit(-1)

    
with open(outfile, "w") as f:
    f.write(headerstr)
    for opcode in opcodes:
        descr = read_description(opcode, docsfolder)
        s = f"* **{opcode}**: {descr} \n"
        f.write(s)

