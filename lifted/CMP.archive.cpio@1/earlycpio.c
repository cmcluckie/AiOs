/* SPDX-License-Identifier: GPL-2.0-only
 *
 * CMP.archive.cpio@1: find the next regular file in an uncompressed cpio
 * archive ("newc", magic 070701).  cpio is the Unix archive format from the
 * 1970s that Linux still uses for its initramfs; here it carries more than one
 * program to the loader in the machine's single boot module.  Called with an
 * empty path it matches every file, and nextoff walks through them in order.
 *
 * Lifted from Linux, commit a5ebb76233b79db01e82e061173cb73c6d2b5c6b (2026-10-10).
 * The machine-readable record is origin.toml beside this file; jig/upstream reads it.
 *   lib/earlycpio.c                    enum cpio_fields 24-40, find_cpio_data 59-123
 *                                      and 129-141 (the pr_warn at 124-128 cut)
 *   include/linux/earlycpio.h          MAX_CPIO_FILE_NAME, struct cpio_data, the declaration 7-16
 * "Copyright 2012 Intel Corporation; author H. Peter Anvin."
 *
 * Cut, and why:
 *   the pr_warn on a long name         no console layer; the name is cut short, as
 *                                      strscpy does, and the caller does not use it
 *   strscpy, strlen, memcmp, PTR_ALIGN glue of ours below
 */

#include "earlycpio.h"

typedef unsigned long size_t;
#define NULL ((void *)0)

/* Glue: what lib/string.c and kernel.h would have provided. */
static size_t strlen(const char *s)
{
	size_t n = 0;

	while (s[n])
		n++;
	return n;
}

static int memcmp(const void *a, const void *b, size_t n)
{
	const unsigned char *x = a, *y = b;

	for (; n; n--, x++, y++)
		if (*x != *y)
			return *x - *y;
	return 0;
}

static void strscpy(char *to, const char *from, size_t n)
{
	size_t i;

	for (i = 0; i + 1 < n && from[i]; i++)
		to[i] = from[i];
	to[i] = 0;
}

#define PTR_ALIGN(p, a) ((const char *)(((unsigned long)(p) + (a) - 1) & ~((unsigned long)(a) - 1)))

enum cpio_fields {
	C_MAGIC,
	C_INO,
	C_MODE,
	C_UID,
	C_GID,
	C_NLINK,
	C_MTIME,
	C_FILESIZE,
	C_MAJ,
	C_MIN,
	C_RMAJ,
	C_RMIN,
	C_NAMESIZE,
	C_CHKSUM,
	C_NFIELDS
};

struct cpio_data find_cpio_data(const char *path, void *data,
				size_t len,  long *nextoff)
{
	const size_t cpio_header_len = 8*C_NFIELDS - 2;
	struct cpio_data cd = { NULL, 0, "" };
	const char *p, *dptr, *nptr;
	unsigned int ch[C_NFIELDS], *chp, v;
	unsigned char c, x;
	size_t mypathsize = strlen(path);
	int i, j;

	p = data;

	while (len > cpio_header_len) {
		if (!*p) {
			/* All cpio headers need to be 4-byte aligned */
			p += 4;
			len -= 4;
			continue;
		}

		j = 6;		/* The magic field is only 6 characters */
		chp = ch;
		for (i = C_NFIELDS; i; i--) {
			v = 0;
			while (j--) {
				v <<= 4;
				c = *p++;

				x = c - '0';
				if (x < 10) {
					v += x;
					continue;
				}

				x = (c | 0x20) - 'a';
				if (x < 6) {
					v += x + 10;
					continue;
				}

				goto quit; /* Invalid hexadecimal */
			}
			*chp++ = v;
			j = 8;	/* All other fields are 8 characters */
		}

		if ((ch[C_MAGIC] - 0x070701) > 1)
			goto quit; /* Invalid magic */

		len -= cpio_header_len;

		dptr = PTR_ALIGN(p + ch[C_NAMESIZE], 4);
		nptr = PTR_ALIGN(dptr + ch[C_FILESIZE], 4);

		if (nptr > p + len || dptr < p || nptr < dptr)
			goto quit; /* Buffer overrun */

		if ((ch[C_MODE] & 0170000) == 0100000 &&
		    ch[C_NAMESIZE] >= mypathsize &&
		    !memcmp(p, path, mypathsize)) {

			if (nextoff)
				*nextoff = (long)nptr - (long)data;

			strscpy(cd.name, p + mypathsize, MAX_CPIO_FILE_NAME);

			cd.data = (void *)dptr;
			cd.size = ch[C_FILESIZE];
			return cd; /* Found it! */
		}
		len -= (nptr - p);
		p = nptr;
	}

quit:
	return cd;
}
