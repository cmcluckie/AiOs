/* SPDX-License-Identifier: GPL-2.0-only
 *
 * AGT.referee@1: the referee, in C.  No model, no judgment.  It reads the
 * constitution and the catalogue and says yes or no, naming a rule and a
 * catalogue number.  The first agent built, the one never stripped: it
 * decides what is stripped.  Rewritten from the shell version (AGT.referee@1)
 * on 2026-10-15, because the referee is permanent code and C is the language
 * of everything permanent in AiOs; the shell version was the spec by example,
 * and this one passes the same four probes.
 *
 *   referee check <flavor>        rules 1, 9, 19: every code resolves, not
 *                                 expired, two programs need a sharing loader
 *   referee hardware <code> <src> rule 12: the program's own assembly, against
 *                                 constitution/instructions.toml
 *   referee rules                 rules 2 to 8 and 21 on every petition, written to
 *                                 tests/referee-results.txt; any FAIL exits 1
 *   referee lines <file>          rule 13: a committee's or mechanic's output is
 *                                 a numbered list of verb-noun lines
 *   referee petition <file>       a draft petition is well-formed: it has the
 *                                 fields, every code is a catalogue number that
 *                                 resolves or comes with a component not yet
 *                                 admitted (rule 1), capability reasons are SPEC
 *                                 codes (rule 2), it has not expired (rule 9)
 *   referee artifact <flavor>     rule 11, on the kernel as linked: every part in
 *                                 it listed, every capability it exposes asked
 *                                 for, switched off and absent (nm), or listed in
 *                                 constitution/extras.toml; run by the builder
 *   referee probe <flavor> <src>  rule 22 for one test program on one flavor
 *   referee probes                rule 22 for every test program in tests/programs
 *
 * The ledger (decisions against the diff) stays a jig script, jig/ledger: it
 * reads git history, which the sealed system will not have; there, admission
 * does that job.
 *
 * Failures name rules and codes, never tool output.  Runs on the jig today,
 * against the repository as files; libc for files and popen for the
 * compiler, git and nm.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAXS 4096
#define MAXL 256
#define PATHL 1024	/* a path built from codes; MAXL names, never overflowing this */

static void die(const char *fmt, ...) __attribute__((format(printf, 1, 2), noreturn));
static void die(const char *fmt, ...)
{
	va_list ap;
	fputs("refused: ", stderr);
	va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
	fputc('\n', stderr);
	exit(1);
}

/* ---- files and the flat TOML the schema allows ---- */

static char *slurp(const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
	char *b = malloc(n + 1);
	if (fread(b, 1, n, f) != (size_t)n) { fclose(f); free(b); return NULL; }
	b[n] = 0; fclose(f);
	return b;
}

static int exists(const char *path) { struct stat st; return stat(path, &st) == 0; }

/* the first `key = "value"` in text (from the start, or from a section) */
static int field_in(const char *text, const char *key, char *out, size_t n)
{
	size_t kl = strlen(key);
	for (const char *p = text; p && *p; p = strchr(p, '\n') ? strchr(p, '\n') + 1 : NULL) {
		if (strncmp(p, key, kl) != 0) continue;
		const char *q = p + kl;
		while (*q == ' ' || *q == '\t') q++;
		if (*q != '=') continue;
		q++;
		while (*q == ' ' || *q == '\t') q++;
		if (*q != '"') continue;
		q++;
		size_t i = 0;
		while (*q && *q != '"' && *q != '\n' && i + 1 < n) out[i++] = *q++;
		out[i] = 0;
		return 1;
	}
	return 0;
}

/* `key = [ "a", "b" ]` -> items, NUL-separated, count returned */
static int list_in(const char *text, const char *key, char items[][MAXL], int max)
{
	size_t kl = strlen(key);
	for (const char *p = text; p && *p; p = strchr(p, '\n') ? strchr(p, '\n') + 1 : NULL) {
		if (strncmp(p, key, kl) != 0) continue;
		const char *q = p + kl;
		while (*q == ' ' || *q == '\t') q++;
		if (*q != '=') continue;
		q++;
		while (*q == ' ' || *q == '\t') q++;
		if (*q != '[') continue;
		int n = 0;
		while (*q && *q != ']' && *q != '\n') {
			if (*q == '"') {
				q++;
				size_t i = 0;
				while (*q && *q != '"' && i + 1 < MAXL) items[n][i++] = *q++;
				items[n][i] = 0;
				if (n < max) n++;
			}
			if (*q) q++;
		}
		return n;
	}
	return 0;
}

static int field(const char *path, const char *key, char *out, size_t n)
{
	char *t = slurp(path);
	if (!t) { out[0] = 0; return 0; }
	int r = field_in(t, key, out, n);
	if (!r) out[0] = 0;
	free(t);
	return r;
}

static int list(const char *path, const char *key, char items[][MAXL], int max)
{
	char *t = slurp(path);
	if (!t) return 0;
	int n = list_in(t, key, items, max);
	free(t);
	return n;
}

/* code -> file.  PART.loader@1 -> index/part/loader@1.toml */
static int code_file(const char *code, char *out, size_t n)
{
	const char *dot = strchr(code, '.');
	if (!dot) { out[0] = 0; return 0; }
	char table[16], rest[MAXL];
	snprintf(table, sizeof table, "%.*s", (int)(dot - code), code);
	snprintf(rest, sizeof rest, "%s%s", dot + 1, strchr(dot + 1, '@') ? "" : "@1");
	const char *dir =
		!strcmp(table, "PUR") ? "constitution/purposes" :
		!strcmp(table, "KEY") ? "constitution/keys" :
		!strcmp(table, "CAP") ? "index/cap" :
		!strcmp(table, "PART") ? "index/part" :
		!strcmp(table, "MACHINE") ? "index/machine" :
		!strcmp(table, "TOOL") ? "index/tool" :
		!strcmp(table, "AGT") ? "index/agent" :
		!strcmp(table, "SPEC") ? "specs" :
		!strcmp(table, "TST") ? "tests" :
		!strcmp(table, "SNS") ? "sensors" : NULL;
	if (!strcmp(table, "FND")) {
		char *at = strchr(rest, '@'); if (at) *at = 0;
		snprintf(out, n, "findings/%s.md", rest);
		return 1;
	}
	if (!dir) { out[0] = 0; return 0; }
	snprintf(out, n, "%s/%s.toml", dir, rest);
	return 1;
}

static int is_code_char(char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '@'; }

