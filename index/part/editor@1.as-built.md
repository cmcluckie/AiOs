# PART.editor@1 as built

Hand-derived from the source on 2026-10-13, pending the generator (plan 7.1).
Measured with the gcc in Claude's workspace (not the jig's).

## What it does
Loads notes.txt from the disk if it is there (an empty file if not), clears the
screen and draws it: 23 rows of text from the first row and column on screen,
then one bottom line in reverse video that always starts with the three
commands, " Ctrl+S Save   Ctrl+Z Undo   Ctrl+Q Quit", followed either by one
message ("Saved.", "Not saved: the disk said no.", "Unsaved.  Ctrl+Q again to
quit.", "Full: 7168 is the most a file can hold.") or by the file's name, a *
if it has changed since it was loaded or saved, and its length "of 7168".
Then reads keys and draws a whole new frame after every key.

Keys: printable bytes (0x20-0x7e) insert at the cursor; Enter (\r, or \n, with
\r\n counted once) inserts a line break; Backspace (0x7f or 0x08) deletes the
byte before the cursor; the VT100 codes ESC [ A B C D move up, down, right and
left (up and down keep the column where the line is long enough), ESC [ H or
ESC [ 1 ~ or ESC [ 7 ~ go to the start of the line, ESC [ F or ESC [ 4 ~ or
ESC [ 8 ~ to its end, ESC [ 3 ~ deletes the byte under the cursor; ESC O
works in place of ESC [.  Ctrl+S (0x13) saves the whole text; Ctrl+Z (0x1a)
swaps the text with the copy taken when the last run of typing or deleting
began, so a second Ctrl+Z swaps it back; Ctrl+Q (0x11) quits, but with
unsaved changes the first one only shows the question and the second quits.
Any other byte, including Tab, does nothing.  Quitting clears the screen.

## Every syscall or hardware access
None.  System calls: load once at start, write once per frame, read, save on
Ctrl+S, exit on quit.

## Every symbol it imports
None; the system call table is handed to _start.

## Every byte it writes anywhere
To the console: frames, each ESC [ ? 25 l, ESC [ H, 23 rows each ending
ESC [ K \r \n, the bottom line between ESC [ 7 m and ESC [ 0 m, then the
cursor's place and ESC [ ? 25 h; at start ESC [ 0 m ESC [ 2 J; on quit
ESC [ 0 m ESC [ 2 J ESC [ H.  To the disk, through the save system call, only
on Ctrl+S: notes.txt, exactly the bytes of the text, at most 7168.

## Size
3805 bytes of code, 20068 of zeroed buffers (the text, its one undo copy and
one frame), 23920 bytes as a flat binary.
