import argparse
import shutil
import subprocess
import sys
from pathlib import Path

QUARANTINE = "com.apple.quarantine"


def run(cmd, verbose=False):
    """Run a command, failing loudly if it exits non-zero."""
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(
            f"Command failed with exit code {result.returncode}: {' '.join(cmd)}\n"
            f"{result.stdout.strip()}\n{result.stderr.strip()}"
        )
    if verbose and result.stdout.strip():
        print(result.stdout.strip())
    return result


def strip_quarantine(paths, verbose=False):
    """Remove the quarantine attribute set by web browsers.

    macOS tags downloads with com.apple.quarantine; Gatekeeper then refuses
    to load them in any process ("Apple cannot check it for malicious
    software"). Removing it is what actually makes the plugins runnable.
    """
    xattr = shutil.which("xattr")
    if xattr is None:
        print("WARNING: 'xattr' not found, quarantine attributes will not be removed",
              file=sys.stderr)
        return
    for d in sorted({str(Path(p).parent) for p in paths}):
        try:
            run([xattr, "-dr", QUARANTINE, d], verbose=verbose)
        except RuntimeError as e:
            if "No such xattr" not in str(e):
                print(f"WARNING: could not clear quarantine on {d}: {e}", file=sys.stderr)
    for p in paths:
        try:
            run([xattr, "-d", QUARANTINE, p], verbose=verbose)
        except RuntimeError as e:
            if "No such xattr" not in str(e):
                print(f"WARNING: could not clear quarantine on {p}: {e}", file=sys.stderr)


def codesign(paths, signature="-", verbose=False):
    """Ad-hoc sign the dylibs and verify the signatures.

    Signing ad-hoc ('-') is required on Apple Silicon, where the kernel
    refuses unsigned code. No entitlements are attached: entitlements only
    take effect on a main executable, so putting them on a library does
    nothing useful (and can even cause signing crashes).

    Note: an ad-hoc signature carries no Team ID, so a csound hardened with
    library validation enabled will still reject these plugins; in that case
    the *host* needs the com.apple.security.cs.disable-library-validation
    entitlement.
    """
    if shutil.which("codesign") is None:
        raise RuntimeError("Could not find the binary 'codesign' in the path")
    failed = []
    for p in paths:
        try:
            run(["codesign", "--force", "--sign", signature, str(p)], verbose=verbose)
            run(["codesign", "--verify", "--strict", "--verbose=2", str(p)], verbose=verbose)
        except RuntimeError as e:
            print(f"FAILED: {p}: {e}", file=sys.stderr)
            failed.append(p)
    if failed:
        raise RuntimeError(f"codesigning failed for {len(failed)} of {len(paths)} files")
    print(f"OK: signed and verified {len(paths)} plugin(s)")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Sign the Csound plugin dylibs so macOS will run them.")
    parser.add_argument("--verbose", action="store_true", help="print codesign output")
    parser.add_argument("--signature", default="-",
                        help="identity to sign with ('-' = ad-hoc, the default)")
    parser.add_argument("dylibs", nargs="+", help="paths to the .dylib files to sign")
    args = parser.parse_args()

    strip_quarantine(args.dylibs, verbose=args.verbose)
    codesign(args.dylibs, signature=args.signature, verbose=args.verbose)