/* every "TABLE.path@v" in quotes anywhere in a file */
static int codes_in(const char *text, char items[][MAXL], int max)
{
	static const char *tables[] = { "PUR", "KEY", "CAP", "PART", "MACHINE", "TOOL", "AGT", "SPEC", "TST", "SNS", "FND", NULL };
	int n = 0;
	for (const char *p = text; (p = strchr(p, '"')); p++) {
		const char *q = p + 1;
		int t;
		for (t = 0; tables[t]; t++) {
			size_t tl = strlen(tables[t]);
			if (!strncmp(q, tables[t], tl) && q[tl] == '.') break;
		}
		if (!tables[t]) continue;
		const char *e = q + strlen(tables[t]) + 1;
		while (is_code_char(*e)) e++;
		if (*e != '"') continue;
		char code[MAXL];
		snprintf(code, sizeof code, "%.*s", (int)(e - q), q);
		int dup = 0;
		for (int i = 0; i < n; i++) if (!strcmp(items[i], code)) dup = 1;
		if (!dup && n < max) strcpy(items[n++], code);
		p = e;
	}
	return n;
}

/* ---- the rows of a petition: capabilities, components, sensors ---- */

struct row { char kind[16], code[MAXL], a[MAXL], b[MAXL]; };

static int rows_of(const char *text, struct row *rows, int max)
{
	int n = 0; struct row cur; int open = 0;
	for (const char *p = text; p && *p; p = strchr(p, '\n') ? strchr(p, '\n') + 1 : NULL) {
		if (!strncmp(p, "[[", 2)) {
			if (open && n < max) rows[n++] = cur;
			memset(&cur, 0, sizeof cur);
			const char *e = strstr(p, "]]");
			snprintf(cur.kind, sizeof cur.kind, "%.*s", e ? (int)(e - p - 2) : 0, p + 2);
			open = !strcmp(cur.kind, "capability") || !strcmp(cur.kind, "component") || !strcmp(cur.kind, "sensor");
			continue;
		}
		if (!open) continue;
		char v[MAXL];
		if (field_in(p, "code", v, sizeof v) && !strncmp(p, "code", 4)) strcpy(cur.code, v);
		else if (!strncmp(p, "reason", 6) && field_in(p, "reason", v, sizeof v)) strcpy(cur.a, v);
		else if (!strncmp(p, "admitted", 8)) { const char *q = strchr(p, '='); if (q) { q++; while (*q == ' ') q++; snprintf(cur.a, sizeof cur.a, "%.*s", (int)strcspn(q, " \t\r\n"), q); } }
		else if (!strncmp(p, "role", 4) && field_in(p, "role", v, sizeof v)) strcpy(cur.b, v);
		else if (!strncmp(p, "binds", 5) && field_in(p, "binds", v, sizeof v)) strcpy(cur.a, v);
		else if (!strncmp(p, "consumer", 8) && field_in(p, "consumer", v, sizeof v)) strcpy(cur.b, v);
	}
	if (open && n < max) rows[n++] = cur;
	return n;
}

/* ---- a set of strings ---- */

struct set { char items[MAXS][MAXL]; int n; };
static int in_set(struct set *s, const char *x) { for (int i = 0; i < s->n; i++) if (!strcmp(s->items[i], x)) return 1; return 0; }
static void add_set(struct set *s, const char *x) { if (!in_set(s, x) && s->n < MAXS) strcpy(s->items[s->n++], x); }

/* the version after the @, 1 if there is none */
static int version_of(const char *code) { const char *at = strchr(code, '@'); return at ? atoi(at + 1) : 1; }

/* Is this component a kernel?  (kind = "kernel" in its catalogue entry) */
static int is_kernel(const char *code)
{
	char cf[PATHL], kind[MAXL];
	if (!code_file(code, cf, sizeof cf)) return 0;
	field(cf, "kind", kind, sizeof kind);
	return !strcmp(kind, "kernel");
}

static int cmpstr(const void *a, const void *b) { return strcmp((const char *)a, (const char *)b); }

static int listdir(const char *dir, const char *suffix, char names[][MAXL], int max)
{
	DIR *d = opendir(dir);
	if (!d) return 0;
	struct dirent *e; int n = 0;
	size_t sl = strlen(suffix);
	while ((e = readdir(d)) && n < max) {
		size_t l = strlen(e->d_name);
		if (l > sl && !strcmp(e->d_name + l - sl, suffix)) strcpy(names[n++], e->d_name);
	}
	closedir(d);
	qsort(names, n, MAXL, cmpstr);
	return n;
}

/* ---- check: rules 1, 9 and 19 on a flavor ---- */

static void today(char *out)
{
	time_t t = time(NULL); struct tm *tm = gmtime(&t);
	strftime(out, 16, "%Y-%m-%d", tm);
}

/* the loader for a set of programs: the newest one any of them depends on */
static void loader_for(char primaries[][MAXL], int np, char *out)
{
	out[0] = 0; int best = -1;
	for (int i = 0; i < np; i++) {
		char cf[PATHL], deps[64][MAXL];
		code_file(primaries[i], cf, sizeof cf);
		int nd = list(cf, "depends", deps, 64);
		for (int j = 0; j < nd; j++) {
			const char *at = strchr(deps[j], '@');
			int v = at ? atoi(at + 1) : 1;
			if (v > best) { best = v; strcpy(out, deps[j]); }
		}
	}
}

