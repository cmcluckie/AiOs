#!/bin/sh
# Petition 0000.  See jig/build.sh.
exec sh "$(dirname "$0")/build.sh" hello "${1:-}"
