// The C library functions the game image imports, bound to macOS's C library.
//
// The image is an AArch64 Linux ELF. Apple's arm64 ABI is AAPCS64 with a few differences, and only
// these matter for the imports:
//   - variadic arguments: Linux passes them in registers, Apple on the stack, and the two va_list
//     types differ. printf, fprintf, sprintf and snprintf go through mu_linux_varargs.S, vsnprintf and
//     vsprintf take the Linux va_list directly; both end in the formatter below, which reads the
//     arguments the AAPCS64 way and prints each conversion with macOS's own snprintf.
//   - arguments narrower than 32 bits: none of these functions has one.
// long and size_t are 64-bit on both, and FILE* is only ever passed back to the library that made it.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// AAPCS64's va_list (what the Linux side's va_start makes).
typedef struct { char* stack; char* gr_top; char* vr_top; int gr_offs; int vr_offs; } lx_va;

static uint64_t lx_gp(lx_va* va) {
  uint64_t v;
  if (va->gr_offs < 0) { memcpy(&v, va->gr_top + va->gr_offs, 8); va->gr_offs += 8; return v; }
  memcpy(&v, va->stack, 8); va->stack += 8; return v;
}
static double lx_fp(lx_va* va) {
  double v;
  if (va->vr_offs < 0) { memcpy(&v, va->vr_top + va->vr_offs, 8); va->vr_offs += 16; return v; }
  memcpy(&v, va->stack, 8); va->stack += 8; return v;
}
static void lx_skip_quad(lx_va* va) {        // a long double (binary128 on Linux): not printable here
  if (va->vr_offs < 0) { va->vr_offs += 16; return; }
  va->stack = (char*)(((uintptr_t)va->stack + 15) & ~(uintptr_t)15) + 16;
}

typedef struct { char* out; size_t cap, len; } Sink;
static void put(Sink* s, const char* text, size_t n) {
  if (s->cap && s->len + 1 < s->cap) {
    size_t room = s->cap - 1 - s->len;
    memcpy(s->out + s->len, text, n < room ? n : room);
  }
  s->len += n;
}

// vsnprintf semantics: writes at most cap bytes including the terminator, returns the full length.
static int lx_format(char* out, size_t cap, const char* format, lx_va* va) {
  Sink s = {out, cap, 0};
  char spec[64], piece[512];
  for (const char* p = format; *p;) {
    if (*p != '%') { const char* q = strchr(p, '%'); size_t n = q ? (size_t)(q - p) : strlen(p); put(&s, p, n); p += n; continue; }
    if (p[1] == '%') { put(&s, "%", 1); p += 2; continue; }
    size_t k = 0; spec[k++] = *p++;
    while (*p && strchr("-+ #0'", *p) && k < 40) spec[k++] = *p++;
    if (*p == '*') { k += (size_t)snprintf(spec + k, sizeof spec - k, "%d", (int)lx_gp(va)); ++p; }
    else while (*p >= '0' && *p <= '9' && k < 40) spec[k++] = *p++;
    if (*p == '.') {
      spec[k++] = *p++;
      if (*p == '*') { k += (size_t)snprintf(spec + k, sizeof spec - k, "%d", (int)lx_gp(va)); ++p; }
      else while (*p >= '0' && *p <= '9' && k < 50) spec[k++] = *p++;
    }
    int longs = 0, halfs = 0, quad = 0;
    for (;; ++p) {
      if (*p == 'l' || *p == 'z' || *p == 'j' || *p == 't' || *p == 'q') ++longs;
      else if (*p == 'h') ++halfs;
      else if (*p == 'L') quad = 1;
      else break;
    }
    const char conv = *p;
    if (!conv) break;
    ++p;
    int n = 0;
    switch (conv) {
      case 'd': case 'i': {
        const uint64_t raw = lx_gp(va);
        long long v = longs ? (long long)raw : halfs == 1 ? (short)raw : halfs >= 2 ? (signed char)raw : (int)raw;
        spec[k++] = 'l'; spec[k++] = 'l'; spec[k++] = conv; spec[k] = 0;
        n = snprintf(piece, sizeof piece, spec, v); break;
      }
      case 'u': case 'o': case 'x': case 'X': {
        const uint64_t raw = lx_gp(va);
        unsigned long long v = longs ? raw : halfs == 1 ? (unsigned short)raw : halfs >= 2 ? (unsigned char)raw : (unsigned int)raw;
        spec[k++] = 'l'; spec[k++] = 'l'; spec[k++] = conv; spec[k] = 0;
        n = snprintf(piece, sizeof piece, spec, v); break;
      }
      case 'c': spec[k++] = 'c'; spec[k] = 0; n = snprintf(piece, sizeof piece, spec, (int)lx_gp(va)); break;
      case 'e': case 'E': case 'f': case 'F': case 'g': case 'G': case 'a': case 'A':
        if (quad) { lx_skip_quad(va); n = snprintf(piece, sizeof piece, "<long double>"); break; }
        spec[k++] = conv; spec[k] = 0; n = snprintf(piece, sizeof piece, spec, lx_fp(va)); break;
      case 's': {
        const char* str = (const char*)(uintptr_t)lx_gp(va);
        spec[k++] = 's'; spec[k] = 0;
        n = snprintf(NULL, 0, spec, str ? str : "(null)");
        if (n >= (int)sizeof piece) {     // long strings: print straight into a big enough buffer
          char* big = malloc((size_t)n + 1);
          if (big) { snprintf(big, (size_t)n + 1, spec, str ? str : "(null)"); put(&s, big, (size_t)n); free(big); }
          continue;
        }
        n = snprintf(piece, sizeof piece, spec, str ? str : "(null)"); break;
      }
      case 'p': spec[k++] = 'p'; spec[k] = 0; n = snprintf(piece, sizeof piece, spec, (void*)(uintptr_t)lx_gp(va)); break;
      case 'n': { void* where = (void*)(uintptr_t)lx_gp(va); if (where) *(int*)where = (int)s.len; continue; }
      default: put(&s, spec, k); put(&s, &conv, 1); continue;
    }
    if (n > 0) put(&s, piece, (size_t)n < sizeof piece ? (size_t)n : sizeof piece - 1);
  }
  if (cap) out[s.len < cap ? s.len : cap - 1] = 0;
  return (int)s.len;
}

