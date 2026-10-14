#!/bin/sh
# The keystrokes the editor's tests type, made with printf so every byte is
# readable here.  Run from the repo root to remake tests/fixtures/editor-*.in.
# The keys are what a VT100 sends: the arrows ESC [ A B C D, Home ESC [ H,
# End ESC [ F, Delete ESC [ 3 ~, Backspace 0x7f, Enter \r; Ctrl+S 0x13,
# Ctrl+Z 0x1a, Ctrl+Q 0x11.
set -eu
cd "$(dirname "$0")"
L='\033[D' R='\033[C' U='\033[A' D='\033[B' HOME='\033[H' END='\033[F' DEL='\033[3~'
BS='\177' ENTER='\r' S='\023' Z='\032' Q='\021'
# typing: Helo world, back 7, l, End !, Home Delete J, End Enter second,
# Up End ?, Down Backspace d, Right (nothing there), save, quit
printf "Helo world$L$L$L$L$L$L${L}l$END!$HOME${DEL}J$END${ENTER}second$U$END?$D${BS}d$R$S$Q" > editor-typing.in
printf "$Q" > editor-quit.in
printf "first version$S$Q" > editor-first.in
printf "$DEL$DEL$DEL$DEL${DEL}second$S$Q" > editor-second.in
# undo: one, Home, "zero " is a new run; one Ctrl+Z takes it back; save "one"
printf "one${HOME}zero $Z$S$Q" > editor-undo1.in
# then " two" at the end, Ctrl+Z takes it back, Ctrl+Z puts it back; save "one two"
printf "$END two$Z$Z$S$Q" > editor-undo2.in
# typing, then Ctrl+Q asks, Ctrl+Q quits without saving
printf "abc$Q$Q" > editor-quit-asks.in