static int cmd_check(const char *flavor)
{
	char ff[PATHL], machine[MAXL], mf[PATHL], pets[32][MAXL], primaries[32][MAXL];
	int np = 0;
	snprintf(ff, sizeof ff, "flavors/%s.toml", flavor);
	if (!exists(ff)) die("no flavor named %s", flavor);
	field(ff, "machine", machine, sizeof machine);
	code_file(machine, mf, sizeof mf);
	if (!exists(mf)) die("missing %s", machine);
	int npet = list(ff, "petitions", pets, 32);
	char now[16]; today(now);
	for (int i = 0; i < npet; i++) {
		char pf[PATHL]; snprintf(pf, sizeof pf, "petitions/%s.toml", pets[i]);
		char *t = slurp(pf);
		if (!t) die("missing petition %s", pets[i]);
		static char codes[MAXS][MAXL];
		int nc = codes_in(t, codes, MAXS);
		for (int c = 0; c < nc; c++) {
			char cf[PATHL];
			if (!code_file(codes[c], cf, sizeof cf)) die("rule 1: %s is not a catalogue number", codes[c]);
			if (!exists(cf)) die("rule 1: missing %s (petition %s)", codes[c], pets[i]);
		}
		/* expires = 2027-04-13, a bare date */
		const char *ex = strstr(t, "\nexpires");
		char exp[16] = "";
		if (ex) { const char *q = strchr(ex, '='); if (q) { q++; while (*q == ' ') q++; snprintf(exp, sizeof exp, "%.10s", q); } }
		if (strcmp(exp, now) <= 0) die("rule 9: petition %s expired %s", pets[i], exp);
		int pending = 0;
		for (const char *p = t; (p = strstr(p, "admitted = false")); p++) pending++;
		static struct row rows[256];
		int nr = rows_of(t, rows, 256);
		for (int r = 0; r < nr; r++)
			if (!strcmp(rows[r].kind, "component") && !strcmp(rows[r].b, "primary") && np < 32) strcpy(primaries[np++], rows[r].code);
		printf("petition %s: well-formed, %d component(s) pending admission\n", pets[i], pending);
		free(t);
	}
	if (np > 1) {
		/* the kernel is the newest boot component the petitions name (as jig/aios
		 * builds it), else the newest kernel the programs depend on; sharing is
		 * there if the kernel or any part a petition names exposes it */
		char loader[MAXL], lf[PATHL], ex[64][MAXL];
		int ok = 0;
		loader[0] = 0;
		for (int i = 0; i < npet; i++) {
			char pf[PATHL]; snprintf(pf, sizeof pf, "petitions/%s.toml", pets[i]);
			char *t = slurp(pf);
			static struct row rows[256];
			int nr = rows_of(t, rows, 256);
			for (int r = 0; r < nr; r++) {
				if (strcmp(rows[r].kind, "component")) continue;
				if (!strcmp(rows[r].b, "boot") && (!loader[0] || version_of(rows[r].code) > version_of(loader))) strcpy(loader, rows[r].code);
				if (!strcmp(rows[r].b, "dependency")) {
					char cf[PATHL]; code_file(rows[r].code, cf, sizeof cf);
					int ne = list(cf, "exposes", ex, 64);
					for (int e = 0; e < ne; e++) if (!strncmp(ex[e], "CAP.cpu.share@", 14)) ok = 1;
				}
			}
			free(t);
		}
		if (!loader[0]) loader_for(primaries, np, loader);
		code_file(loader, lf, sizeof lf);
		int ne = list(lf, "exposes", ex, 64);
		for (int i = 0; i < ne; i++) if (!strncmp(ex[i], "CAP.cpu.share@", 14)) ok = 1;
		if (!ok) die("rule 19: flavor %s needs %d programs on one machine; %s runs one (CAP.cpu.share, plan 9.6)", flavor, np, loader);
	}
	printf("flavor %s: buildable on %s, program", flavor, machine);
	for (int i = 0; i < np; i++) printf(" %s", primaries[i]);
	printf("\n");
	return 0;
}

/* ---- artifact: rule 11, the kernel as linked, held to the petitions ---- */

/* a flavor: its petitions, what they ask for, the components they list, and the
 * kernel the builder builds for it (the newest boot component, else the newest
 * kernel its programs depend on) */
struct flavor { char pets[32][MAXL]; int npet; char kernel[MAXL]; };
static struct set asked, listed;

static void read_flavor(const char *name, struct flavor *f)
{
	char ff[PATHL], primaries[32][MAXL];
	int np = 0;
	snprintf(ff, sizeof ff, "flavors/%s.toml", name);
	if (!exists(ff)) die("no flavor named %s", name);
	f->npet = list(ff, "petitions", f->pets, 32);
	f->kernel[0] = 0;
	asked.n = listed.n = 0;
	for (int i = 0; i < f->npet; i++) {
		char pf[PATHL]; snprintf(pf, sizeof pf, "petitions/%s.toml", f->pets[i]);
		char *t = slurp(pf);
		if (!t) die("missing petition %s", f->pets[i]);
		static struct row rows[256];
		int nr = rows_of(t, rows, 256);
		for (int r = 0; r < nr; r++) {
			if (!strcmp(rows[r].kind, "capability")) add_set(&asked, rows[r].code);
			if (strcmp(rows[r].kind, "component")) continue;
			add_set(&listed, rows[r].code);
			if (!strcmp(rows[r].b, "boot") && (!f->kernel[0] || version_of(rows[r].code) >= version_of(f->kernel))) strcpy(f->kernel, rows[r].code);
			if (!strcmp(rows[r].b, "primary") && np < 32) strcpy(primaries[np++], rows[r].code);
		}
		free(t);
	}
	if (!f->kernel[0]) loader_for(primaries, np, f->kernel);
}

/* constitution/extras.toml: what each flavor's kernel exposed that none of its
 * petitions asked for, when rule 11 was first checked (decision 0065).  It only
 * shrinks. */
static void extras_of(const char *flavor, struct set *out)
{
	out->n = 0;
	char *t = slurp("constitution/extras.toml");
	if (!t) return;
	char fl[MAXL] = "", cap[MAXL] = "", v[MAXL];
	int open = 0;
	for (const char *p = t; p && *p; p = strchr(p, '\n') ? strchr(p, '\n') + 1 : NULL) {
		if (!strncmp(p, "[[", 2)) {
			if (open && !strcmp(fl, flavor) && cap[0]) add_set(out, cap);
			open = !strncmp(p, "[[extra]]", 9);
			fl[0] = cap[0] = 0;
			continue;
		}
		if (!open) continue;
		if (!strncmp(p, "flavor", 6) && field_in(p, "flavor", v, sizeof v)) strcpy(fl, v);
		else if (!strncmp(p, "cap ", 4) && field_in(p, "cap", v, sizeof v)) strcpy(cap, v);
	}
	if (open && !strcmp(fl, flavor) && cap[0]) add_set(out, cap);
	free(t);
}

