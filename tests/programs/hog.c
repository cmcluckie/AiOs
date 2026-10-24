/* Probe for TST.share.on-time@2: the rudest program there is.  It never calls
 * the door and never ends.  BYTE's sieve was rude for 5 seconds under
 * emulation and 8 ms on the jig (findings 0012); this one is rude forever on
 * any machine, so whether anything else runs is up to the kernel alone.  Not a
 * component; never admitted.
 * Boots on: share, gateshare.
 */
#include "door.h"

void __attribute__((section(".text.start"), noreturn)) _start(const struct door *door)
{
	volatile unsigned long n = 0;

	(void)door;
	for (;;)
		n++;
}
