	for (i = 0; i < n; i++) {
		if (s[i] == '\n') {
			if (con_wrapped) {
				if (i > from)
					vga_write(s + from, (unsigned)(i - from));
				from = i + 1;	/* the line already ended at the edge */
			}
			con_col = 0;
			con_wrapped = 0;
		} else if (s[i] != '\r') {
			con_wrapped = 0;
			if (++con_col == 80) {
				con_col = 0;
				con_wrapped = 1;
			}
		}
	}
	if (n > from)
		vga_write(s + from, (unsigned)(n - from));
}

static void screen_put(const char *s, unsigned long n)
{
	unsigned long i, from = 0;
	unsigned int k;

	for (i = 0; i < n; i++) {
		unsigned char c = (unsigned char)s[i];

		if (con_state == 0) {
			if (c != 0x1b)
				continue;
			if (i > from)
				screen_text(s + from, i - from);
			con_state = 1;
		} else if (con_state == 1) {
			con_state = c == '[' ? 2 : 0;
			con_np = 0;
			con_par[0] = 0;
		} else if (c >= '0' && c <= '9') {
			con_par[con_np] = con_par[con_np] * 10 + (c - '0');
		} else if (c == ';') {
			if (con_np < 7)
				con_par[++con_np] = 0;
		} else {
			if (c == 'm')
				for (k = 0; k <= con_np; k++)
					con_sgr(con_par[k]);
			else if (c == 'H') {
				vga_home();
				con_col = con_wrapped = 0;
			} else if (c == 'J' && con_par[0] == 2) {
				vga_clear();
				con_col = con_wrapped = 0;
			}
			con_state = 0;
		}
		if (con_state == 0)
			from = i + 1;
	}
	if (con_state == 0 && n > from)
		screen_text(s + from, n - from);
}
#endif

/* Every byte to every console. */
void kernel_say(const char *s, unsigned long n)
{
	serial8250_write(CONSOLE_PORT, s, n);
#ifdef HAVE_SCREEN_VGA
	screen_put(s, n);
#endif
}

void kernel_flush(void)
{
	serial8250_flush(CONSOLE_PORT);
}

static void door_write(const void *s, unsigned long n)
{
	unsigned long f = irq_save();

	kernel_say(s, n);
	irq_restore(f);
}

/* Wait for one byte, then take whatever else has already arrived.  Paid
 * TECH-DEBT 2: interrupts are re-enabled during the spin-wait so the timer
 * tick fires and other programs get their turns while this one waits. */
static unsigned long door_read(void *buf, unsigned long n)
{
	unsigned char *p = buf;
	unsigned long got = 0, f;

	if (n == 0)
		return 0;
	__asm__ volatile("sti" : : : "memory");
	while (!serial8250_poll(CONSOLE_PORT))
		;
	f = irq_save();
	p[got++] = serial8250_getc(CONSOLE_PORT);
	while (got < n && serial8250_poll(CONSOLE_PORT))
		p[got++] = serial8250_getc(CONSOLE_PORT);
	irq_restore(f);
	return got;
}

/*
 * The clock, as @4.  khz is how many counter ticks make a millisecond,
 * measured once at boot; start is the counter then.  0 without a clock part.
 */
#if defined(HAVE_TIMER_KVMCLOCK) || defined(HAVE_TIMER_TSC_PIT)
static unsigned long clock_khz;
static unsigned long long clock_start;

/* the processor's counter, read the way Linux's rdtsc() reads it */
static unsigned long long cycles(void)
{
	unsigned long lo, hi;

	__asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
	return ((unsigned long long)hi << 32) | lo;
}
#endif

static unsigned long door_ms(void)
{
#if defined(HAVE_TIMER_KVMCLOCK) || defined(HAVE_TIMER_TSC_PIT)
	unsigned long long t;
	unsigned long hi, lo, q, r;

	if (!clock_khz)
		return 0;
	t = cycles() - clock_start;
	hi = (unsigned long)(t >> 32);
	lo = (unsigned long)t;
	if (hi >= clock_khz)
		return 0xffffffffUL;	/* 49 days and 17 hours: the call's last value, held */
	__asm__("divl %4" : "=a"(q), "=d"(r) : "0"(lo), "1"(hi), "rm"(clock_khz));
	(void)r;
	return q;
#else
	return 0;
#endif
}

static long door_load(const char *name, void *buf, unsigned long max)
{
#ifdef HAVE_FS_MINIX
	unsigned long f = irq_save();
	long r = minix_load(name, buf, max);

	irq_restore(f);
	return r;
#else
	(void)name; (void)buf; (void)max;
	return -1;
#endif
}

static long door_save(const char *name, const void *buf, unsigned long n)
{
#ifdef HAVE_FS_MINIX
	unsigned long f = irq_save();
	long r = minix_save(name, buf, n);

	irq_restore(f);
	return r;
#else
	(void)name; (void)buf; (void)n;
	return -1;
#endif
}

static void __attribute__((noreturn)) door_exit(int code)
{
	(void)code;			/* unobservable from outside the machine; findings 0000 #9 */
#ifdef HAVE_SHARE
	share_exit();			/* resumes the next program, if there is one */
#endif
	__asm__ volatile("cli");
	kernel_flush();
	kernel_halt();
}

/* what the kernel does for each system call; the gate (PART.gate@1) is the only caller */
static const struct door kernel_calls = { door_write, door_exit, door_alloc, door_read, door_ms, door_load, door_save };

void __attribute__((noreturn)) loader_main(const struct hvm_start_info *si)
{
	const struct hvm_modlist_entry *mod;
	const unsigned char *src;
	unsigned char *dst;
	unsigned long n;

	serial8250_init(CONSOLE_PORT);
#ifdef HAVE_SCREEN_VGA
	vga_clear();		/* whatever the firmware left on the screen goes */
#endif
#ifdef HAVE_TIMER_KVMCLOCK
