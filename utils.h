#define SET_BIT(bitfield, bit) bitfield |= 1UL << bit
#define CLEAR_BIT(bitfield, bit) bitfield &= ~(1UL << bit)
#define TOGGLE_BIT(bitfield, bit) bitfield ^= 1UL << bit;
#define SET_BIT_N(bitfield, n, value) bitfield = (bitfield & ~(1UL << n)) | (value << n)
#define GET_BIT_N(bitfield, n) (bitfield >> n) & 1U