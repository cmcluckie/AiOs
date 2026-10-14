/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.disk.ata-pio@1: IDE, whose registers go back to the IBM PC/AT's hard
 * disk controller (1984); the kind of disk Minix and Linux 0.01 ran on.  The
 * first disk on the primary channel, one 512-byte sector per command,
 * programmed I/O, polled: no DMA, no interrupts.
 * Exposes ata_read, ata_write and ata_flush.  Requires CAP.dev.disk.
 *
 * Lifted from Linux, commit a5ebb76233b79db01e82e061173cb73c6d2b5c6b (2026-10-10).
 * The machine-readable record is origin.toml beside this file; jig/upstream reads it.
 *   include/linux/ata.h               ATA_SECT_SIZE 28, the device and status bits
 *                                     157-169, the register offsets 187-197,
 *                                     ATA_CMD_FLUSH 212, ATA_CMD_PIO_READ/WRITE 234, 236,
 *                                     lba_28_ok 1050-1054
 *   drivers/ata/libata-sff.c          ata_sff_check_status 63-66, ata_sff_altstatus
 *                                     83-101, ata_sff_sync 139-142, ata_sff_pause 156-160,
 *                                     ata_sff_set_devctl 226-238, ata_sff_dev_select
 *                                     254-265, ata_sff_tf_load 342-375,
 *                                     ata_sff_exec_command 430-434, ata_sff_data_xfer 475-487
 *   include/linux/libata.h            ata_sff_busy_wait 2149-2161, ata_wait_idle 2173-2181
 *   drivers/ata/libata-core.c         the LBA28 taskfile: 795, 816-822
 *   drivers/ata/pata_legacy.c         the legacy ports: 0x1f0 at 92, its control at
 *                                     io + 0x0206 at 285
 *   arch/x86/include/asm/io.h         outsw, insw: the rep forms, 272-302
 *   arch/x86/include/asm/shared/io.h  BUILDIO 7-20: the inb and outb instructions
 *
 * Cut, and why:
 *   udelay(10) and the max count in   time needs a clock and this machine has none;
 *   ata_sff_busy_wait, so the 10 ms   a loop count instead would be the turbo bug
 *   limit in ata_wait_idle            (part 9).  We wait as long as it takes; a disk
 *                                     that never answers hangs the machine.  A status
 *                                     of 0xff, nobody on the bus, still ends the wait.
 *   ndelay(400) in ata_sff_pause      the same reason; the altstatus read stays.  A real
 *                                     drive wants the 400 ns.  Findings 0010.
 *   struct ata_port, ata_ioports,     one channel, one disk, fixed ports: the register
 *   ata_taskfile, ata_queued_cmd      writes stay, the plumbing around them goes
 *   the LBA48 and CHS paths           a disk under 128 GiB needs neither
 *   the PIO state machine             (ata_sff_hsm_move): one sector per command,
 *   (ata_sff_hsm_move), DMA, IRQs     polled, so it is wait, check, move 512 bytes
 *   the slave device, ATAPI           one disk, the master
 *   the trailing odd byte in          sectors are always 512 bytes
 *   ata_sff_data_xfer
 */

#include "ata.h"

/* arch/x86/include/asm/shared/io.h */
static inline unsigned char inb(unsigned short port)
{
	unsigned char v;
	__asm__ volatile("inb %w1, %b0" : "=a"(v) : "Nd"(port));
	return v;
}

static inline void outb(unsigned char v, unsigned short port)
{
	__asm__ volatile("outb %b0, %w1" : : "a"(v), "Nd"(port));
}

/* arch/x86/include/asm/io.h, BUILDIO(w, u16): the rep forms */
static inline void outsw(unsigned short port, const void *addr, unsigned long count)
{
	__asm__ volatile("rep outsw" : "+S"(addr), "+c"(count) : "d"(port) : "memory");
}

