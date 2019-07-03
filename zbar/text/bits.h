#ifndef __bits_h__
#define __bits_h__

#include <stdint.h>

/* GCC */
#ifdef __GNUC__

/* arch-independent gcc intrinsics */
#if defined(__GNUC_MINOR__) && (__GNUC__*256 + __GNUC_MINOR__ >= 0x303)
#define clz32 __builtin_clz
#endif

#if defined(__i386__) || defined(__amd64__)

/* clz32 */
#ifndef clz32
#define clz32 _clz32_impl
static __inline int _clz32_impl(uint32_t val) 
{
 uint32_t res;
 asm ("bsrl %1, %0\n\t"
      "xorl $31, %0\n\t" : "=r"(res) : "rm"(val) : "cc");
 return res;
}
#endif

#endif /* __i386__ || __amd64__ */

/* MSVC */
#elif defined(_MSC_VER)

#include <intrin.h>

/* clz32 */
#define clz32 _clz32_impl
static __forceinline int _clz32_impl(uint32_t val)
{
 unsigned long pos;
 _BitScanReverse(&pos, val);
 return pos ^ 31;
}

#endif

/* generic clz */
#ifndef clz32
#define clz32 _clz32_impl
static __inline int _clz32_impl(uint32_t val)
{
 int result = 1;
 if (!(val >> 16)) { result += 16; val <<= 16; }
 if (!(val >> 24)) { result += 8;  val <<= 8;  }
 if (!(val >> 28)) { result += 4;  val <<= 4;  }
 if (!(val >> 30)) { result += 2;  val <<= 2;  }
 return result - (val >> 31);
}
#endif

#endif /* __bits_h__ */
