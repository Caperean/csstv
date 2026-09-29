# CSSTV ABI Documentation

## Version
- ABI Version: 1.0
- Library Version: 0.1.4
- Date: 2026

## Overview

This document describes the Application Binary Interface (ABI) for the CSSTV library. The ABI defines the binary compatibility guarantees between the library and applications that use it.

## Binary Compatibility Guarantees

### Stable ABI Components

The following ABI components are guaranteed to remain stable within the 0.1.x release series:

- **Public C API function signatures**
- **Public structure layouts** (csstv_encoder_t, csstv_decoder_t, csstv_image_t)
- **Alignment requirements** (16-byte alignment for encoder/decoder handles)
- **Storage sizes** (CSSTV_ENCODER_STORAGE_SIZE, CSSTV_DECODER_STORAGE_SIZE)
- **Enum values** (csstv_mode_t, csstv_pixel_format_t, csstv_status_t)
- **Public constants and macros**

### Unstable ABI Components

The following are NOT guaranteed to remain stable:

- **Internal C++ class layouts** (EncoderState, DecoderState, ModeDriver)
- **Private structure fields**
- **Vtable layouts** (may change with compiler/optimization level)
- **Internal function names** (not part of public API)
- **Compiler-specific optimizations**

## Type Definitions

### Encoder Handle

```c
typedef struct {
    csstv_encoder_alignment_t alignment;
    uint8_t storage[CSSTV_ENCODER_STORAGE_SIZE];
} csstv_encoder_t;
```

**Properties:**
- Size: 512 bytes (CSSTV_ENCODER_STORAGE_SIZE)
- Alignment: 16 bytes (minimum, enforced by csstv_encoder_alignment_t)
- Placement: Stack-allocated or embedded in caller-owned storage
- Lifetime: Managed by caller via init/deinit functions

**Storage Layout:**
```
Offset 0-15:    alignment (csstv_encoder_alignment_t)
Offset 16-511:  storage (internal state)
```

### Decoder Handle

```c
typedef struct {
    csstv_decoder_alignment_t alignment;
    uint8_t storage[CSSTV_DECODER_STORAGE_SIZE];
} csstv_decoder_t;
```

**Properties:**
- Size: 4096 bytes (CSSTV_DECODER_STORAGE_SIZE)
- Alignment: 16 bytes (minimum, enforced by csstv_decoder_alignment_t)
- Placement: Stack-allocated or embedded in caller-owned storage
- Lifetime: Managed by caller via init/deinit functions

**Storage Layout:**
```
Offset 0-15:    alignment (csstv_decoder_alignment_t)
Offset 16-4111: storage (internal state)
```

### Image Structure

```c
typedef struct {
    uint16_t width;
    uint16_t height;
    csstv_pixel_format_t format;
    size_t stride;
    uint8_t *data;
} csstv_image_t;
```

**Properties:**
- Size: 24 bytes (on 64-bit platforms)
- Alignment: Default (8 bytes on 64-bit)
- Placement: Caller-owned memory
- Lifetime: Must remain valid during encoder/decoder operations

**Storage Layout (64-bit):**
```
Offset 0-1:     width (uint16_t)
Offset 2-3:     height (uint16_t)
Offset 4-7:     format (csstv_pixel_format_t)
Offset 8-15:    stride (size_t)
Offset 16-23:   data (uint8_t*)
```

## Mode Information

```c
typedef struct {
    csstv_mode_t mode;
    uint16_t width;
    uint16_t height;
    uint32_t duration_ms;
} csstv_mode_info_t;
```

**Properties:**
- Size: 12 bytes
- Alignment: Default (4 bytes)
- Placement: Stack-allocated or caller-owned

**Storage Layout:**
```
Offset 0-3:     mode (csstv_mode_t)
Offset 4-5:     width (uint16_t)
Offset 6-7:     height (uint16_t)
Offset 8-11:    duration_ms (uint32_t)
```

## Storage Requirements

### Encoder Storage

**Minimum Size:** 512 bytes
**Actual Usage:** ~300-400 bytes depending on mode
**Padding:** Reserved for future expansion

**Memory Usage Breakdown:**
- EncoderState structure: ~120 bytes
- ModeDriver (PD): ~250-350 bytes
- Alignment padding: ~16 bytes
- Reserved space: ~100+ bytes

### Decoder Storage

**Minimum Size:** 4096 bytes
**Actual Usage:** ~3500-3800 bytes depending on mode
**Padding:** Reserved for future expansion

**Memory Usage Breakdown:**
- DecoderState structure: ~120 bytes
- DecoderDriver (PD): ~3000-3500 bytes
- Per-line buffers (PD290): ~2800 bytes
- Alignment padding: ~16 bytes
- Reserved space: ~200+ bytes

## Alignment Requirements

### Encoder Handle
- **Minimum Alignment:** 16 bytes
- **Recommended:** 16 bytes
- **Enforced by:** csstv_encoder_alignment_t union

### Decoder Handle
- **Minimum Alignment:** 16 bytes
- **Recommended:** 16 bytes
- **Enforced by:** csstv_decoder_alignment_t union

### Image Data
- **Minimum Alignment:** No specific requirement
- **Recommended:** Natural alignment for pixel format
- **Notes:** 1-byte alignment for grayscale, 4-byte for RGB888 recommended

## Platform-Specific Notes

### x86-64 (Linux, Windows, macOS)
- **Pointer Size:** 8 bytes
- **Size_t:** 8 bytes
- **Alignment:** 16 bytes for encoder/decoder handles
- **Endianness:** Little-endian