int mu_vsnprintf_lx(char* out, size_t cap, const char* format, lx_va* va) { lx_va copy = *va; return lx_format(out, cap, format, &copy); }
int mu_vsprintf_lx(char* out, const char* format, lx_va* va) { lx_va copy = *va; return lx_format(out, (size_t)1 << 30, format, &copy); }
int mu_vfprintf_lx(FILE* file, const char* format, lx_va* va) {
  lx_va copy = *va; char small[1024];
  const int n = lx_format(small, sizeof small, format, &copy);
  if (n < 0) return n;
  if ((size_t)n < sizeof small) return (int)fwrite(small, 1, (size_t)n, file) == n ? n : -1;
  char* big = malloc((size_t)n + 1);
  if (!big) return -1;
  copy = *va; lx_format(big, (size_t)n + 1, format, &copy);
  const int written = (int)fwrite(big, 1, (size_t)n, file);
  free(big);
  return written == n ? n : -1;
}
int mu_vprintf_lx(const char* format, lx_va* va) { return mu_vfprintf_lx(stdout, format, va); }

int mu_lx_printf(const char*, ...);       // mu_linux_varargs.S (the prototypes only name them)
int mu_lx_fprintf(FILE*, const char*, ...);
int mu_lx_sprintf(char*, const char*, ...);
int mu_lx_snprintf(char*, size_t, const char*, ...);

// The image references the stack protector's global guard (GCC on AArch64 Linux), if built with it.
uintptr_t mu_stack_chk_guard = 0x595e9fbd94fda766ull;
static void mu_stack_chk_fail(void) { fputs("game image: stack protector check failed\n", stderr); abort(); }

struct MuImport { const char* name; void* address; };
#define F(f) { #f, (void*)&f }
static const struct MuImport table[] = {
  F(malloc), F(free), F(memchr), F(memcmp), F(memcpy), F(memmove), F(memset),
  F(strcat), F(strchr), F(strcmp), F(strcpy), F(strlen), F(strncmp), F(strncpy), F(strrchr),
  F(strtoul), F(atoi), F(getenv),
  F(fopen), F(fclose), F(fflush), F(fwrite),
  { "fprintf", (void*)&mu_lx_fprintf }, { "printf", (void*)&mu_lx_printf },
  { "snprintf", (void*)&mu_lx_snprintf }, { "sprintf", (void*)&mu_lx_sprintf },
  { "vsnprintf", (void*)&mu_vsnprintf_lx }, { "vsprintf", (void*)&mu_vsprintf_lx },
  // fabs and fabsf clear the sign bit; sqrtf is correctly rounded by IEEE 754. No other maths comes
  // from the C library: the simulation's own routines are inside the image.
  F(fabs), F(fabsf), F(sqrtf),
  { "__stack_chk_guard", &mu_stack_chk_guard },
  { "__stack_chk_fail", (void*)&mu_stack_chk_fail },
};

void* mu_import(const char* name) {
  for (size_t i = 0; i < sizeof table / sizeof table[0]; ++i)
    if (!strcmp(name, table[i].name)) return table[i].address;
  return NULL;
}
