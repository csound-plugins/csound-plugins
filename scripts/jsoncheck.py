#!/usr/bin/env python3
import json
import sys

def print_with_linenumbers(s):
    for i, line in enumerate(s.splitlines()):
        print(f"{i+1:003d} {line}")


jsonfile = sys.argv[1]
s = open(jsonfile).read()
try:
    d = json.loads(s)
    print("ok")
    sys.exit(0)
except Exception as e:
    print_with_linenumbers(s)
    print(e)
    sys.exit(1)
