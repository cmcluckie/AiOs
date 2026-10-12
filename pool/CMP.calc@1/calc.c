/* SPDX-License-Identifier: GPL-2.0-only
 *
 * CMP.calc@1.  PUR.0003: a reverse-Polish calculator at the serial console.
 * Type numbers and operators, it keeps a stack, shows the top after each line,
 * and quits on q.  The input chapter and the stack chapter at once.
 *
 * Requires CAP.tty.read, CAP.tty.write and CAP.proc.exit through the loader's
 * door, and nothing else.  Integers only, 32-bit, wrapping on + - * like the
 * hardware; division by zero and the one division that overflows are refused,
 * because on this machine they would otherwise end it (there is no handler).
 *
 * The console does not echo: the program echoes what you type, handles
 * backspace, and treats Enter as either \r (a terminal) or \n (a pipe).
 */

#include "../CMP.loader@3/door.h"

#define LINE  80
#define DEPTH 64

static const struct door *door;
static int stack[DEPTH];
static int depth;

static void out(const char *s, unsigned long n) { door->write(s, n); }

static void put(const char *s)
{
	unsigned long n = 0;
	while (s[n]) n++;
	out(s, n);
}

static void put_int(int v)
{
	char buf[12];
	int i = 11;
	unsigned int u = v < 0 ? 0u - (unsigned int)v : (unsigned int)v;

	buf[i] = '\0';
	do { buf[--i] = (char)('0' + u % 10); u /= 10; } while (u);
	if (v < 0) buf[--i] = '-';
	put(buf + i);
}

static int same(const char *a, unsigned long n, const char *b)
{
	unsigned long i;
	for (i = 0; i < n; i++)
		if (b[i] != a[i]) return 0;
	return b[n] == '\0';
}

static __attribute__((noreturn)) void bye(void)
{
	put("bye\n");
	door->exit(0);
}

/* Parse a whole token as a decimal integer.  0: not a number.  1: ok.  2: too big. */
static int number(const char *t, unsigned long n, int *v)
{
	unsigned long i = 0, limit = 2147483647UL, acc = 0;
	int neg = 0;

	if (t[0] == '-' && n > 1) { neg = 1; i = 1; limit = 2147483648UL; }
	if (i == n) return 0;
	for (; i < n; i++)
		if (t[i] < '0' || t[i] > '9') return 0;
	for (i = neg; i < n; i++) {
		unsigned long d = (unsigned long)(t[i] - '0');
		/* check before multiplying: on this machine unsigned long is 32 bits,
		 * and checking after lets 99999999999 wrap to 1215752191 */
		if (acc > (limit - d) / 10) return 2;
		acc = acc * 10 + d;
	}
	*v = neg ? (int)(0u - (unsigned int)acc) : (int)acc;
	return 1;
}

static int need(int k)
{
	if (depth >= k) return 1;
	put("error: stack empty\n");
	return 0;
}

/* One token.  Returns 0 to stop the line on an error. */
static int token(const char *t, unsigned long n)
{
	int v, a, b;

	switch (number(t, n, &v)) {
	case 1:
		if (depth == DEPTH) { put("error: stack full\n"); return 0; }
		stack[depth++] = v;
		return 1;
	case 2:
		put("error: too big\n");
		return 0;
	}
	if (same(t, n, "q") || same(t, n, "quit")) bye();
	if (same(t, n, "clear")) { depth = 0; return 1; }
	if (same(t, n, "drop")) { if (!need(1)) return 0; depth--; return 1; }
	if (same(t, n, "dup"))  {
		if (!need(1)) return 0;
		if (depth == DEPTH) { put("error: stack full\n"); return 0; }
		stack[depth] = stack[depth - 1]; depth++; return 1;
	}
	if (same(t, n, "swap")) {
		if (!need(2)) return 0;
		v = stack[depth - 1]; stack[depth - 1] = stack[depth - 2]; stack[depth - 2] = v;
		return 1;
	}
	if (n == 1 && (t[0] == '+' || t[0] == '-' || t[0] == '*' || t[0] == '/' || t[0] == '%')) {
		if (!need(2)) return 0;
		a = stack[depth - 2]; b = stack[depth - 1];
		if ((t[0] == '/' || t[0] == '%') && b == 0) { put("error: divide by zero\n"); return 0; }
		if ((t[0] == '/' || t[0] == '%') && a == -2147483647 - 1 && b == -1) { put("error: overflow\n"); return 0; }
		switch (t[0]) {
		case '+': v = (int)((unsigned int)a + (unsigned int)b); break;
		case '-': v = (int)((unsigned int)a - (unsigned int)b); break;
		case '*': v = (int)((unsigned int)a * (unsigned int)b); break;
		case '/': v = a / b; break;
		default:  v = a % b; break;
		}
		depth--;
		stack[depth - 1] = v;
		return 1;
	}
	put("error: unknown '"); out(t, n); put("'\n");
	return 0;
}

static void line(const char *s, unsigned long n)
{
	unsigned long i = 0, start;

	while (i < n) {
		while (i < n && s[i] == ' ') i++;
		if (i == n) break;
		start = i;
		while (i < n && s[i] != ' ') i++;
		if (!token(s + start, i - start)) return;
	}
	if (depth) { put("= "); put_int(stack[depth - 1]); put("\n"); }
	else put("(empty)\n");
}

void __attribute__((section(".text.start"), noreturn)) _start(const struct door *d)
{
	char buf[LINE], c;
	unsigned long len = 0;
	int last_cr = 0;

	door = d;
	put("RPN calculator.  Numbers and + - * / %, dup swap drop clear, q to quit.\n> ");
	for (;;) {
		door->read(&c, 1);
		if (c == '\n' && last_cr) { last_cr = 0; continue; }	/* \r\n is one Enter */
		last_cr = (c == '\r');
		if (c == '\r' || c == '\n') {
			put("\n");
			line(buf, len);
			len = 0;
			put("> ");
		} else if (c == 3 || c == 4) {			/* Ctrl-C, Ctrl-D */
			put("\n");
			bye();
		} else if (c == 8 || c == 127) {		/* backspace */
			if (len) { len--; put("\b \b"); }
		} else if (c >= ' ' && c <= '~' && len < LINE) {
			buf[len++] = c;
			out(&c, 1);
		}
	}
}
