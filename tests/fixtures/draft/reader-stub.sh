#!/bin/sh
# A stand-in for the reader in the draft step's test: answers pin and read, and
# writes down every question it is asked in $STUBLOG.
echo "$*" >> "${STUBLOG:-/dev/null}"
case "$1" in
pin) echo stub ;;
read) echo "usr/src/cmd/cat.c:1: /*" ;;
*) echo "the stub does not answer $1" >&2; exit 1 ;;
esac
