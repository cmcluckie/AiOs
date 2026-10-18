# PART.gate@1 as built

Hand-derived from the source on 2026-10-18, pending the generator (plan 7.1).
Measured with the gcc in Claude's workspace (not the jig's).

## What it does
Runs every program in ring 3, the processor's least privileged level, and gives
it exactly one way to the kernel: `int 0x80`.  Anything else a program reaches
for (the kernel's memory, the screen's memory, an I/O port, a privileged
instruction) is refused by the processor, and the kernel prints a Guru
Meditation line and halts, or, if a sharing part has registered a scheduler,
hands the stop to it.

Memory: every address maps to itself, except the program's window, which maps
to the program's own RAM.  The first 4 MiB are 4 KiB pages, kernel only, with
two exceptions: one read-only page of system call stubs (section `.gate_user`,
link script of the kernel) that ring 3 may read and run, and, for a single
program on its own, the gate's 16 KiB stack that ring 3 stands on.  Every
other 4 MiB is one large page, kernel only, except the window (a multiple of
4 MiB, from 4 MiB up, in 4 MiB pages that ring 3 may use).  CR0.WP is on, so
the kernel obeys read-only pages too.  Largest window: up to 0xffc00000.  The
kernel, the stub page and the gate's stack must fit under 4 MiB, or the kernel
halts before the program starts.

System calls: number in eax, arguments in ebx, ecx, edx, the answer in eax
(Linux's 32-bit x86 convention).  Linux's numbers: exit 1, read 3, write 4.
Ours, TECH-DEBT 1 (findings 0021 #5): alloc 1000, ms 1001, load 1002, save
1003.  Any other number answers -38 (ENOSYS).  Write accepts file 1 or 2, read
file 0, else -9 (EBADF).  Every buffer is checked to lie inside the program's
window (or the gate's own stack for a lone program), and every file name too,
up to 256 bytes with its terminating zero, else -14 (EFAULT) and the kernel
does not touch the address.  The program is handed a table of seven stubs, the
same seven entries at the same offsets as the table of every earlier loader
(write, exit, alloc, read, ms, load, save at 0 to 24), each a cdecl function
that runs `int 0x80`, so programs built for any earlier kernel run unchanged.
A program that returns from `_start` returns into a stub that exits.

One program: `gate_run`.  Interrupts off, the kernel's boot stack as the
processor's landing place for a trap.  Several programs: `gate_space_new` (a
page directory, a 16 KiB kernel stack and a first set of registers for each,
the same virtual window in all, up to four), `gate_schedule` (the sharing
part's two decisions: what a hardware interrupt does, what a stop does),
`gate_start`, `gate_resume`.  Hardware interrupts on the 8259's vectors 0x30
to 0x3f and the APIC's spurious vector 0xff are routed only when a scheduler
is registered; with none, no gate exists for them.

## Every syscall or hardware access
Instructions: `lgdt`, `lidt`, `ltr`, `ljmp` (to reload cs), moves to ds, es,
ss, fs, gs; `int 0x80` (in the stubs); `iret`, `pushal`, `popal`, `cld`.
Registers: cr0 (set PG and WP), cr3 (every switch of program), cr4 (PSE), cr2
(read, on a page fault).  No port, no device: it never touches the 8259 or the
8254, which belong to the sharing part.

## Every symbol it imports
kernel_say, kernel_flush, kernel_halt (kernel.h); kernel_end, stack_top,
gate_user_start, gate_stack_lo, gate_stack_hi (the link script).  Through the
`struct door` it is given, for each system call: write, exit, alloc, read, ms,
load, save.  Exports: gate_run, gate_share_calls, gate_schedule,
gate_space_new, gate_start, gate_resume (gate.h); trap0 to trap31, trap0x80,
trap0x30 to trap0x3f, trap0xff, trap_irq_table, gate_enter, gate_resume_frame
(gate_entry.S); gate_door, gate_return (gate_user.S).

## Every byte it writes anywhere
The descriptor table (6 entries: null, kernel code and data, user code and
data, task state segment), the task state segment (26 words: esp0, ss0, and
the I/O map offset set past the segment's end, so every port is closed to
ring 3), the interrupt table (256 entries), four page directories, one page
table, four kernel stacks (16 KiB each), the frame that resumes each program,
and, for each shared program, two words at the top of its own stack (_start's
argument and its return address); or, for a lone program, the same two words
at the top of the gate's own stack.  The answer to a system call, in eax of
the saved frame.

## Size
gate.c 395 lines, gate_entry.S 131, gate_user.S 88, gate.h 57.  The kernel
file (loader.elf) for the gate flavor: 20,232 bytes (text 8,369, data 124, bss
112,824 of which the kernel stacks, page directories and the gate's own stack
are the most); the stub page is 4,096 bytes of address space, about 150 bytes of it
code and table (gate_return is at offset 0x96).  For the gateshare flavor: 21,028 bytes (text 11,169).
