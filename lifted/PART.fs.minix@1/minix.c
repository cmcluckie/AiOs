/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.fs.minix@1: the original Minix filesystem (v1, 14-character names, magic
 * 0x137F), the one Linux started on in 1991.  Finds a file in the root
 * directory and reads it; makes a new file there and writes it.  Files up to
 * seven 1 KiB blocks, the inode's direct zones.  No subdirectories, no
 * overwrite, no delete: nothing has asked.  Requires PART.disk.ata-pio.
 *
 * Lifted from Linux, commit a5ebb76233b79db01e82e061173cb73c6d2b5c6b (2026-10-10).
 * bitmap.c still opens "Copyright (C) 1991, 1992  Linus Torvalds".
 * The machine-readable record is origin.toml beside this file; jig/upstream reads it.
 *   include/uapi/linux/minix_fs.h  MINIX_ROOT_INO 18, MINIX_INODES_PER_BLOCK 29,
 *                                  struct minix_inode 35-43, struct minix_super_block
 *                                  66-77, struct minix_dir_entry 98-101
 *   include/uapi/linux/magic.h     MINIX_SUPER_MAGIC 43
 *   include/uapi/linux/fs.h        BLOCK_SIZE 40-41
 *   include/uapi/linux/stat.h      S_IFREG 12
 *   fs/minix/inode.c               minix_check_superblock 182-190, the superblock
 *                                  read and the V1 branch of minix_fill_super 247-265,
 *                                  where the bitmaps live 317-330, V1_minix_update_inode
 *                                  646-655
 *   fs/minix/bitmap.c              minix_new_block 71-95, minix_V1_raw_inode 106-128,
 *                                  minix_new_inode 213-251
 *   fs/minix/dir.c                 minix_last_byte 33-41, minix_next_entry 78-81,
 *                                  namecompare 137-143, minix_find_entry 154-197,
 *                                  minix_add_link 199-273
 *   fs/minix/itree_v1.c            block_to_path, the direct branch 37-38
 *   fs/minix/namei.c               minix_mknod 44-50: the order a new file is made in
 *
 * Cut, and why:
 *   the VFS: struct inode, super_block,  Linux's shared file machinery.  One
 *   dentry, folios, buffer heads,        filesystem, one directory, one block at a
 *   the page cache, mark_*_dirty         time: read it, change it, write it back
 *   locks (bitmap_lock, pointers_lock)   one program, nothing runs at the same time
 *   minix V2 and V3, 30-character names  the original only
 *   indirect zones (get_branch,          files up to 7 KiB; the plan says so, and the
 *   alloc_branch), truncate, free_*      program's file is under 1 KiB
 *   s_state, MINIX_VALID_FS              there is no mount and no unmount: every
 *                                        change is written through before save returns
 *   times                                no clock of day on this machine (plan 9.12):
 *                                        new files are dated 1 January 1970
 *   printk                               the caller reports; failures are -1
 * Glue, ours: bread and bwrite (two sectors a block), the first-zero-bit and
 * set-bit for a little-endian bitmap (minix.h maps them to the *_le helpers),
 * and copying fields instead of memcpy.
 */

#include "minix.h"
#include "../PART.disk.ata-pio@1/ata.h"

/* include/uapi/linux/fs.h */
#define BLOCK_SIZE_BITS 10
#define BLOCK_SIZE (1<<BLOCK_SIZE_BITS)

/* include/uapi/linux/magic.h */
#define MINIX_SUPER_MAGIC	0x137F		/* minix v1 fs, 14 char names */

/* include/uapi/linux/stat.h */
#define S_IFREG  0100000

/* include/uapi/linux/minix_fs.h */
typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;

#define MINIX_ROOT_INO 1
#define MINIX_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct minix_inode)))

struct minix_inode {
	__u16 i_mode;
	__u16 i_uid;
	__u32 i_size;
	__u32 i_time;
	__u8  i_gid;
	__u8  i_nlinks;
	__u16 i_zone[9];
};

struct minix_super_block {
	__u16 s_ninodes;
	__u16 s_nzones;
	__u16 s_imap_blocks;
	__u16 s_zmap_blocks;
	__u16 s_firstdatazone;
	__u16 s_log_zone_size;
	__u32 s_max_size;
	__u16 s_magic;
	__u16 s_state;
	__u32 s_zones;
};

