/* CMP.archive.cpio@1: find the files in an uncompressed cpio archive.  See earlycpio.c for provenance. */
#ifndef AIOS_EARLYCPIO_H
#define AIOS_EARLYCPIO_H

#define MAX_CPIO_FILE_NAME 18

struct cpio_data {
	void *data;
	unsigned long size;
	char name[MAX_CPIO_FILE_NAME];
};

struct cpio_data find_cpio_data(const char *path, void *data, unsigned long len, long *nextoff);

#endif