static int cmd_artifact(const char *flavor)
{
	struct flavor f;
	static struct set parts, syms, exposed, extras, kept;
	char elf[PATHL], kp[PATHL], kf[PATHL], cf[PATHL], items[64][MAXL];
	read_flavor(flavor, &f);
	snprintf(elf, sizeof elf, "out/%s/loader.elf", flavor);
	snprintf(kp, sizeof kp, "out/%s/kernel-parts.txt", flavor);
	if (!exists(elf)) die("rule 11: flavor %s has no kernel built, %s", flavor, elf);
	/* the parts in the kernel: the builder's list when it chose them (plan 5.8),
	 * else everything the kernel depends on, as it was built before @8 */
	parts.n = 0;
	char *t = slurp(kp);
	if (t) {
		for (char *l = strtok(t, "\n"); l; l = strtok(NULL, "\n")) if (*l) add_set(&parts, l);
		free(t);
	} else {
		code_file(f.kernel, kf, sizeof kf);
		int nd = list(kf, "depends", items, 64);
		for (int j = 0; j < nd; j++) add_set(&parts, items[j]);
	}
	for (int k = 0; k < parts.n; k++) {
		code_file(parts.items[k], cf, sizeof cf);
		int nd = list(cf, "depends", items, 64);
		for (int j = 0; j < nd; j++) add_set(&parts, items[j]);
	}
	for (int k = 0; k < parts.n; k++)
		if (!in_set(&listed, parts.items[k])) die("rule 11: flavor %s's kernel holds %s, which none of its petitions lists", flavor, parts.items[k]);
	/* what is in the image, by name */
	syms.n = 0;
	char cmd[PATHL + 16]; snprintf(cmd, sizeof cmd, "nm '%s' 2>/dev/null", elf);
	FILE *nm = popen(cmd, "r");
	if (!nm) die("rule 11: cannot list the symbols of %s", elf);
	char line[MAXS];
	while (fgets(line, sizeof line, nm)) {
		char a[MAXL], ty[8], name[MAXL];
		if (sscanf(line, "%255s %7s %255s", a, ty, name) == 3 && strcmp(ty, "U")) add_set(&syms, name);
	}
	if (pclose(nm) != 0 || !syms.n) die("rule 11: nm could not read %s", elf);
	/* every capability the kernel and its parts expose, and the ones switched off */
	exposed.n = 0;
	static char off_cap[64][MAXL], off_sym[64][MAXL];
	int noff = 0;
	add_set(&parts, f.kernel);
	for (int k = 0; k < parts.n; k++) {
		code_file(parts.items[k], cf, sizeof cf);
		int ne = list(cf, "exposes", items, 64);
		for (int j = 0; j < ne; j++) add_set(&exposed, items[j]);
		ne = list(cf, "off_unless_asked", items, 64);
		for (int j = 0; j < ne && noff < 64; j++)
			if (sscanf(items[j], "%255s %255s", off_cap[noff], off_sym[noff]) == 2) noff++;
	}
	extras_of(flavor, &extras);
	kept.n = 0;
	int off = 0;
	for (int k = 0; k < exposed.n; k++) {
		const char *c = exposed.items[k];
		int o = -1;
		for (int j = 0; j < noff; j++) if (!strcmp(off_cap[j], c)) o = j;
		if (o >= 0) {
			int in = in_set(&syms, off_sym[o]);
			if (in_set(&asked, c) && !in) die("rule 11: flavor %s asks for %s, and %s is not in its kernel", flavor, c, off_sym[o]);
			if (!in_set(&asked, c) && in) die("rule 11: flavor %s does not ask for %s, which its kernel switches off unless asked, and %s is in it", flavor, c, off_sym[o]);
			if (!in_set(&asked, c)) { off++; printf("%s: switched off, not asked for\n", c); }
			continue;
		}
		if (in_set(&asked, c)) continue;
		if (in_set(&extras, c)) { add_set(&kept, c); printf("%s: not asked for; listed in constitution/extras.toml\n", c); continue; }
		die("rule 11: flavor %s's kernel exposes %s, which none of its petitions asks for", flavor, c);
	}
	for (int k = 0; k < extras.n; k++)
		if (!in_set(&kept, extras.items[k])) die("rule 11: constitution/extras.toml lists %s for flavor %s, whose kernel no longer exposes it unasked; take the line off", extras.items[k], flavor);
	printf("flavor %s: %s and %d part(s), all listed; nothing exposed unasked but %d switched off and %d in constitution/extras.toml\n", flavor, f.kernel, parts.n - 1, off, kept.n);
	return 0;
}

/* ---- probes: rule 22, a test program boots only where it is granted ---- */

/* the system call table of a kernel, from its door.h: each entry's name and the
 * capability its comment names, without the version */
static int door_of(const char *kernel, char names[][MAXL], char caps[][MAXL], int max)
{
	char kf[PATHL], src[MAXL], dh[PATHL];
	code_file(kernel, kf, sizeof kf);
	field(kf, "source", src, sizeof src);
	snprintf(dh, sizeof dh, "%sdoor.h", src);
	char *t = slurp(dh);
	if (!t) return 0;
	int n = 0;
	for (char *p = t; (p = strstr(p, "(*")) && n < max; p += 2) {
		char *e = strchr(p, ')'), *nl = strchr(p, '\n'), *c = strstr(p, "/* CAP.");
		if (!e || !nl || !c || c > nl) continue;
		snprintf(names[n], MAXL, "%.*s", (int)(e - p - 2), p + 2);
		c += 3;
		snprintf(caps[n], MAXL, "%.*s", (int)strcspn(c, ": "), c);
		n++;
	}
	free(t);
	return n;
}

static int granted(const char *cap, struct set *extras)
{
	size_t l = strlen(cap);
	for (int i = 0; i < asked.n; i++) if (!strncmp(asked.items[i], cap, l) && asked.items[i][l] == '@') return 1;
	for (int i = 0; i < extras->n; i++) if (!strncmp(extras->items[i], cap, l) && extras->items[i][l] == '@') return 1;
	return 0;
}

/* SRC, a test program, booted on FLAVOR's kernel: the number of refusals */
static int probe_one(const char *flavor, const char *src, const char *text)
{
	struct flavor f;
	static struct set extras;
	char names[16][MAXL], caps[16][MAXL], refuse[16][MAXL];
	int bad = 0, nref = 0;
	read_flavor(flavor, &f);
	extras_of(flavor, &extras);
	int nd = door_of(f.kernel, names, caps, 16);
	if (!nd) { printf("rule 22: %s boots on flavor %s, whose kernel %s has no door.h\n", src, flavor, f.kernel); return 1; }
	/* "Expects refusal: CAP.fs.write." names capabilities without versions; the
	 * line's full stop is not part of the last one */
	const char *r = strstr(text, "Expects refusal:");
	if (r) {
		const char *nl = strchr(r, '\n');
		for (const char *p = r; (p = strstr(p, "CAP.")) && (!nl || p < nl) && nref < 16; p += 4) {
			size_t l = strspn(p, "CAP.abcdefghijklmnopqrstuvwxyz0123456789-");
			while (l && p[l - 1] == '.') l--;
			snprintf(refuse[nref++], MAXL, "%.*s", (int)l, p);
		}
	}
	int used[16] = { 0 };
	for (int k = 0; k < nd; k++) {
		char call[MAXL + 4]; snprintf(call, sizeof call, "->%s(", names[k]);
		if (!strstr(text, call)) continue;
		int ref = -1;
		for (int i = 0; i < nref; i++) if (!strcmp(refuse[i], caps[k])) ref = i;
		if (ref >= 0) used[ref] = 1;
		if (granted(caps[k], &extras) && ref >= 0) { printf("rule 22: %s says it expects %s refused on flavor %s, whose petitions ask for it\n", src, caps[k], flavor); bad++; }
		if (!granted(caps[k], &extras) && ref < 0) { printf("rule 22: %s calls %s (%s) on flavor %s, whose petitions do not ask for it\n", src, names[k], caps[k], flavor); bad++; }
	}
	for (int i = 0; i < nref; i++) if (!used[i]) { printf("rule 22: %s says it expects %s refused and makes no call that needs it\n", src, refuse[i]); bad++; }
	return bad;
}

