#!/usr/bin/env python3

# Given a manifest.json, generate a README.md file
import json
import argparse
import sys
import tempfile
import os
import subprocess
import xml.etree.ElementTree as et
import io
from pathlib import Path


header_template = \
"""
## {libname}

{description}

"""

def markdown_to_xml(mdfile: str) -> str:
    """
    returns the abstract from mdfile

    mdfile: a markdown file
    """
    xmlfile = tempfile.mktemp(suffix=".xml")
    subprocess.call(["pandoc", "--to", "docbook5", "-o", xmlfile, mdfile])
    # before parsing it, we need to add the xlink namespace, since
    # pandoc leaves that out :-(
    xmlfile2 = tempfile.mktemp(suffix=".xml")
    with open(xmlfile2, "w") as f:
        f.write('<article xmlns:xlink="http://www.w3.org/1999/xlink">')
        f.write(open(xmlfile).read())
        f.write('</article>')
    tree = et.parse(xmlfile2)
    os.remove(xmlfile)
    os.remove(xmlfile2)
    return tree


def remove_newlines(s):
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
            xml = et.tostring(para, encoding="unicode")
            md = xml_to_markdown(xml)
        return remove_newlines(md)
    return None


def read_description(opcode: str, docsfolder: Path) -> str:
    mdfile = docsfolder / f"{opcode}.md"
    if not mdfile.exists():
        print(f"Could not find opcode file {mdfile}")
        return "?"
    tree = markdown_to_xml(str(mdfile))
    abstract = xml_get_abstract(tree)
    return abstract

def manifest_ok(manifest: dict):
    neededkeys = {"name", "description", "opcodes"}
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
              "\n  description: a description of what this library does")
        print(manifest)
        raise Exception(f"manifest malformed")

    libname = manifest["name"]
    description = manifest["description"]
    opcodes = manifest["opcodes"]
    headerstr = header_template.format(libname=libname, description=description)
    
    docsfolder = Path(docsfolder) if docsfolder else pluginroot/"docs"
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
    
    

