#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

//
// compiler magic
//

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x)           (sizeof(x) / sizeof((x)[0]))
#endif

#if !defined(UNUSED_PARAM)
#define UNUSED_PARAM(x)         (void)(x);
#endif

#if !defined(__always_inline)
#define __always_inline __inline __attribute__ ((__always_inline__))
#endif

#define MACRO_STR_EXPAND(mm)    #mm
#define MACRO_TO_STR(m)         MACRO_STR_EXPAND(m)

#define ACCESS_ONCE(x) 		(*(volatile typeof(x) *)&(x))
#define READ_ONCE(x) 		({ typeof(x) ___x = ACCESS_ONCE(x); ___x; })
#define WRITE_ONCE(x, val) 	({ ACCESS_ONCE(x) = (val); })
#define barrier() 		__asm__ __volatile__("": : :"memory")

//
// bit magic
//

#define BITS_PER_LONG           (__SIZEOF_LONG__ * __CHAR_BIT__)
#define BITS_PER_LONG_LONG      (__SIZEOF_LONG_LONG__ * __CHAR_BIT__)

#define BIT(nr)                 (1UL << (nr))
#define BIT_ULL(nr)             (1ULL << (nr))
#define BIT_MASK(nr)            (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#define BIT_ULL_MASK(nr)        (1ULL << ((nr) % BITS_PER_LONG_LONG))
#define BIT_ULL_WORD(nr)        ((nr) / BITS_PER_LONG_LONG)
#define BITS_PER_BYTE           8

/*
 * Create a contiguous bitmask starting at bit position @l and ending at
 * position @h. For example
 * GENMASK_ULL(39, 21) gives us the 64bit vector 0x000000ffffe00000.
 */
#define GENMASK(h, l) \
        (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define GENMASK_ULL(h, l) \
        (((~0ULL) - (1ULL << (l)) + 1) & \
         (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

#define BITS_MASK               GENMASK
#define BITS_MASK_ULL           GENMASK_ULL

static inline int test_bit_u8(volatile uint8_t *bm, uint32_t bit)
{
	return (*bm & BIT(bit));
}

static inline void set_bit_u8(volatile uint8_t *bm, uint32_t bit)
{
	*bm |= BIT(bit);
}

static inline void clear_bit_u8(volatile uint8_t *bm, uint32_t bit)
{
	*bm &= ~BIT(bit);
}

#endif /* __UTILS_H__ */