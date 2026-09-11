/*
 * src/n64/libc_impl.c
 *
 * N64 port — bare-metal C library implementations
 *
 * Provides memcpy, memset, memcmp, strlen, etc. for the N64 build
 * which links with -nostdlib (no system libc).
 *
 * We keep these simple and correct; the N64 CPU is fast enough that
 * unoptimised versions are fine for our 240×160 game.
 */

#include <stddef.h>
#include <stdint.h>

/* Same as N64_ReadRomWord() in n64/defines.h -- this file is built without
 * the game headers, so it carries its own copy. The read must be volatile:
 * given a plain load, GCC turns the byte extractions below back into byte
 * loads, which is precisely the access the PI bus cannot serve. */
static inline uint32_t ReadRomWord(const void *p)
{
    return *(const volatile uint32_t *)p;
}

/* -----------------------------------------------------------------------
 * Memory functions
 * --------------------------------------------------------------------- */
void *memcpy(void *dst, const void *src, size_t n)
{
    /* Byte-granularity reads from ROM (.rodata, a KSEG1 pointer straight
     * onto the cartridge's PI bus) are unreliable -- the PI bus doesn't
     * support sub-word CPU accesses and silently returns the wrong byte
     * a large fraction of the time (see bios.c's lz77_decomp for the
     * full story; that was the root cause of the game's tile/tilemap
     * corruption). memcpy() is used both directly and via DmaCopy16/32,
     * so a naive byte-at-a-time loop here corrupts anything copied
     * straight from a ROM asset. Fetch the containing 4-byte-aligned
     * word instead (always a reliable access, and just as correct for
     * ordinary RAM sources) and extract however many of its bytes are
     * needed before advancing to the next one. */
    uint8_t *d = (uint8_t *)dst;
    uintptr_t addr = (uintptr_t)src;
    size_t i = 0;
    while (i < n) {
        uint32_t w = ReadRomWord((const void *)(addr & ~(uintptr_t)3));
        int byteOffset = (int)(addr & 3);
        size_t bytesFromThisWord = (size_t)(4 - byteOffset);
        if (bytesFromThisWord > n - i)
            bytesFromThisWord = n - i;
        for (size_t j = 0; j < bytesFromThisWord; j++)
            d[i + j] = (uint8_t)(w >> (24 - (byteOffset + (int)j) * 8));
        i += bytesFromThisWord;
        addr += bytesFromThisWord;
    }
    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) {
        /* Same non-overlapping shape as memcpy -- src may be ROM, so
         * route it through the same word-read/byte-extract path. */
        memcpy(dst, src, n);
    } else if (d > s) {
        /* Overlapping and copying backward: dst and src must be in the
         * same buffer (ROM can't overlap a RAM destination), so src is
         * always RAM here and plain byte access is fine. */
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

void *memset(void *dst, int c, size_t n)
{
    uint8_t *d = (uint8_t *)dst;
    uint8_t  v = (uint8_t)c;
    /* Word-aligned fast path */
    size_t i = 0;
    while (i < n && ((uintptr_t)(d + i) & 3)) { d[i++] = v; }
    uint32_t wv = (uint32_t)v | ((uint32_t)v << 8) |
                  ((uint32_t)v << 16) | ((uint32_t)v << 24);
    while (i + 4 <= n) {
        *(uint32_t *)(d + i) = wv;
        i += 4;
    }
    while (i < n) { d[i++] = v; }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const uint8_t *p = (const uint8_t *)a;
    const uint8_t *q = (const uint8_t *)b;
    for (size_t i = 0; i < n; i++) {
        if (p[i] != q[i]) return (int)p[i] - (int)q[i];
    }
    return 0;
}

void *memchr(const void *s, int c, size_t n)
{
    const uint8_t *p = (const uint8_t *)s;
    for (size_t i = 0; i < n; i++)
        if (p[i] == (uint8_t)c) return (void *)(p + i);
    return NULL;
}

/* -----------------------------------------------------------------------
 * String functions
 * --------------------------------------------------------------------- */
size_t strlen(const char *s)
{
    size_t n = 0;
    while (*s++) n++;
    return n;
}

size_t strnlen(const char *s, size_t max)
{
    size_t n = 0;
    while (n < max && s[n]) n++;
    return n;
}

char *strcpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    size_t i = 0;
    while (i < n && src[i]) { dst[i] = src[i]; i++; }
    while (i < n) dst[i++] = '\0';
    return dst;
}

