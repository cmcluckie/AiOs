/* PART.fs.minix@2: the original Minix filesystem, files in the root directory.  See minix.c for provenance. */
#ifndef AIOS_MINIX_H
#define AIOS_MINIX_H

int minix_mount(void);					/* 0 if the disk holds a Minix v1 filesystem, -1 if not */
long minix_load(const char *name, void *buf, unsigned long max);	/* the file's bytes, up to max; its length, or -1 */
#ifdef GRANT_FS_WRITE
long minix_save(const char *name, const void *buf, unsigned long n);	/* the file, new or written over; 0, or -1 */
#endif

#endif