struct minix_dir_entry {
	__u16 inode;
	char name[];
};

/* fs/minix/minix.h struct minix_sb_info, the fields V1 uses */
static struct {
	unsigned long s_ninodes;
	unsigned long s_nzones;
	unsigned long s_imap_blocks;
	unsigned long s_zmap_blocks;
	unsigned long s_firstdatazone;
	unsigned long s_log_zone_size;
	int s_dirsize;
	int s_namelen;
	int mounted;
} sbi;

enum { DIRECT = 7 };		/* fs/minix/itree_v1.c: zones 0-6 are direct */

/* Glue: a filesystem block is two disk sectors. */
static int bread(unsigned long block, void *buf)
{
	unsigned char *b = buf;

	return ata_read(block * 2, b) || ata_read(block * 2 + 1, b + 512) ? -1 : 0;
}

static int bwrite(unsigned long block, const void *buf)
{
	const unsigned char *b = buf;

	return ata_write(block * 2, b) || ata_write(block * 2 + 1, b + 512) ? -1 : 0;
}

static void copy(void *to, const void *from, unsigned long n)
{
	unsigned char *t = to;
	const unsigned char *f = from;

	while (n--)
		*t++ = *f++;
}

static void zero(void *to, unsigned long n)
{
	unsigned char *t = to;

	while (n--)
		*t++ = 0;
}

static unsigned long length(const char *s)
{
	unsigned long n = 0;

	while (s[n])
		n++;
	return n;
}

/* Glue: little-endian bitmaps, bit 0 is the low bit of byte 0 (minix.h's
 * find_first_zero_bit_le, __set_bit_le). */
static unsigned long find_first_zero_bit(const unsigned char *map, unsigned long size)
{
	unsigned long i;

	for (i = 0; i < size; i++)
		if (!(map[i >> 3] & (1 << (i & 7))))
			return i;
	return size;
}

static void set_bit(unsigned long nr, unsigned char *map)
{
	map[nr >> 3] |= 1 << (nr & 7);
}

static unsigned char sb_block[BLOCK_SIZE], map[BLOCK_SIZE], iblock[BLOCK_SIZE], dblock[BLOCK_SIZE];

/* fs/minix/inode.c minix_fill_super, the V1 branch, and minix_check_superblock */
int minix_mount(void)
{
	const struct minix_super_block *ms = (const void *)sb_block;

	if (sbi.mounted)
		return 0;
	if (ata_init() || bread(1, sb_block))
		return -1;
	sbi.s_ninodes = ms->s_ninodes;
	sbi.s_nzones = ms->s_nzones;
	sbi.s_imap_blocks = ms->s_imap_blocks;
	sbi.s_zmap_blocks = ms->s_zmap_blocks;
	sbi.s_firstdatazone = ms->s_firstdatazone;
	sbi.s_log_zone_size = ms->s_log_zone_size;
	if (ms->s_magic != MINIX_SUPER_MAGIC)
		return -1;		/* the original only: v1, 14-character names */
	sbi.s_dirsize = 16;
	sbi.s_namelen = 14;
	if (sbi.s_log_zone_size != 0)
		return -1;
	if (sbi.s_ninodes < 1 || sbi.s_firstdatazone <= 4 ||
	    sbi.s_firstdatazone >= sbi.s_nzones)
		return -1;
	sbi.mounted = 1;
	return 0;
}

/* fs/minix/inode.c: the bitmaps start at block 2, inode map first, then zone map */
static unsigned long imap_block(unsigned long i) { return 2 + i; }
static unsigned long zmap_block(unsigned long i) { return 2 + sbi.s_imap_blocks + i; }

/* fs/minix/bitmap.c minix_V1_raw_inode: which block an inode lives in, and where */
static struct minix_inode *raw_inode(unsigned long ino, unsigned long *block)
{
	if (!ino || ino > sbi.s_ninodes)
		return 0;
	ino--;
	*block = 2 + sbi.s_imap_blocks + sbi.s_zmap_blocks +
		 ino / MINIX_INODES_PER_BLOCK;
	if (bread(*block, iblock))
		return 0;
	return (struct minix_inode *)iblock + ino % MINIX_INODES_PER_BLOCK;
}