char *strcat(char *dst, const char *src)
{
    char *d = dst + strlen(dst);
    while ((*d++ = *src++));
    return dst;
}

char *strncat(char *dst, const char *src, size_t n)
{
    char *d = dst + strlen(dst);
    size_t i = 0;
    while (i < n && src[i]) { d[i] = src[i]; i++; }
    d[i] = '\0';
    return dst;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    while (n && *a && *a == *b) { a++; b++; n--; }
    if (!n) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

char *strchr(const char *s, int c)
{
    for (; *s; s++)
        if (*s == (char)c) return (char *)s;
    return (c == '\0') ? (char *)s : NULL;
}

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    for (; *s; s++)
        if (*s == (char)c) last = s;
    return (c == '\0') ? (char *)s : (char *)last;
}

char *strstr(const char *hay, const char *needle)
{
    size_t nlen = strlen(needle);
    if (!nlen) return (char *)hay;
    for (; *hay; hay++)
        if (strncmp(hay, needle, nlen) == 0) return (char *)hay;
    return NULL;
}

/* -----------------------------------------------------------------------
 * Math / stdlib helpers
 * --------------------------------------------------------------------- */
int abs(int x)   { return x < 0 ? -x : x; }
long labs(long x) { return x < 0 ? -x : x; }

/* Simple pseudo-random number generator (xorshift32) */
static uint32_t sRandState = 0x12345678;
int rand(void)
{
    sRandState ^= sRandState << 13;
    sRandState ^= sRandState >> 17;
    sRandState ^= sRandState << 5;
    return (int)(sRandState & 0x7FFFFFFF);
}

void srand(unsigned int seed)
{
    sRandState = seed ? seed : 0x12345678;
}

/* atoi — used in some game paths */
int atoi(const char *s)
{
    int sign = 1, result = 0;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') { s++; }
    while (*s >= '0' && *s <= '9')
        result = result * 10 + (*s++ - '0');
    return sign * result;
}

/* -----------------------------------------------------------------------
 * Minimal printf / sprintf
 * The game uses DebugPrintf which expands to nothing (NDEBUG build),
 * but some code paths call sprintf for string formatting.
 * We provide a tiny implementation covering %d, %u, %x, %s, %c.
 * --------------------------------------------------------------------- */