### x86-32 (Linux, Windows)
- **Pointer Size:** 4 bytes
- **Size_t:** 4 bytes
- **Alignment:** 16 bytes for encoder/decoder handles
- **Endianness:** Little-endian

### ARM (32-bit, 64-bit)
- **Pointer Size:** 4 bytes (32-bit), 8 bytes (64-bit)
- **Size_t:** 4 bytes (32-bit), 8 bytes (64-bit)
- **Alignment:** 16 bytes for encoder/decoder handles
- **Endianness:** Little-endian (typically)

### AVR (8-bit)
- **Pointer Size:** 2 bytes (near), 3 bytes (far)
- **Size_t:** 2 bytes
- **Alignment:** Natural (1-2 bytes)
- **Endianness:** Little-endian
- **Special:** Uses freestanding C headers only

### RISC-V (32-bit, 64-bit)
- **Pointer Size:** 4 bytes (32-bit), 8 bytes (64-bit)
- **Size_t:** 4 bytes (32-bit), 8 bytes (64-bit)
- **Alignment:** 16 bytes for encoder/decoder handles
- **Endianness:** Little-endian (typically)
- **Special:** Uses freestanding C headers with picolibc

## Compiler Compatibility

### Supported Compilers
- **GCC:** 7.0+
- **Clang:** 5.0+
- **MSVC:** 2017+
- **Arm CC:** 6.0+

### C++ Standard
- **Required:** C++17
- **ABI Impact:** Inline placement new, alignas, constexpr

### C Standard
- **Required:** C99 (for public API)
- **ABI Impact:** Struct layout, enum values

## Versioning Policy

### Semantic Versioning
- **Major (X):** Breaking ABI changes
- **Minor (X.Y):** New features, ABI compatible
- **Patch (X.Y.Z):** Bug fixes, ABI compatible

### Current ABI Version: 1.0
Compatible with library versions 0.1.0 through 0.1.x

### ABI Breaking Changes
The following require a major version bump:
- Changing csstv_encoder_t or csstv_decoder_t size
- Changing csstv_image_t layout
- Adding/removing public enum values
- Changing function signatures
- Changing alignment requirements

### ABI Compatible Changes
The following do NOT require a major version bump:
- Adding new public functions
- Increasing storage sizes (reserved space)
- Bug fixes in implementation
- Performance optimizations
- Adding new modes

## Dynamic Linking Considerations

### Symbol Visibility
- **Public API:** Exported symbols (csstv_*)
- **Internal API:** Hidden symbols (csstv::*)
- **C++ Namespaces:** Mangled, not part of C ABI

### Exported Symbols
```
csstv_encoder_init
csstv_encoder_init_auto
csstv_encoder_set_image
csstv_encoder_read
csstv_encoder_finished
csstv_encoder_reset
csstv_encoder_deinit
csstv_decoder_init
csstv_decoder_init_auto
csstv_decoder_set_image
csstv_decoder_write
csstv_decoder_finished
csstv_decoder_get_image
csstv_decoder_reset
csstv_decoder_deinit
csstv_decoder_get_detected_mode
csstv_mode_supported
csstv_mode_get_info
csstv_version_string
```

### Static Linking
- **Recommended:** Yes (no external dependencies)
- **Benefits:** No ABI concerns, smaller binaries
- **Notes:** Library is designed for static linking

## Memory Safety

### No Heap Allocation
- **Encoder:** All memory in stack/storage
- **Decoder:** All memory in stack/storage
- **Runtime:** No malloc/free/new/delete

### Placement Construction
- **Pattern:** `new (storage) Type(...)` for internal objects
- **Destruction:** Explicit destructor calls
- **Benefits:** Deterministic memory usage, no fragmentation

### Thread Safety
- **Current:** Not thread-safe
- **Usage:** One encoder/decoder per thread
- **Future:** May add thread-safe variants

## Testing ABI Compatibility

### ABI Verification Tests
- Structure size checks
- Alignment checks
- Field offset verification
- Enum value validation

### Cross-Platform Testing
- x86-64 (Linux, Windows, macOS)
- x86-32 (Linux, Windows)
- ARM (32-bit, 64-bit)
- AVR (8-bit)
- RISC-V (32-bit, 64-bit)

### Compiler Testing
- GCC multiple versions
- Clang multiple versions
- MSVC multiple versions
- Cross-compilation

## Recommendations for Users

### Static Linking
```c
// Recommended: Stack allocation
csstv_encoder_t encoder;
csstv_status_t status = csstv_encoder_init(&encoder, CSSTV_MODE_PD50, 48000);
```

### Version Checking
```c
// Check library version before use
const char *version = csstv_version_string();
// Compare against expected version
```

### Storage Management
```c
// Ensure sufficient storage
static_assert(sizeof(csstv_encoder_t) >= CSSTV_ENCODER_STORAGE_SIZE);
static_assert(alignof(csstv_encoder_t) >= 16);
```

### Error Handling
```c
// Always check return values
csstv_status_t status = csstv_encoder_init(&encoder, mode, rate);
if (status != CSSTV_OK) {
    // Handle error
}
```

## Future ABI Considerations

### Potential Changes
- **Larger storage sizes:** May increase for new modes
- **New structures:** May add new public types
- **New modes:** May add mode-specific fields

### Backward Compatibility
- **Storage growth:** Will maintain minimum sizes
- **Deprecation:** Will deprecate before removal
- **Transition period:** Will support old APIs during transition

## Contact

For ABI-related questions or concerns:
- GitHub Issues: https://github.com/Caperean/csstv/issues
- Documentation: See include/csstv.h for API details
