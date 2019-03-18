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
import pathlib


header_template = \
"""
## {libraryname}

{description}

"""


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
            xml = et.tostring(para, encoding="unicode")
            md = xml_to_markdown(xml)
        md = remove_newlines(md)
        return md
    return None


def read_description(opcode: str, docsfolder: pathlib.Path) -> str:
    mdfile = docsfolder / f"{opcode}.md"
    if not mdfile.exists():
        print(f"Could not find opcode file {mdfile}")
        return "?"
    tree = markdown_to_xml(str(mdfile))
    abstract = xml_get_abstract(tree)
    return abstract


def generate(manifestpath, docsfolder=None, overwrite=False, linkopcodes=False, outfile=None):
    pluginroot = pathlib.Path(manifestpath).parent.absolute()
    if not os.path.exists(manifestpath):
        raise Exception(f"Coud not find manifest: {manifestpath}")

    try:
        manifest = json.load(open(manifestpath))
    except json.decoder.JSONDecodeError:
        print(manifestpath)
        raise Exception("Could not parse manifest")

    libname = manifest.get("name")
    if libname is None:
        raise Exception("manifest should define the key 'name' with the name of the library")

    description = manifest.get("description")
    if description is None:
        raise Exception("manifest should define the key 'description' with"
                        " a description of what the opcodes in this library do")

    opcodes = manifest.get('opcodes')
    if opcodes is None:
        raise Exception("manifest should define the key 'opcodes' holding a list of opcodes")

    headerstr = header_template.format(libraryname=libname, description=description)
    if docsfolder:
        docsfolder = pathlib.Path(docsfolder)
    else:
        docsfolder = pluginroot / "docs"

    if not (docsfolder.exists() and docsfolder.is_dir()):
        raise Exception(f"documentation folder not found (searched: {docsfolder})"
                        ", can't generate README")

    if outfile and os.path.exists(outfile) and not overwrite:
        raise Exception(f"\nOutfile {outfile} exists, aborting (use --overwrite to overwrite)\n")

    if linkopcodes:
        templ = "* [{opcode}]({opcode}): {descr} \n"
    else:
        templ = "* **{opcode}**: {descr} \n"
    if outfile is None:
        f = io.StringIO()
    else:
        f = open(outfile, "w")
    f.write(headerstr)
    for opcode in opcodes:
        descr = read_description(opcode, docsfolder)
        s = templ.format(opcode=opcode, descr=descr)
        f.write(s)
    return f.getvalue()
    
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--docsfolder", default=None)
    parser.add_argument("-o", "--overwrite", action="store_true")
    parser.add_argument("--outfile", default=None)
    parser.add_argument("--linkopcodes", action="store_true")
    parser.add_argument("manifest")
    args = parser.parse_args()
    manifestpath = os.path.split(args.manifest)[0]
    try:
        s = generate(manifestpath, docsfolder=args.docsfolder, overwrite=args.overwrite, 
                     linkopcodes=args.linkopcodes, outfile=args.outfile)
    except Exception as e:
        print(str(e))
        sys.exit(-1)
    if args.outfile is None:
        print(s)
    
        
    
    
    
    
    
    
