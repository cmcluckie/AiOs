# PART.gate@2 as built

Hand-derived from the source on 2026-10-20, pending the generator (plan 7.1).
Measured with the gcc and QEMU in Claude's workspace (software emulation), not the jig's.

## What it does
Everything PART.gate@1 does, unchanged for every program @1 ran (its as-built
spec still holds; TST.gate.ladder-2@1 runs the old ladder on it).  Added (part 20,
findings 0023 item 10):

- gate_exec: a program from a Linux program file.  Its window is any 4 MiB-aligned
  virtual range at or above 4 MiB (the loader gives 16 MiB from 0x08000000) on its
  own physical RAM (from 4 MiB).  It starts at the file's entry with the stack the
  loader laid out; it is handed no table and knocks on int 0x80 itself.  Its heap
  starts at the page after its last piece: brk grows up from there, mmap2 down from
  1 MiB below where the stack starts.
- The calls it answers for such a program, at Linux's 386 numbers, each as small as
  cat needs, each refusing the rest with Linux's error number:
  open (5): read-only, the disk's root directory only, a leading / allowed; written
  to or made, EROFS; a name below the root, or not there, ENOENT; more than four
  open, EMFILE.  The file is read whole when opened (up to 8,192 bytes), from the
  kernel's load call.  close (6).  read (3): the console (Ctrl+D ends the input, as
  at the start of a line on a Unix terminal; it is not handed over) or an open
  file.  write (4) and writev (146): the console.  brk (45): moves the heap's end
  within its room, else stays, as Linux does.  mmap2 (192): fresh private memory
  only, zeroed; anything else EINVAL; no room, ENOMEM.  munmap (91): says yes and
  gives nothing back.  set_thread_area (243): one segment, GDT entry 6, ring 3 data
  at the base asked for, 4 GiB; any other entry EINVAL.  set_tid_address (258): 1.
  statx (383): an open descriptor only (the console as a character device with a
  1 KiB block; a file as a read-only regular file and its size); by name, ENOSYS.
  ioctl (54): ENOTTY.  exit (1) and exit_group (252): "exit N" on the console, then
  the kernel's exit.  1000 to 1003, the old table's own alloc, ms, load and save, ENOSYS for a Linux program since part 21 (decision 0075; old programs keep them).  Any other number, ENOSYS.  Every buffer and name is checked to
  lie inside the window, else EFAULT, and the kernel does not touch it.

## What it does not turn on
The processor's vector instructions (SSE): AiOs never sets CR4.OSFXSR, so a program
that uses them is stopped with an invalid-instruction fault (vector 6), as the jig
showed (findings 0023 item 13).  Programs built against musl are built for the 486.

## Every syscall or hardware access
As @1, and it loads %gs's descriptor only through the table: a program sets %gs
itself after set_thread_area.

## Every symbol it imports
As @1.  Exports gate_exec as well (gate.h).

## Every byte it writes anywhere
As @1, and: GDT entry 6 (the program's thread segment); the entry number back into
the program's user_desc; the open files' table (four slots of 8,192 bytes); the
program's buffers it was asked to fill (read, statx, mmap2's zeroes); "exit N".

## Size
gate.c 704 lines, 545 of them code (@1: 302).  gate_entry.S, gate_user.S and
gate.h as @1 but for gate.h's gate_exec.  The kernel file for the cat flavor:
29,048 bytes (text 16,524, data 124, bss 153,800, the open files' table among it).