static int cmd_probe(const char *flavor, const char *src)
{
	char *t = slurp(src);
	if (!t) die("no test program %s", src);
	int bad = probe_one(flavor, src, t);
	free(t);
	if (bad) return 1;
	printf("%s on flavor %s: every call it makes is asked for, or expected refused\n", src, flavor);
	return 0;
}

/* every test program written against the system call table, and the probe
 * scripts that boot them */
static int cmd_probes(void)
{
	static char cs[256][MAXL], shs[256][MAXL];
	int nc = listdir("tests/programs", ".c", cs, 256), ns = listdir("tests/programs", ".sh", shs, 256);
	int bad = 0, checked = 0;
	static char *sh[256];
	for (int i = 0; i < ns; i++) { char p[PATHL]; snprintf(p, sizeof p, "tests/programs/%s", shs[i]); sh[i] = slurp(p); }
	for (int i = 0; i < nc; i++) {
		char src[PATHL], fl[16][MAXL];
		snprintf(src, sizeof src, "tests/programs/%s", cs[i]);
		char *t = slurp(src);
		if (!t || !strstr(t, "door.h\"")) { free(t); continue; }
		checked++;
		int nf = 0, nowhere = 0;
		const char *b = strstr(t, "Boots on:");
		if (!b) { printf("rule 22: %s has no Boots on line: which flavors' kernels it boots on\n", src); bad++; free(t); continue; }
		b += 9;
		while (*b == ' ') b++;
		if (!strncmp(b, "no flavor", 9)) nowhere = 1;	/* "Boots on: no flavor's kernel." */
		const char *end = nowhere ? b : b + strcspn(b, "\n");
		while (b < end && nf < 16) {
			while (b < end && (*b == ' ' || *b == ',')) b++;
			size_t l = 0;
			while (b + l < end && ((b[l] >= 'a' && b[l] <= 'z') || (b[l] >= '0' && b[l] <= '9') || b[l] == '-')) l++;
			if (!l) break;
			snprintf(fl[nf++], MAXL, "%.*s", (int)l, b);
			b += l;
			if (*b == '.') break;
		}
		if (!nf && !nowhere) { printf("rule 22: %s names no flavor on its Boots on line\n", src); bad++; }
		for (int k = 0; k < nf; k++) bad += probe_one(fl[k], src, t);
		/* a script that boots it on exactly one flavor's kernel boots it on one it names */
		for (int s2 = 0; s2 < ns; s2++) {
			if (!sh[s2] || !strstr(sh[s2], src)) continue;
			if (strstr(sh[s2], "${AIOS_SHARE")) continue;
			char seen[16][MAXL]; int nseen = 0;
			for (const char *p = sh[s2]; (p = strstr(p, "out/")); p += 4) {
				size_t l = strspn(p + 4, "abcdefghijklmnopqrstuvwxyz0123456789-");
				if (!l || strncmp(p + 4 + l, "/loader.elf", 11)) continue;
				char nm2[MAXL]; snprintf(nm2, sizeof nm2, "%.*s", (int)l, p + 4);
				int dup = 0; for (int q = 0; q < nseen; q++) if (!strcmp(seen[q], nm2)) dup = 1;
				if (!dup && nseen < 16) strcpy(seen[nseen++], nm2);
			}
			if (nseen != 1) continue;
			int ok = 0; for (int k = 0; k < nf; k++) if (!strcmp(fl[k], seen[0])) ok = 1;
			if (!ok) { printf("rule 22: tests/programs/%s boots %s on flavor %s's kernel, and %s does not say it boots there\n", shs[s2], src, seen[0], src); bad++; }
		}
		free(t);
	}
	for (int i = 0; i < ns; i++) free(sh[i]);
	if (bad) { printf("%d refusal(s)\n", bad); return 1; }
	printf("%d test programs: each boots only where every call it makes is asked for, listed, or expected refused\n", checked);
	return 0;
}

/* ---- hardware: rule 12 on a program's own assembly ---- */

static int cmd_hardware(const char *code, const char *src)
{
	char *il = slurp("constitution/instructions.toml");
	if (!il) die("no constitution/instructions.toml");
	static char forb[64][MAXL]; int nf = 0, cr = 0;
	for (const char *p = il; p && *p; p = strchr(p, '\n') ? strchr(p, '\n') + 1 : NULL) {
		if (*p < 'a' || *p > 'z') continue;
		size_t l = strspn(p, "abcdefghijklmnopqrstuvwxyz");
		const char *q = p + l; while (*q == ' ') q++;
		if (*q != '=') continue;
		if (l == 2 && !strncmp(p, "cr", 2)) { cr = 1; continue; }
		if (nf < 64) snprintf(forb[nf++], MAXL, "%.*s", (int)l, p);
	}
	free(il);
	const char *cc = getenv("CC") ? getenv("CC") : "gcc";
	const char *cflags = getenv("CFLAGS") ? getenv("CFLAGS") : "";
	char cmd[MAXS];
	snprintf(cmd, sizeof cmd, "%s %s -S -o - '%s'", cc, cflags, src);
	FILE *f = popen(cmd, "r");
	if (!f) die("rule 12: cannot run the compiler");
	char line[MAXS], hit[MAXS] = "";
	while (!hit[0] && fgets(line, sizeof line, f)) {
		const char *p = line; while (*p == ' ' || *p == '\t') p++;
		size_t l = strspn(p, "abcdefghijklmnopqrstuvwxyz");
		for (int i = 0; i < nf; i++) {
			size_t fl = strlen(forb[i]);
			if (l < fl || strncmp(p, forb[i], fl)) continue;
			size_t rest = l - fl;
			if (rest == 0 || (rest == 1 && strchr("bwl", p[fl]))) {
				char after = p[l];
				if (after == ' ' || after == '\t' || after == '\n' || after == '\r' || after == 0) { snprintf(hit, sizeof hit, "%.*s", (int)strcspn(p, "\r\n"), p); break; }
			}
		}
		if (!hit[0] && cr && !strncmp(p, "mov", 3) && strstr(p, "%cr") && strspn(strstr(p, "%cr") + 3, "0123456789")) snprintf(hit, sizeof hit, "%.*s", (int)strcspn(p, "\r\n"), p);
	}
	int st = pclose(f);
	if (hit[0]) die("rule 12: %s touches the hardware itself (%s); a program asks the system, through its system calls", code, hit);
	/* fail closed: a program the compiler would not turn into instructions was
	 * never looked at, so it is not passed (findings 0019 #6) */
	if (st != 0) die("rule 12: %s could not be compiled to read its instructions; nothing was checked, so it is refused", code);
	return 0;
}

