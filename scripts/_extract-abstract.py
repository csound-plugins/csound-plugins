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
parser.add_argument("markdownfile")
args = parser.parse_args()


if not os.path.exists(args.markdownfile):
    print(f"Coud not find file: {args.markdownfile}")
    sys.exit(-1)


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


def read_description(mdfile):
    if not os.path.exists(mdfile):
        print(f"Could not find opcode file {mdfile}")
        return "?"
    tree = markdown_to_xml(mdfile)
    abstract = xml_get_abstract(tree)
    return abstract


descr = read_description(args.markdownfile)
print(descr)