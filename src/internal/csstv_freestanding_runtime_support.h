#ifndef CSSTV_FREESTANDING_RUNTIME_SUPPORT_H
#define CSSTV_FREESTANDING_RUNTIME_SUPPORT_H

/*
 * Freestanding / no-libstdc++ support.
 *
 * AVR and bare-metal RISC-V toolchains typically ship without libstdc++,
 * so <new> is unavailable. CSSTV_NO_CXX_STDLIB selects the fallbacks
 * below (and the definitions in the matching .cpp).
 *
 * Auto-enabled for AVR and RISC-V; can also be forced on a hosted
 * toolchain (see the AVR Fallback Correctness CI job) to exercise the
 * same code path natively.
 */

#if (defined(__AVR__) || defined(__riscv)) && !defined(CSSTV_NO_CXX_STDLIB)
#define CSSTV_NO_CXX_STDLIB 1
#endif

#ifndef CSSTV_NO_CXX_STDLIB
#define CSSTV_NO_CXX_STDLIB 0
#endif

#if CSSTV_NO_CXX_STDLIB

#include <stddef.h>

/* Placement new is a library facility normally declared in <new>. */
inline void *operator new(size_t, void *ptr) noexcept
{
    return ptr;
}

inline void operator delete(void *, void *) noexcept {}

#endif /* CSSTV_NO_CXX_STDLIB */

#endif /* CSSTV_FREESTANDING_RUNTIME_SUPPORT_H */