/* ---- rules: 2 to 8 on every petition, written to tests/referee-results.txt ---- */

static FILE *logf; static int fails;

static void report(int rule, const char *id, const char *bad)
{
	if (bad[0]) { fprintf(logf, "FAIL  rule %d  petition %s  %s\n", rule, id, bad); fails++; }
	else fprintf(logf, "PASS  rule %d  petition %s\n", rule, id);
}

static int cmd_rules(void)
{
	const char *path = "tests/referee-results.txt";
	logf = fopen(path, "w");
	if (!logf) die("cannot write %s", path);
	char commit[64] = "not a git checkout";
	FILE *g = popen("git rev-parse --short HEAD 2>/dev/null", "r");
	if (g) { if (fgets(commit, sizeof commit, g)) commit[strcspn(commit, "\n")] = 0; pclose(g); }
	char stamp[32]; time_t t = time(NULL); strftime(stamp, sizeof stamp, "%Y-%m-%dT%H:%MZ", gmtime(&t));
	fprintf(logf, "# tests/referee-results.txt, written by the referee (AGT.referee@1, rules).  Do not edit by hand.\n");
	fprintf(logf, "# run %s at commit %s: rules 2 to 8 and 21 of schema/petition.md, every petition\n", stamp, commit);
	static char pets[256][MAXL], specs[256][MAXL], tests[256][MAXL];
	int npet = listdir("petitions", ".toml", pets, 256);
	int nspec = listdir("specs", ".toml", specs, 256);
	int ntest = listdir("tests", ".toml", tests, 256);
	static struct set bound; bound.n = 0;
	for (int i = 0; i < ntest; i++) { char tf[PATHL], b[MAXL]; snprintf(tf, sizeof tf, "tests/%s", tests[i]); field(tf, "binds", b, sizeof b); add_set(&bound, b); }
	static struct row rows[256];
	static struct set caps, comps, all, exposed;
	for (int i = 0; i < npet; i++) {
		char pf[PATHL], id[MAXL]; snprintf(pf, sizeof pf, "petitions/%s", pets[i]);
		char *t = slurp(pf);
		field_in(t, "id", id, sizeof id);
		int nr = rows_of(t, rows, 256);
		caps.n = comps.n = all.n = exposed.n = 0;
		char primary[MAXL] = "", boot[MAXL] = "";
		for (int r = 0; r < nr; r++) {
			if (!strcmp(rows[r].kind, "capability")) add_set(&caps, rows[r].code);
			if (!strcmp(rows[r].kind, "component")) { add_set(&comps, rows[r].code); add_set(&all, rows[r].code); if (!strcmp(rows[r].b, "primary")) strcpy(primary, rows[r].code); if (!strcmp(rows[r].b, "boot")) strcpy(boot, rows[r].code); }
		}
		/* the components and everything they depend on, walked once.  A program
		 * depends on the kernel it was built for; it runs on the petition's boot
		 * kernel instead when that one is newer (the ladder, SPEC.loader.4@1), so
		 * that is the kernel walked (plan 5.8). */
		for (int k = 0; k < all.n; k++) {
			char cf[PATHL], deps[64][MAXL];
			code_file(all.items[k], cf, sizeof cf);
			int nd = list(cf, "depends", deps, 64);
			for (int j = 0; j < nd; j++) {
				if (boot[0] && strcmp(deps[j], boot) && is_kernel(deps[j]) && version_of(boot) >= version_of(deps[j]))
					add_set(&all, boot);
				else
					add_set(&all, deps[j]);
			}
		}
		char bad[MAXS * 2];
		/* rule 2: every capability's reason is a SPEC clause of a component in the petition */
		bad[0] = 0;
		for (int r = 0; r < nr && !bad[0]; r++) {
			if (strcmp(rows[r].kind, "capability")) continue;
			char sf[PATHL], comp[MAXL];
			if (strncmp(rows[r].a, "SPEC.", 5)) { snprintf(bad, sizeof bad, "%s reason \"%s\" is not a SPEC", rows[r].code, rows[r].a); break; }
			code_file(rows[r].a, sf, sizeof sf);
			if (!exists(sf)) { snprintf(bad, sizeof bad, "%s reason %s does not exist", rows[r].code, rows[r].a); break; }
			field(sf, "component", comp, sizeof comp);
			if (!in_set(&comps, comp)) snprintf(bad, sizeof bad, "%s reason %s belongs to %s, not in the petition", rows[r].code, rows[r].a, comp);
		}
		report(2, id, bad);
		/* rule 3: every SPEC clause of the primary component is bound by a TST */
		bad[0] = 0;
		for (int s = 0; s < nspec && !bad[0]; s++) {
			char sf[PATHL], comp[MAXL], sc[MAXL];
			snprintf(sf, sizeof sf, "specs/%s", specs[s]);
			field(sf, "component", comp, sizeof comp);
			if (strcmp(comp, primary)) continue;
			field(sf, "code", sc, sizeof sc);
			if (!in_set(&bound, sc)) snprintf(bad, sizeof bad, "%s has no test", sc);
		}
		report(3, id, bad);
		/* rule 4: every CAP a component requires is asked for */
		bad[0] = 0;
		for (int k = 0; k < all.n && !bad[0]; k++) {
			char cf[PATHL], req[64][MAXL];
			code_file(all.items[k], cf, sizeof cf);
			int nq = list(cf, "requires", req, 64);
			for (int j = 0; j < nq; j++) if (!in_set(&caps, req[j])) { snprintf(bad, sizeof bad, "%s requires %s, not asked for", all.items[k], req[j]); break; }
		}
		report(4, id, bad);
		/* rule 5: every CAP asked for is exposed by a component in the petition */
		bad[0] = 0;
		for (int k = 0; k < all.n; k++) {
			char cf[PATHL], ex[64][MAXL];
			code_file(all.items[k], cf, sizeof cf);
			int ne = list(cf, "exposes", ex, 64);
			for (int j = 0; j < ne; j++) add_set(&exposed, ex[j]);
		}
		for (int k = 0; k < caps.n && !bad[0]; k++) if (!in_set(&exposed, caps.items[k])) snprintf(bad, sizeof bad, "%s is granted but nothing in the petition exposes it", caps.items[k]);
		report(5, id, bad);
		/* rule 6: every component's run-time depends are in the petition, transitively */
		bad[0] = 0;
		for (int k = 0; k < all.n && !bad[0]; k++) if (!in_set(&comps, all.items[k])) snprintf(bad, sizeof bad, "%s is depended on but not listed", all.items[k]);
		report(6, id, bad);
		/* rule 7: a component marked admitted has an admission in its catalogue entry */
		bad[0] = 0;
		for (int r = 0; r < nr && !bad[0]; r++) {
			if (strcmp(rows[r].kind, "component") || strcmp(rows[r].a, "true")) continue;
			char cf[PATHL], adm[MAXL];
			code_file(rows[r].code, cf, sizeof cf);
			field(cf, "admitted", adm, sizeof adm);
			if (!adm[0]) snprintf(bad, sizeof bad, "%s is marked admitted and has no admission", rows[r].code);
		}
		report(7, id, bad);
		/* rule 8: every sensor names a SPEC clause and a consumer key */
		bad[0] = 0;
		for (int r = 0; r < nr && !bad[0]; r++) {
			if (strcmp(rows[r].kind, "sensor")) continue;
			char sf[PATHL], kf[PATHL];
			if (strncmp(rows[r].a, "SPEC.", 5)) { snprintf(bad, sizeof bad, "%s names no SPEC clause", rows[r].code); break; }
			code_file(rows[r].a, sf, sizeof sf);
			if (!exists(sf)) { snprintf(bad, sizeof bad, "%s binds %s, which does not exist", rows[r].code, rows[r].a); break; }
			if (!rows[r].b[0] || !code_file(rows[r].b, kf, sizeof kf) || !exists(kf)) snprintf(bad, sizeof bad, "%s has no consumer", rows[r].code);
		}
		report(8, id, bad);
		/* rule 21: every catalogue entry the petition names carries one plain sentence saying what it is */
		bad[0] = 0;
		for (int r = 0; r < nr && !bad[0]; r++) {
			if (strcmp(rows[r].kind, "component") && strcmp(rows[r].kind, "capability")) continue;
			char cf[PATHL], sent[MAXS];
			code_file(rows[r].code, cf, sizeof cf);
			if (!exists(cf)) continue;	/* rule 1's problem, named by check */
			if (!field(cf, strcmp(rows[r].kind, "capability") ? "sentence" : "touches", sent, sizeof sent) || !sent[0]) snprintf(bad, sizeof bad, "%s has no sentence saying what it is", rows[r].code);
		}
		report(21, id, bad);
		free(t);
	}
	fprintf(logf, "# %d failure(s)\n", fails);
	fclose(logf);
	char *out = slurp(path); fputs(out, stdout); free(out);
	return fails ? 1 : 0;
}

