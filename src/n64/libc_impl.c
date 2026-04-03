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

/* -----------------------------------------------------------------------
 * Memory functions
 * --------------------------------------------------------------------- */
void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else if (d > s) {
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

double sin(double x)  { return __builtin_sin(x); }
double cos(double x)  { return __builtin_cos(x); }
double fabs(double x) { double r; asm volatile("abs.d %0,%1":"=f"(r):"f"(x)); return r; }
float fabsf(float x)  { float r; asm volatile("abs.s %0,%1":"=f"(r):"f"(x)); return r; }

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