static int read_inode(unsigned long ino, struct minix_inode *in)
{
	unsigned long block;
	struct minix_inode *p = raw_inode(ino, &block);

	if (!p)
		return -1;
	copy(in, p, sizeof *in);
	return 0;
}

/* fs/minix/inode.c V1_minix_update_inode: the inode back where it lives */
static int write_inode(unsigned long ino, const struct minix_inode *in)
{
	unsigned long block;
	struct minix_inode *p = raw_inode(ino, &block);

	if (!p)
		return -1;
	copy(p, in, sizeof *in);
	return bwrite(block, iblock);
}

/* fs/minix/bitmap.c minix_new_block: the first free zone, marked busy */
static unsigned long new_block(void)
{
	unsigned long bits_per_zone = 8 * BLOCK_SIZE;
	unsigned long i;

	for (i = 0; i < sbi.s_zmap_blocks; i++) {
		unsigned long j;

		if (bread(zmap_block(i), map))
			return 0;
		j = find_first_zero_bit(map, bits_per_zone);
		if (j < bits_per_zone) {
			set_bit(j, map);
			if (bwrite(zmap_block(i), map))
				return 0;
			j += i * bits_per_zone + sbi.s_firstdatazone-1;
			if (j < sbi.s_firstdatazone || j >= sbi.s_nzones)
				break;
			return j;
		}
	}
	return 0;
}

/* fs/minix/bitmap.c minix_new_inode, the bitmap half: the first free inode number */
static unsigned long new_inode(void)
{
	unsigned long bits_per_zone = 8 * BLOCK_SIZE;
	unsigned long i, j = bits_per_zone;

	for (i = 0; i < sbi.s_imap_blocks; i++) {
		if (bread(imap_block(i), map))
			return 0;
		j = find_first_zero_bit(map, bits_per_zone);
		if (j < bits_per_zone)
			break;
	}
	if (j >= bits_per_zone)
		return 0;
	set_bit(j, map);
	if (bwrite(imap_block(i), map))
		return 0;
	j += i * bits_per_zone;
	if (!j || j > sbi.s_ninodes)
		return 0;
	return j;
}

/* fs/minix/itree_v1.c block_to_path, the direct branch: block n of a file is
 * zone n, for n under 7.  create allocates a zeroed zone where there is none. */
static unsigned long bmap(struct minix_inode *in, unsigned long n, int create)
{
	unsigned long z;

	if (n >= DIRECT)
		return 0;
	if (in->i_zone[n] || !create)
		return in->i_zone[n];
	z = new_block();
	if (!z)
		return 0;
	zero(dblock, BLOCK_SIZE);
	if (bwrite(z, dblock))
		return 0;
	in->i_zone[n] = z;
	return z;
}

/* fs/minix/dir.c minix_last_byte, with Linux's page as our block */
static unsigned last_byte(const struct minix_inode *dir, unsigned long n)
{
	unsigned last = BLOCK_SIZE;

	if (n == (dir->i_size >> BLOCK_SIZE_BITS))
		last = dir->i_size & (BLOCK_SIZE - 1);
	return last;
}

/* fs/minix/dir.c namecompare */
static inline int namecompare(int len, int maxlen,
	const char * name, const char * buffer)
{
	unsigned long i;

	if (len < maxlen && buffer[len])
		return 0;
	for (i = 0; i < (unsigned long)len; i++)
		if (name[i] != buffer[i])
			return 0;
	return 1;
}

/* fs/minix/dir.c minix_find_entry: the inode number, or 0 */
static unsigned long find_entry(const struct minix_inode *dir, const char *name, int namelen)
{
	unsigned long n, npages = (dir->i_size + BLOCK_SIZE - 1) >> BLOCK_SIZE_BITS;
	char *p;

	for (n = 0; n < npages; n++) {
		char *kaddr = (char *)dblock, *limit;
		unsigned long z = bmap((struct minix_inode *)dir, n, 0);

		if (!z || bread(z, dblock))
			continue;
		limit = kaddr + last_byte(dir, n) - sbi.s_dirsize;
		for (p = kaddr; p <= limit; p = p + sbi.s_dirsize) {
			struct minix_dir_entry *de = (struct minix_dir_entry *)p;

			if (!de->inode)
				continue;
			if (namecompare(namelen, sbi.s_namelen, name, de->name))
				return de->inode;
		}
	}
	return 0;
}

