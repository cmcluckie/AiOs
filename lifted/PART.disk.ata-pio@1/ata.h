/* PART.disk.ata-pio@1: the first IDE disk, 512 bytes at a time, by polling.  See ata.c for provenance. */
#ifndef AIOS_ATA_H
#define AIOS_ATA_H

int ata_init(void);					/* 0 if a disk answers on the primary channel, -1 if not */
int ata_read(unsigned long sector, void *buf);		/* one 512-byte sector into buf; 0 or -1 */
int ata_write(unsigned long sector, const void *buf);	/* one 512-byte sector from buf; 0 or -1 */
int ata_flush(void);					/* the disk's own cache to the platter; 0 or -1 */

#endif
