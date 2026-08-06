# Documentation

## Dependencies

* python3
* mkdocs

We assume that python is present (python >= 3.9)

    pip install mkdocs --user

## Usage

To generate all documentation, at the root of the project, do:


    scripts/generate-docs 
    mkdocs build

The documentation will be in `./site`

## Portable Linux build

To reproduce `buildlinuxportable` locally, create its portable archive, and
test the user installer from that archive:

    scripts/buildlinuxportable-local.sh

The script installs the job's Ubuntu build dependencies using `apt` and writes
all build output to `local-build/linux-portable/`.