/* fs/minix/dir.c minix_add_link: the first empty slot, or the one at the end */
static int add_link(struct minix_inode *dir, unsigned long dirino,
		    const char *name, int namelen, unsigned long ino)
{
	unsigned long n, npages = (dir->i_size + BLOCK_SIZE - 1) >> BLOCK_SIZE_BITS;
	unsigned long z, pos;
	char *kaddr = (char *)dblock, *p;
	struct minix_dir_entry *de;

	for (n = 0; n <= npages; n++) {
		char *limit, *dir_end;

		z = bmap(dir, n, 1);
		if (!z || bread(z, dblock))
			return -1;
		dir_end = kaddr + last_byte(dir, n);
		limit = kaddr + BLOCK_SIZE - sbi.s_dirsize;
		for (p = kaddr; p <= limit; p = p + sbi.s_dirsize) {
			de = (struct minix_dir_entry *)p;
			if (p == dir_end) {
				/* We hit i_size */
				de->inode = 0;
				goto got_it;
			}
			if (!de->inode)
				goto got_it;
			if (namecompare(namelen, sbi.s_namelen, name, de->name))
				return -1;
		}
	}
	return -1;

got_it:
	pos = (n << BLOCK_SIZE_BITS) + (unsigned long)(p - kaddr);
	copy(de->name, name, namelen);
	zero(de->name + namelen, sbi.s_dirsize - namelen - 2);
	de->inode = ino;
	if (bwrite(z, dblock))
		return -1;
	if (pos + sbi.s_dirsize > dir->i_size)
		dir->i_size = pos + sbi.s_dirsize;
	return write_inode(dirino, dir);
}

long minix_load(const char *name, void *buf, unsigned long max)
{
	struct minix_inode root, in;
	unsigned long ino, n, got = 0;
	int namelen = (int)length(name);

	if (minix_mount() || read_inode(MINIX_ROOT_INO, &root))
		return -1;
	ino = find_entry(&root, name, namelen);
	if (!ino || read_inode(ino, &in))
		return -1;
	if (in.i_size > DIRECT * BLOCK_SIZE)
		return -1;		/* indirect zones were cut */
	for (n = 0; got < in.i_size && got < max; n++) {
		unsigned long z = bmap(&in, n, 0), k = in.i_size - got;

		if (k > BLOCK_SIZE)
			k = BLOCK_SIZE;
		if (k > max - got)
			k = max - got;
		if (z) {
			if (bread(z, dblock))
				return -1;
			copy((char *)buf + got, dblock, k);
		} else {
			zero((char *)buf + got, k);	/* a hole reads as zeros */
		}
		got += k;
	}
	return (long)got;
}

/* fs/minix/namei.c minix_mknod (minix_create calls it): a new inode, then its name; the data, which Linux writes later from the page cache, goes in between */
long minix_save(const char *name, const void *buf, unsigned long len)
{
	struct minix_inode root, in;
	unsigned long ino, n, done = 0;
	int namelen = (int)length(name);

	if (namelen < 1 || namelen > 14 || len > DIRECT * BLOCK_SIZE)
		return -1;
	if (minix_mount() || read_inode(MINIX_ROOT_INO, &root))
		return -1;
	if (find_entry(&root, name, namelen))
		return -1;		/* no overwrite: nothing has asked for one */
	ino = new_inode();
	if (!ino)
		return -1;
	zero(&in, sizeof in);
	in.i_mode = S_IFREG | 0644;
	in.i_nlinks = 1;
	in.i_size = len;
	for (n = 0; done < len; n++) {
		unsigned long z = bmap(&in, n, 1), k = len - done;

		if (!z)
			return -1;
		if (k > BLOCK_SIZE)
			k = BLOCK_SIZE;
		zero(dblock, BLOCK_SIZE);
		copy(dblock, (const char *)buf + done, k);
		if (bwrite(z, dblock))
			return -1;
		done += k;
	}
	if (write_inode(ino, &in) || add_link(&root, MINIX_ROOT_INO, name, namelen, ino))
		return -1;
	return ata_flush() ? -1 : 0;
}
