import argparse
import subprocess
import os
import shutil
from pathlib import Path


_entitlements = r"""
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.security.cs.disable-library-validation</key>
    <true/>
    <key>com.apple.security.device.audio-input</key>
    <true/>
    <key>com.apple.security.device.camera</key>
    <true/>
</dict>
</plist>
"""


def save_entitlements(target: str, appname='adhoc-codesign', force=False) -> Path:
    path = Path(f"~/Library/Application Support/{appname}/{target}.entitlements").expanduser()
    if path.exists() and not force:
        contents = open(path).read()
        if contents == _entitlements:
            return path
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, 'w') as f:
        f.write(_entitlements)
    plutil = shutil.which('plutil')
    if plutil:
        subprocess.call([plutil, path.as_posix()])
    return path


def codesign(dylibpaths: list[str], target: str, signature='-') -> Path:
    """
    Codesign the given library binaries using entitlements

    Args:
        dylibpaths: a list of paths to codesign
        signature: the signature used. '-' indicates to sign it locally
    """
    if not shutil.which('codesign'):
        raise RuntimeError("Could not find the binary 'codesign' in the path")
    entitlements_path = save_entitlements(target=target)
    assert os.path.exists(entitlements_path)
    for dylibpath in dylibpaths:
        subprocess.call(['codesign', '--force', '--sign', signature, '--entitlements', entitlements_path, dylibpath])
        subprocess.call(['codesign', '--display', '--verbose', dylibpath])
    return entitlements_path


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--signature', default='-')
    parser.add_argument('-t', '--target', default='generic', help="Will be the name of the entitlements file")
    parser.add_argument("dylibs", nargs="+")
    args = parser.parse_args()
    entitlements_path = codesign(args.dylibs, target=args.target, signature=args.signature)
    print(f"Saved entitlements file to '{entitlements_path}'")