/* ---- lines: rule 13, a numbered list of verb-noun lines ---- */

static int cmd_lines(const char *path)
{
	char *t = slurp(path);
	if (!t) die("rule 13: no file %s", path);
	char *vl = slurp("constitution/verbs.toml");
	if (!vl) die("no constitution/verbs.toml");
	static struct set verbs; verbs.n = 0;
	for (const char *p = vl; p && *p; p = strchr(p, '\n') ? strchr(p, '\n') + 1 : NULL) {
		if (*p < 'a' || *p > 'z') continue;
		size_t l = strspn(p, "abcdefghijklmnopqrstuvwxyz");
		const char *q = p + l; while (*q == ' ') q++;
		if (*q == '=') { char v[MAXL]; snprintf(v, sizeof v, "%.*s", (int)l, p); add_set(&verbs, v); }
	}
	free(vl);
	static struct set asked; asked.n = 0;
	int n = 0;
	for (char *p = t; p && *p; ) {
		char *e = strchr(p, '\n'); if (e) *e = 0;
		char *line = p; p = e ? e + 1 : NULL;
		size_t ll = strlen(line); while (ll && (line[ll - 1] == '\r' || line[ll - 1] == ' ')) line[--ll] = 0;
		if (!ll) continue;
		n++;
		char num[16]; snprintf(num, sizeof num, "%d. ", n);
		if (strncmp(line, num, strlen(num))) die("rule 13: line %d is not numbered %d: \"%s\"", n, n, line);
		const char *rest = line + strlen(num);
		const char *sp = strchr(rest, ' ');
		if (!sp || !sp[1]) die("rule 13: line %d has no noun: \"%s\"", n, line);
		char verb[MAXL], noun[MAXL];
		snprintf(verb, sizeof verb, "%.*s", (int)(sp - rest), rest);
		snprintf(noun, sizeof noun, "%s", sp + 1);
		if (strchr(noun, ' ')) die("rule 13: line %d is more than a verb and a noun: \"%s\"", n, line);
		if (!in_set(&verbs, verb)) die("rule 13: line %d uses a verb not in constitution/verbs.toml: \"%s\"", n, verb);
		if (!strncmp(noun, "VERB.", 5)) {
			if (strcmp(verb, "record") && strcmp(verb, "ask")) die("rule 13: line %d: only record and ask may name a verb that does not exist: \"%s\"", n, line);
			add_set(&asked, noun + 5);
		} else {
			char cf[PATHL];
			if (!code_file(noun, cf, sizeof cf) || !exists(cf)) die("rule 13: line %d names a noun that is not in the catalogue: \"%s\"", n, noun);
		}
	}
	if (!n) die("rule 13: %s is empty", path);
	/* a verb asked for cannot also be used: a used verb must be in the constitution (checked above), and one that is cannot be asked for */
	for (int i = 0; i < asked.n; i++) if (in_set(&verbs, asked.items[i])) die("rule 13: the verb \"%s\" is asked for and already exists", asked.items[i]);
	printf("rule 13: %d line(s), every verb in the constitution, every noun in the catalogue", n);
	if (asked.n) { printf("; asked for:"); for (int i = 0; i < asked.n; i++) printf(" %s", asked.items[i]); }
	printf("\n");
	free(t);
	return 0;
}

/* ---- petition: a draft petition is well-formed (the shape, rules 1, 2 and 9) ---- */

