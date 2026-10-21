/* PART.divide64@1: dividing one 64-bit number by another on a 32-bit processor.
 * The 386 can only divide 64 bits by 32 in one instruction, so for a full 64 by
 * 64 divide the compiler calls a helper, __udivmoddi4, that normally comes with
 * the compiler's own runtime library (libgcc).  The jig's compiler has none for
 * 32-bit programs built without the standard libraries, and musl's printf needs
 * it to print large numbers (findings 0023).  This is long division in base 2,
 * one bit at a time, the way it is done on paper: slow, short, and easy to check.
 * Dividing by zero gives all ones and leaves the number as the remainder; the
 * real helper traps.  Nothing here divides, so the compiler cannot call it back. */
typedef unsigned long long u64;

u64 __udivmoddi4(u64 n, u64 d, u64 *rem)
{
	u64 q = 0, r = 0;
	int i;

	for (i = 63; i >= 0; i--) {
		r = (r << 1) | ((n >> i) & 1);
		if (r >= d) {
			r -= d;
			q |= (u64)1 << i;
		}
	}
	if (rem)
		*rem = r;
	return q;
}