static char *fmt_uint(char *buf, unsigned long val, int base, int upper)
{
    static const char *lc = "0123456789abcdef";
    static const char *uc = "0123456789ABCDEF";
    const char *digits = upper ? uc : lc;
    char tmp[32];
    int  i = 0;
    if (!val) { buf[0] = '0'; buf[1] = '\0'; return buf; }
    while (val) { tmp[i++] = digits[val % base]; val /= base; }
    for (int j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
    buf[i] = '\0';
    return buf;
}

int vsnprintf(char *buf, size_t size, const char *fmt, __builtin_va_list ap)
{
    size_t pos = 0;
    char tmp[32];

#define PUT(c) do { if (pos + 1 < size) buf[pos++] = (c); } while (0)

    for (; *fmt; fmt++) {
        if (*fmt != '%') { PUT(*fmt); continue; }
        fmt++;
        if (*fmt == '%') { PUT('%'); continue; }

        int width = 0, zero_pad = 0;
        if (*fmt == '0') { zero_pad = 1; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') { width = width*10 + (*fmt++ - '0'); }

        switch (*fmt) {
        case 'd': case 'i': {
            long v = __builtin_va_arg(ap, int);
            int neg = (v < 0); if (neg) v = -v;
            fmt_uint(tmp, (unsigned long)v, 10, 0);
            int len = (int)strlen(tmp) + neg;
            if (zero_pad) for (int i = len; i < width; i++) PUT('0');
            else          for (int i = len; i < width; i++) PUT(' ');
            if (neg) PUT('-');
            for (char *p = tmp; *p; p++) PUT(*p);
            break;
        }
        case 'u': {
            unsigned long v = __builtin_va_arg(ap, unsigned int);
            fmt_uint(tmp, v, 10, 0);
            int len = (int)strlen(tmp);
            for (int i = len; i < width; i++) PUT(zero_pad ? '0' : ' ');
            for (char *p = tmp; *p; p++) PUT(*p);
            break;
        }
        case 'x': case 'X': {
            unsigned long v = __builtin_va_arg(ap, unsigned int);
            fmt_uint(tmp, v, 16, (*fmt == 'X'));
            int len = (int)strlen(tmp);
            for (int i = len; i < width; i++) PUT(zero_pad ? '0' : ' ');
            for (char *p = tmp; *p; p++) PUT(*p);
            break;
        }
        case 's': {
            const char *s = __builtin_va_arg(ap, const char *);
            if (!s) s = "(null)";
            int len = (int)strlen(s);
            for (int i = len; i < width; i++) PUT(' ');
            while (*s) PUT(*s++);
            break;
        }
        case 'c': {
            char c = (char)__builtin_va_arg(ap, int);
            PUT(c);
            break;
        }
        case 'p': {
            unsigned long v = (unsigned long)__builtin_va_arg(ap, void *);
            fmt_uint(tmp, v, 16, 0);
            PUT('0'); PUT('x');
            for (char *p = tmp; *p; p++) PUT(*p);
            break;
        }
        default:
            PUT('%'); PUT(*fmt);
            break;
        }
    }
#undef PUT
    if (size > 0) buf[pos < size ? pos : size - 1] = '\0';
    return (int)pos;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int r = vsnprintf(buf, size, fmt, ap);
    __builtin_va_end(ap);
    return r;
}

int sprintf(char *buf, const char *fmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int r = vsnprintf(buf, (size_t)-1, fmt, ap);
    __builtin_va_end(ap);
    return r;
}

/* printf — output to nothing (no UART on N64 in our port) */
int printf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

/* -----------------------------------------------------------------------
 * Math functions (subset used by m4a.c and game code)
 * --------------------------------------------------------------------- */
#include <math.h>  /* Use compiler's math.h for declarations */

/* Wrappers that use MIPS FPU instructions */
double sqrt(double x)
{
    /* MIPS sqrt.d instruction */
    double r;
    asm volatile ("sqrt.d %0, %1" : "=f"(r) : "f"(x));
    return r;
}

float sqrtf(float x)
{
    float r;
    asm volatile ("sqrt.s %0, %1" : "=f"(r) : "f"(x));
    return r;
}

/* -----------------------------------------------------------------------
 * sin / cos
 *
 * BUG THIS REPLACES: `double sin(double x) { return __builtin_sin(x); }`.
 * GCC has no software fallback for __builtin_sin/__builtin_cos on a
 * double in a freestanding build (no libm) -- it just lowers the builtin
 * to a call to the C library function named "sin"/"cos".  Since THIS
 * function IS named sin(), that is a call to itself: a tail call with no
 * base case, which -O2 turns into a literal `j <self>` instruction --
 * an infinite loop baked directly into the binary, no recursion/stack
 * overflow, no crash, nothing but a permanently spinning CPU the instant
 * anything calls sin() or cos() (ObjAffineSet, used for sprite rotation/
 * scale, is the first caller reached in the intro sequence).  sinf/cosf
 * inherit the same bug since they just call through to __builtin_sin on
 * a widened double, i.e. through this same broken path.
 *
 * Real implementation: reduce x to [-pi, pi], then Taylor series.  Inputs
 * in this codebase come from GBA-style angle-as-s16 conversions
 * (angle = rotation/32768 * pi) so they already land in roughly that
 * range before this ever runs; the reduction below just makes the
 * function correct for arbitrary inputs too.
 * --------------------------------------------------------------------- */
/* NOT plain double literals: GCC pools an arbitrary (non-trivially-integer)
 * double constant like 3.14159... in .rodata and loads it with `ldc1`, a
 * 64-bit load.  .rodata lives in ROM (KSEG1 cart space) on this port, and
 * real N64 PI-bus hardware cannot do a 64-bit transaction in one go --
 * ares enforces this accurately and freezes the CPU the instant it sees a
 * 64-bit read land outside RDRAM ("[Bus::freezeDualRead] CPU frozen
 * because of 64-bit read from non-RDRAM area"), which is exactly what
 * cosf's `x + N64_HALF_PI` did the moment anything called it.
 *
 * Two things that do NOT avoid this, both tried and confirmed still
 * producing an ldc1 from ROM in the built ELF:
 *   - (double)355/(double)113: 355 and 113 are compile-time constants, so
 *     -O2 constant-folds the whole division into the same kind of double
 *     literal at compile time and pools THAT instead.
 *   - a union type-pun of two uint32_t halves into a double, computed
 *     inline: GCC's constant folder evaluates the union access at compile
 *     time too when every input is a literal, right back to a poolable
 *     double constant.
 *
 * What actually works: a genuinely mutable (non-const) global.  The
 * compiler can never assume it knows a non-const global's value at
 * compile time -- another translation unit could have written to it --
 * so a read can never be folded into a literal, and non-const data goes
 * into .data/.bss (RDRAM) rather than .rodata (ROM) in the first place.
 * Computed once, lazily, from the same raw-bits union (now unfoldable
 * since it feeds a variable the optimizer must treat as opaque) and
 * cached for subsequent calls.
 *
 * volatile on cache/ready is load-bearing, not defensive: without it,
 * -O2's interprocedural constant propagation sees straight through the
 * whole "lazy init" idiom anyway.  Neither static ever has its address
 * escape this function, so once inlined the optimizer can prove ready
 * starts at 0, prove the if-branch always runs on the (only reachable)
 * first call, evaluate that branch's union at compile time same as
 * before, and fold the result into a plain pooled double right back
 * where we started (confirmed: still ldc1 from ROM without volatile).
 * volatile forbids exactly that: a volatile read or write is an
 * observable effect the compiler must actually perform and can never
 * assume the value of ahead of time, which blocks the fold.
 *
 * That alone still was not enough: hi/lo themselves are plain (non-
 * volatile) parameters, so GCC is still free to evaluate the union
 * access at compile time -- it just moved to computing that constant
 * ahead of time and pooling THAT into .rodata for the one-time init
 * (confirmed: still ldc1 from ROM, just relocated to the lazy-init
 * branch instead of every call).  Routing the write and the read-back
 * through volatile-qualified pointers forces both to be genuine runtime
 * memory operations GCC cannot fold through, so the double is actually
 * assembled at runtime from register-held integers via real stores and a
 * real load -- to the stack (RDRAM), never to ROM. */
/* Small-integer-valued doubles (1.0, 2.0, ...) turn out to need the same
 * care: confirmed by rebuilding and re-checking the disassembly, GCC
 * still pools some of these via .rodata + ldc1 in this codebase/version
 * rather than always preferring the (cheaper, register-only) cvt.d.w
 * path -- e.g. atan()'s two `1.0`s and one `2.0` did, even though the
 * Taylor series loops' `(double)(2*n+1)` right next to them (an int
 * variable, not a literal) did not.  Routing the literal through a
 * volatile int forces a genuine runtime conversion GCC cannot fold: the
 * value is unknowable at compile time, so cvt.d.w is the only option
 * left, and no ROM access is ever involved. */
static double n64_int2dbl(int n)
{
    volatile int vn = n;
    return (double)vn;
}

static double n64_lazy_double(volatile double *cache, volatile int *ready, uint32_t hi, uint32_t lo)
{
    if (!*ready) {
        union { struct { uint32_t hi, lo; } w; double d; } u;  /* big-endian: hi first */
        volatile uint32_t *vw = (volatile uint32_t *)&u;
        vw[0] = hi;
        vw[1] = lo;
        *cache = *(volatile double *)&u;
        *ready = 1;
    }
    return *cache;
}

static double n64_pi(void)
{
    static volatile double cache; static volatile int ready;
    return n64_lazy_double(&cache, &ready, 0x400921FBu, 0x54442D18u); /* 3.14159265358979323846 */
}

static double n64_two_pi(void)
{
    static volatile double cache; static volatile int ready;
    return n64_lazy_double(&cache, &ready, 0x401921FBu, 0x54442D18u); /* 6.28318530717958647692 */
}

static double n64_half_pi(void)
{
    static volatile double cache; static volatile int ready;
    return n64_lazy_double(&cache, &ready, 0x3FF921FBu, 0x54442D18u); /* 1.57079632679489661923 */
}

#define N64_PI      (n64_pi())
#define N64_TWO_PI  (n64_two_pi())
#define N64_HALF_PI (n64_half_pi())

static double n64_fmod(double x, double y)
{
    /* (int), not (long long): a 32-bit double<->int conversion compiles to
     * the MIPS FPU's native trunc.w.d/cvt.d.w instructions.  long long
     * would need libgcc's __fixdfdi/__floatdidf soft-conversion helpers,
     * which this -nostdlib build doesn't link.  32 bits comfortably
     * covers every quotient this function is actually asked to reduce
     * (bounded rotation angles), so there is no real range lost here. */
    double q = x / y;
    double iq = (double)(int)q;   /* truncate toward zero */
    return x - iq * y;
}

double sin(double x)
{
    double x2, term, sum;
    int n;

    x = n64_fmod(x, N64_TWO_PI);
    if (x > N64_PI)
        x -= N64_TWO_PI;
    else if (x < -N64_PI)
        x += N64_TWO_PI;

    /* Taylor series around 0; x is already in [-pi, pi] so this converges
     * to well within float precision in a handful of terms. */
    x2 = x * x;
    term = x;
    sum = x;
    for (n = 1; n <= 8; n++) {
        term *= -x2 / (double)((2 * n) * (2 * n + 1));
        sum += term;
    }
    return sum;
}

double cos(double x)
{
    return sin(x + N64_HALF_PI);
}

float sinf(float x) { return (float)sin((double)x); }
float cosf(float x) { return (float)cos((double)x); }
double fabs(double x) { double r; asm volatile("abs.d %0,%1":"=f"(r):"f"(x)); return r; }
float fabsf(float x)  { float r; asm volatile("abs.s %0,%1":"=f"(r):"f"(x)); return r; }

/* -----------------------------------------------------------------------
 * atan / atan2
 *
 * Same self-referential-builtin bug as sin/cos above: `atan2(y,x) {
 * return __builtin_atan2(y,x); }` lowers to a call to atan2() itself (no
 * libm in this freestanding build), which -O2 turns into an infinite
 * `j <self>` loop.  Not reached by the intro sequence, but src/n64/bios.c's
 * ArcTan2() calls through to this, and ArcTan2() is used by
 * src/battle_anim_mons.c -- so this would hang exactly the same way, just
 * later, the first time a battle animation needing it plays.
 *
 * Real implementation: atan(x) via the argument-halving identity
 * atan(x) = 2*atan(x / (1 + sqrt(1+x^2))), which shrinks any input to at
 * most tan(pi/8) =~ 0.414 in magnitude before the Taylor series below has
 * to do any work, so it converges quickly everywhere; atan2 adds the
 * standard quadrant handling atan() alone can't express.
 * --------------------------------------------------------------------- */
static double n64_atan_series(double x)
{
    double x2 = x * x, term = x, sum = x;
    int n;
    for (n = 1; n <= 12; n++) {
        term *= -x2;
        sum += term / (double)(2 * n + 1);
    }
    return sum;
}

double atan(double x)
{
    double y = x / (n64_int2dbl(1) + sqrt(n64_int2dbl(1) + x * x));
    return n64_int2dbl(2) * n64_atan_series(y);
}

double atan2(double y, double x)
{
    /* Every comparison and the final return route "0" through
     * n64_int2dbl() rather than a bare 0.0 literal: confirmed by
     * disassembly that this codebase's GCC pools even a zero comparison
     * via .rodata + ldc1 in this function, same as the nonzero constants
     * addressed above -- it is not just the "awkward" values that need
     * this treatment. */
    double zero = n64_int2dbl(0);
    if (x > zero)
        return atan(y / x);
    if (x < zero)
        return y >= zero ? atan(y / x) + N64_PI : atan(y / x) - N64_PI;
    /* x == 0 */
    if (y > zero)  return N64_HALF_PI;
    if (y < zero)  return -N64_HALF_PI;
    return zero;   /* atan2(0, 0): undefined, matches common convention */
}

float atan2f(float y, float x) { return (float)atan2((double)y, (double)x); }

/* __memcpy_chk / __memset_chk — GCC fortify stubs (should not be called with _FORTIFY_SOURCE=0) */
void *__memcpy_chk(void *dst, const void *src, __SIZE_TYPE__ n, __SIZE_TYPE__ dstlen)
{
    (void)dstlen;
    return memcpy(dst, src, n);
}
void *__memset_chk(void *dst, int c, __SIZE_TYPE__ n, __SIZE_TYPE__ dstlen)
{
    (void)dstlen;
    return memset(dst, c, n);
}

/* -----------------------------------------------------------------------
 * 64-bit integer arithmetic helpers (MIPS soft-float ABI)
 * GCC may emit calls to these for 64-bit divisions / conversions.
 * --------------------------------------------------------------------- */
/* Signed 64-bit division */
long long __divdi3(long long a, long long b)
{
    if (b == 0) return 0;
    int neg = 0;
    unsigned long long ua = (unsigned long long)a;
    unsigned long long ub = (unsigned long long)b;
    if (a < 0) { ua = (unsigned long long)(-a); neg ^= 1; }
    if (b < 0) { ub = (unsigned long long)(-b); neg ^= 1; }
    /* Simple long division */
    unsigned long long q = 0, r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((ua >> i) & 1);
        if (r >= ub) { r -= ub; q |= (1ULL << i); }
    }
    return neg ? -(long long)q : (long long)q;
}

/* Unsigned 64-bit division */
unsigned long long __udivdi3(unsigned long long a, unsigned long long b)
{
    if (b == 0) return 0;
    unsigned long long q = 0, r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((a >> i) & 1);
        if (r >= b) { r -= b; q |= (1ULL << i); }
    }
    return q;
}

/* __bswapsi2 — byte-swap 32-bit value (GCC builtins may call this) */
unsigned int __bswapsi2(unsigned int x)
{
    return ((x & 0xFF000000u) >> 24)
         | ((x & 0x00FF0000u) >>  8)
         | ((x & 0x0000FF00u) <<  8)
         | ((x & 0x000000FFu) << 24);
}

/* -----------------------------------------------------------------------
 * Heap stubs — the game uses its own InitHeap()/AllocInternal() in malloc.c
 * We only need these if something calls stdlib malloc directly.
 * --------------------------------------------------------------------- */
void *malloc(size_t size) { (void)size; return NULL; }
void  free(void *p)       { (void)p; }

/* -----------------------------------------------------------------------
 * exit — shouldn't be called; if it is, halt the CPU.
 * --------------------------------------------------------------------- */
void exit(int code)
{
    (void)code;
    for (;;) asm volatile ("wait");
}

/* -----------------------------------------------------------------------
 * __assert_fail — GCC may emit calls to this for assert().
 * --------------------------------------------------------------------- */
void __assert_fail(const char *expr, const char *file,
                   unsigned int line, const char *func)
{
    (void)expr; (void)file; (void)line; (void)func;
    for (;;) asm volatile ("wait");
}
