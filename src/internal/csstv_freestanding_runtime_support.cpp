#include "csstv_freestanding_runtime_support.h"

#if CSSTV_NO_CXX_STDLIB

#include <stddef.h>

/*
 * Runtime stubs required when linking C++ without libstdc++/libsupc++.
 *
 * Plain operator new/delete should never run in this codebase (only
 * placement new is used). The deleting destructor emitted for classes
 * with a virtual destructor still references them as vtable slots, so
 * the linker needs definitions. Infinite-loop stubs keep a mistaken
 * call from returning corrupted memory.
 *
 * __cxa_pure_virtual similarly fills the pure-virtual vtable slot that
 * every abstract ModeDriver / DecoderDriver carries.
 *
 * Symbols are weak so targets that still pull in a partial libstdc++
 * (some AVR packages) do not hit multiple-definition errors.
 */

__attribute__((weak)) void *operator new(size_t)
{
    /* cppcheck-suppress infiniteLoop */
    for (;;)
    {
    }
}

__attribute__((weak)) void *operator new[](size_t)
{
    /* cppcheck-suppress infiniteLoop */
    for (;;)
    {
    }
}

__attribute__((weak)) void operator delete(void *) noexcept {}

__attribute__((weak)) void operator delete(void *, size_t) noexcept {}

__attribute__((weak)) void operator delete[](void *) noexcept {}

__attribute__((weak)) void operator delete[](void *, size_t) noexcept {}

extern "C" __attribute__((weak)) void __cxa_pure_virtual(void)
{
    /* cppcheck-suppress infiniteLoop */
    for (;;)
    {
    }
}

#endif /* CSSTV_NO_CXX_STDLIB */
