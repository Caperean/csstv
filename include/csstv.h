```c
#ifndef CSSTV_H
#define CSSTV_H

/*
 * csstv - SSTV encoder library
 *
 * Version 0.1
 *
 * Currently implemented:
 *   - SSTV PD family
 *   - PD50
 *   - PD90
 *   - PD120
 *   - PD160
 *   - PD180
 *   - PD240
 *   - PD290
 *
 * The public API is intentionally mode-independent so that additional
 * SSTV families and decoding can be added later without breaking the API.
 *
 * The implementation may be written in C++, but this header provides
 * a C-compatible ABI.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "csstv_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Version                                                                    */
/* ========================================================================== */

#define CSSTV_VERSION_MAJOR 0U
#define CSSTV_VERSION_MINOR 1U
#define CSSTV_VERSION_PATCH 0U

#define CSSTV_VERSION_NUMBER \
    ((CSSTV_VERSION_MAJOR * 10000U) + \
     (CSSTV_VERSION_MINOR * 100U) + \
     CSSTV_VERSION_PATCH)

/* ========================================================================== */
/* Status                                                                     */
/* ========================================================================== */

typedef enum
{
    CSSTV_OK = 0,

    CSSTV_ERROR = -1,
    CSSTV_ERROR_NULL = -2,
    CSSTV_ERROR_INVALID_ARGUMENT = -3,
    CSSTV_ERROR_INVALID_MODE = -4,
    CSSTV_ERROR_UNSUPPORTED_MODE = -5,
    CSSTV_ERROR_INVALID_IMAGE = -6,
    CSSTV_ERROR_INVALID_SAMPLE_RATE = -7,
    CSSTV_ERROR_NOT_INITIALIZED = -8,
    CSSTV_ERROR_NOT_READY = -9

} csstv_status_t;

/* ========================================================================== */
/* Mode                                                                       */
/* ========================================================================== */

/*
 * Mode IDs are intentionally generic.
 *
 * The ID space is divided into families:
 *
 *   0x01xx - PD
 *   0x02xx - reserved
 *   0x03xx - reserved
 *   ...
 *
 * Only the PD family is implemented in version 0.1.
 */

typedef uint16_t csstv_mode_t;

/* -------------------------------------------------------------------------- */
/* PD family                                                                  */
/* -------------------------------------------------------------------------- */

#define CSSTV_MODE_PD50  ((csstv_mode_t)0x0101U)
#define CSSTV_MODE_PD90  ((csstv_mode_t)0x0102U)
#define CSSTV_MODE_PD120 ((csstv_mode_t)0x0103U)
#define CSSTV_MODE_PD160 ((csstv_mode_t)0x0104U)
#define CSSTV_MODE_PD180 ((csstv_mode_t)0x0105U)
#define CSSTV_MODE_PD240 ((csstv_mode_t)0x0106U)
#define CSSTV_MODE_PD290 ((csstv_mode_t)0x0107U)

/* ========================================================================== */
/* Pixel format                                                               */
/* ========================================================================== */

typedef enum
{
    CSSTV_PIXEL_GRAY8 = 0,
    CSSTV_PIXEL_RGB888,
    CSSTV_PIXEL_BGR888,
    CSSTV_PIXEL_RGB565

} csstv_pixel_format_t;

/* ========================================================================== */
/* Image                                                                      */
/* ========================================================================== */

/*
 * Image descriptor.
 *
 * The encoder does not take ownership of the image buffer and does not
 * copy the image.
 *
 * The buffer must remain valid until encoding is finished or the encoder
 * is reset/deinitialized.
 */

typedef struct
{
    const void *data;

    uint16_t width;
    uint16_t height;

    /*
     * Number of bytes between the beginning of consecutive rows.
     */
    size_t stride;

    csstv_pixel_format_t format;

} csstv_image_t;

/* ========================================================================== */
/* Mode information                                                           */
/* ========================================================================== */

typedef struct
{
    csstv_mode_t mode;

    uint16_t width;
    uint16_t height;

    /*
     * Nominal SSTV transmission time in milliseconds.
     */
    uint32_t duration_ms;

} csstv_mode_info_t;

/* ========================================================================== */
/* PCM                                                                        */
/* ========================================================================== */

typedef int16_t csstv_sample_t;

/* ========================================================================== */
/* Encoder                                                                    */
/* ========================================================================== */

#if CSSTV_ENABLE_ENCODER

/*
 * Encoder storage.
 *
 * The encoder is statically allocated by the application.
 *
 * The implementation is hidden inside this storage so that the public
 * header does not expose C++ classes or implementation details.
 *
 * CSSTV_ENCODER_STORAGE_SIZE may be overridden in csstv_config.h.
 */

#ifndef CSSTV_ENCODER_STORAGE_SIZE
#define CSSTV_ENCODER_STORAGE_SIZE 256U
#endif

/*
 * Alignment suitable for the internal implementation.
 */
typedef union
{
    uint64_t u64;
    void *pointer;
    long double long_double;

} csstv_encoder_alignment_t;

/*
 * Opaque encoder object with caller-owned storage.
 *
 * No dynamic memory allocation is required.
 */
typedef struct
{
    csstv_encoder_alignment_t alignment;

    uint8_t storage[CSSTV_ENCODER_STORAGE_SIZE];

} csstv_encoder_t;

/* ========================================================================== */
/* Encoder initialization                                                     */
/* ========================================================================== */

/*
 * Initialize an encoder.
 *
 * mode:
 *   One of the supported CSSTV_MODE_* values.
 *
 * sample_rate:
 *   Output PCM sample rate in Hz.
 *
 * No image data is required during initialization.
 */
csstv_status_t csstv_encoder_init(
    csstv_encoder_t *encoder,
    csstv_mode_t mode,
    uint32_t sample_rate
);

/* ========================================================================== */
/* Image                                                                      */
/* ========================================================================== */

/*
 * Set the image to be encoded.
 *
 * The image is not copied.
 */
csstv_status_t csstv_encoder_set_image(
    csstv_encoder_t *encoder,
    const csstv_image_t *image
);

/* ========================================================================== */
/* Streaming output                                                           */
/* ========================================================================== */

/*
 * Generate PCM samples.
 *
 * samples:
 *   Destination buffer.
 *
 * capacity:
 *   Maximum number of samples that may be written.
 *
 * written:
 *   Number of generated samples.
 *
 * The encoder may return fewer samples than requested.
 *
 * This function is intended for embedded streaming, for example:
 *
 *     int16_t buffer[256];
 *     size_t count;
 *
 *     while (!csstv_encoder_finished(&encoder))
 *     {
 *         csstv_encoder_read(
 *             &encoder,
 *             buffer,
 *             256,
 *             &count
 *         );
 *
 *         audio_write(buffer, count);
 *     }
 */
csstv_status_t csstv_encoder_read(
    csstv_encoder_t *encoder,
    csstv_sample_t *samples,
    size_t capacity,
    size_t *written
);

/* ========================================================================== */
/* Encoder state                                                              */
/* ========================================================================== */

/*
 * Returns true when the complete SSTV transmission has been generated.
 */
bool csstv_encoder_finished(
    const csstv_encoder_t *encoder
);

/*
 * Reset the encoder.
 *
 * The selected mode and sample rate remain unchanged.
 *
 * The image is detached.
 */
csstv_status_t csstv_encoder_reset(
    csstv_encoder_t *encoder
);

/*
 * Deinitialize the encoder.
 *
 * No dynamic memory is required by the public API.
 */
void csstv_encoder_deinit(
    csstv_encoder_t *encoder
);

#endif /* CSSTV_ENABLE_ENCODER */

/* ========================================================================== */
/* Mode information                                                           */
/* ========================================================================== */

/*
 * Check whether a mode is compiled into this build.
 *
 * In version 0.1 this returns true only for enabled PD modes.
 */
bool csstv_mode_supported(
    csstv_mode_t mode
);

/*
 * Get properties of a supported SSTV mode.
 */
csstv_status_t csstv_mode_get_info(
    csstv_mode_t mode,
    csstv_mode_info_t *info
);

/* ========================================================================== */
/* Library information                                                        */
/* ========================================================================== */

/*
 * Return the library version as a string.
 *
 * Example:
 *
 *     "0.1.0"
 */
const char *csstv_version_string(void);

/* ========================================================================== */
/* C++ ABI                                                                    */
/* ========================================================================== */

#ifdef __cplusplus
}
#endif

#endif /* CSSTV_H */
```