static inline void insw(unsigned short port, void *addr, unsigned long count)
{
	__asm__ volatile("rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

/* include/linux/ata.h */
#define ATA_SECT_SIZE		512
#define ATA_NIEN		(1 << 1)	/* disable-irq flag */
#define ATA_LBA			(1 << 6)	/* LBA28 selector */
#define ATA_DEVICE_OBS		((1 << 7) | (1 << 5)) /* obs bits in dev reg */
#define ATA_BUSY		(1 << 7)	/* BSY status bit */
#define ATA_DRDY		(1 << 6)	/* device ready */
#define ATA_DF			(1 << 5)	/* device fault */
#define ATA_DRQ			(1 << 3)	/* data request i/o */
#define ATA_ERR			(1 << 0)	/* have an error */
#define ATA_REG_DATA		0x00
#define ATA_REG_ERR		0x01
#define ATA_REG_NSECT		0x02
#define ATA_REG_LBAL		0x03
#define ATA_REG_LBAM		0x04
#define ATA_REG_LBAH		0x05
#define ATA_REG_DEVICE		0x06
#define ATA_REG_STATUS		0x07
#define ATA_REG_FEATURE		ATA_REG_ERR
#define ATA_REG_CMD		ATA_REG_STATUS
#define ATA_CMD_FLUSH		0xE7
#define ATA_CMD_PIO_READ	0x20
#define ATA_CMD_PIO_WRITE	0x30

/* drivers/ata/pata_legacy.c: the first legacy channel, its control block at io + 0x0206 */
#define IO	0x1f0
#define CTL	(IO + 0x0206)

/* libata-sff.c ata_sff_check_status */
static unsigned char check_status(void)
{
	return inb(IO + ATA_REG_STATUS);
}

/* libata-sff.c ata_sff_altstatus, ata_sff_sync, ata_sff_pause: read the
 * alternate status, which flushes the writes before it; ndelay(400) cut. */
static void pause(void)
{
	(void)inb(CTL);
}

/* libata.h ata_sff_busy_wait: until the bits clear or nobody is there (0xff).
 * udelay(10) and the count cut: see the top of this file. */
static unsigned char busy_wait(unsigned int bits)
{
	unsigned char status;

	do {
		status = check_status();
	} while (status != 0xff && (status & bits));
	return status;
}

/* libata.h ata_wait_idle */
static unsigned char wait_idle(void)
{
	return busy_wait(ATA_BUSY | ATA_DRQ);
}

/* libata-sff.c ata_sff_set_devctl, ata_sff_dev_select: interrupts off at the
 * drive, the master selected.  Then: is anybody there? */
int ata_init(void)
{
	unsigned char status;

	outb(ATA_NIEN, CTL);
	outb(ATA_DEVICE_OBS, IO + ATA_REG_DEVICE);
	pause();
	status = wait_idle();
	if (status == 0xff || !(status & ATA_DRDY))
		return -1;		/* no controller (0xff), or no drive on it */
	return 0;
}

/* libata-sff.c ata_sff_tf_load and ata_sff_exec_command, with the LBA28
 * taskfile libata-core.c ata_build_rw_tf builds; one sector. */
static int command(unsigned long sector, unsigned char cmd)
{
	/* include/linux/ata.h lba_28_ok: the ending block must be under 0x0fffffff */
	if (sector + 1 >= (1UL << 28) - 1)
		return -1;
	wait_idle();
	outb(0, IO + ATA_REG_FEATURE);
	outb(1, IO + ATA_REG_NSECT);
	outb(sector & 0xff, IO + ATA_REG_LBAL);
	outb((sector >> 8) & 0xff, IO + ATA_REG_LBAM);
	outb((sector >> 16) & 0xff, IO + ATA_REG_LBAH);
	outb(ATA_DEVICE_OBS | ATA_LBA | ((sector >> 24) & 0xf), IO + ATA_REG_DEVICE);
	wait_idle();
	outb(cmd, IO + ATA_REG_CMD);
	pause();
	return 0;
}

/* After a command: wait while busy, then the drive either wants data (DRQ) or
 * has failed (ERR, DF).  The polled shape of ata_sff_hsm_move. */
static int data_ready(void)
{
	unsigned char status = busy_wait(ATA_BUSY);

	if (status == 0xff || (status & (ATA_ERR | ATA_DF)) || !(status & ATA_DRQ))
		return -1;
	return 0;
}

/* libata-sff.c ata_sff_data_xfer: whole 16-bit words, rep insw / rep outsw */
int ata_read(unsigned long sector, void *buf)
{
	if (command(sector, ATA_CMD_PIO_READ) || data_ready())
		return -1;
	insw(IO + ATA_REG_DATA, buf, ATA_SECT_SIZE >> 1);
	return 0;
}

int ata_write(unsigned long sector, const void *buf)
{
	unsigned char status;

	if (command(sector, ATA_CMD_PIO_WRITE) || data_ready())
		return -1;
	outsw(IO + ATA_REG_DATA, buf, ATA_SECT_SIZE >> 1);
	status = busy_wait(ATA_BUSY);
	return (status == 0xff || (status & (ATA_ERR | ATA_DF))) ? -1 : 0;
}

int ata_flush(void)
{
	unsigned char status;

	wait_idle();
	outb(ATA_CMD_FLUSH, IO + ATA_REG_CMD);
	pause();
	status = busy_wait(ATA_BUSY);
	return (status == 0xff || (status & (ATA_ERR | ATA_DF))) ? -1 : 0;
}