static int cmd_petition(const char *path)
{
	static const char *need[] = { "id", "purpose", "submitter", "status", NULL };
	static const char *statuses[] = { "draft", "pending", "review", "approved", "rejected", "expired", "retired", NULL };
	static const char *roles[] = { "primary", "dependency", "boot", "toolchain", "machine", NULL };
	char *t = slurp(path), v[MAXL], id[MAXL], now[16], exp[16] = "";
	if (!t) die("well-formed: %s cannot be read", path);
	if (strncmp(t, "[petition]", 10) && !strstr(t, "\n[petition]")) die("well-formed: %s has no [petition] table", path);
	/* a petition is fields, never prose: every line is a comment, a table header or key = value */
	int ln = 1;
	for (const char *p = t; *p; ln++) {
		const char *e = strchr(p, '\n');
		size_t n = e ? (size_t)(e - p) : strlen(p);
		const char *q = p, *end = p + n;
		while (q < end && (*q == ' ' || *q == '\t' || *q == '\r')) q++;
		if (q < end && *q != '#' && *q != '[') {
			const char *k = q;
			while (k < end && (isalnum((unsigned char)*k) || *k == '_' || *k == '-')) k++;
			const char *name_end = k;
			while (k < end && (*k == ' ' || *k == '\t')) k++;
			if (name_end == q || k >= end || *k != '=') die("well-formed: line %d is not TOML, a petition is fields and never prose: '%.40s'", ln, q);
		}
		p = e ? e + 1 : end;
	}
	for (int i = 0; need[i]; i++)
		if (!field_in(t, need[i], v, sizeof v) || !v[0]) die("well-formed: [petition] has no %s", need[i]);
	field_in(t, "id", id, sizeof id);
	if (strlen(id) != 4 || strspn(id, "0123456789") != 4) die("well-formed: the id '%s' is not four digits", id);
	field_in(t, "purpose", v, sizeof v);
	char cf[PATHL];
	if (strncmp(v, "PUR.", 4) || !code_file(v, cf, sizeof cf) || !exists(cf)) die("rule 1: missing %s", v);
	field_in(t, "status", v, sizeof v);
	int ok = 0;
	for (int i = 0; statuses[i]; i++) if (!strcmp(v, statuses[i])) ok = 1;
	if (!ok) die("well-formed: the status '%s' is not one the schema has", v);
	/* expires = 2027-04-13, a bare date */
	const char *ex = strstr(t, "\nexpires");
	if (ex) { const char *q = strchr(ex, '='); if (q) { q++; while (*q == ' ') q++; snprintf(exp, sizeof exp, "%.10s", q); } }
	if (strlen(exp) != 10 || exp[4] != '-' || exp[7] != '-') die("well-formed: [petition] has no expires date");
	today(now);
	if (strcmp(exp, now) <= 0) die("rule 9: petition %s expired %s", id, exp);
	static struct row rows[256];
	int nr = rows_of(t, rows, 256);
	static struct set pend;
	pend.n = 0;
	for (int r = 0; r < nr; r++) {
		if (rows[r].code[0] && !code_file(rows[r].code, cf, sizeof cf)) die("rule 1: %s is not a catalogue number", rows[r].code);
		if (!strcmp(rows[r].kind, "component")) {
			ok = 0;
			for (int i = 0; roles[i]; i++) if (!strcmp(rows[r].b, roles[i])) ok = 1;
			if (!rows[r].code[0]) die("well-formed: a [[component]] has no code");
			if (!ok) die("well-formed: %s has the role '%s', which the schema does not have", rows[r].code, rows[r].b);
			if (strcmp(rows[r].a, "true") && strcmp(rows[r].a, "false")) die("well-formed: %s has no admitted = true or false", rows[r].code);
			if (!strcmp(rows[r].a, "false")) add_set(&pend, rows[r].code);
		} else if (!strcmp(rows[r].kind, "capability")) {
			if (strncmp(rows[r].a, "SPEC.", 5)) die("rule 2: capability %s has the reason '%s', which is not a SPEC code", rows[r].code, rows[r].a);
		}
	}
	static char codes[MAXS][MAXL];
	int nc = codes_in(t, codes, MAXS);
	for (int c = 0; c < nc; c++) {
		const char *at = strchr(codes[c], '@');
		if (!code_file(codes[c], cf, sizeof cf)) die("rule 1: %s is not a catalogue number", codes[c]);
		if (!at || at[1] < '1' || at[1] > '9') die("rule 1: %s has no version", codes[c]);
		if (exists(cf) || in_set(&pend, codes[c])) continue;
		/* a clause, test or sensor of a component not yet admitted comes with it */
		if (pend.n > 0 && (!strncmp(codes[c], "SPEC.", 5) || !strncmp(codes[c], "TST.", 4) || !strncmp(codes[c], "SNS.", 4))) continue;
		die("rule 1: missing %s", codes[c]);
	}
	printf("petition %s: well-formed, %d component(s) pending admission\n", id, pend.n);
	for (int i = 0; i < pend.n; i++) printf("pending %s\n", pend.items[i]);
	free(t);
	return 0;
}

int main(int argc, char **argv)
{
	/* run from the repository root, wherever the binary sits */
	char self[MAXS];
	ssize_t l = readlink("/proc/self/exe", self, sizeof self - 1);
	if (l > 0) { self[l] = 0; char *s = strstr(self, "/out/"); if (!s) s = strstr(self, "/pool/"); if (s) { *s = 0; if (chdir(self)) die("cannot reach the repository"); } }
	if (argc >= 3 && !strcmp(argv[1], "check")) return cmd_check(argv[2]);
	if (argc >= 4 && !strcmp(argv[1], "hardware")) return cmd_hardware(argv[2], argv[3]);
	if (argc >= 2 && !strcmp(argv[1], "rules")) return cmd_rules();
	if (argc >= 3 && !strcmp(argv[1], "lines")) return cmd_lines(argv[2]);
	if (argc >= 3 && !strcmp(argv[1], "petition")) return cmd_petition(argv[2]);
	if (argc >= 3 && !strcmp(argv[1], "artifact")) return cmd_artifact(argv[2]);
	if (argc >= 4 && !strcmp(argv[1], "probe")) return cmd_probe(argv[2], argv[3]);
	if (argc >= 2 && !strcmp(argv[1], "probes")) return cmd_probes();
	fputs("referee check <flavor> | hardware <code> <src> | rules | lines <file> | petition <file> | artifact <flavor> | probe <flavor> <src> | probes\n", stderr);
	return 2;
}
