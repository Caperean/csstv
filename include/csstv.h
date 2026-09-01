
#ifndef CSSTV_H
#define CSSTV_H

/*
 * csstv - SSTV encoder library
 *
 * Version 0.01

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



typedef struct
{
    const void *data;

    uint16_t width;
    uint16_t height;

   
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


 // Encoder storage.


#ifndef CSSTV_ENCODER_STORAGE_SIZE
#define CSSTV_ENCODER_STORAGE_SIZE 256U
#endif

typedef union
{
    uint64_t u64;
    void *pointer;
    long double long_double;

} csstv_encoder_alignment_t;


typedef struct
{
    csstv_encoder_alignment_t alignment;

    uint8_t storage[CSSTV_ENCODER_STORAGE_SIZE];

} csstv_encoder_t;

/* ========================================================================== */
/* Encoder initialization                                                     */
/* ========================================================================== */


csstv_status_t csstv_encoder_init(
    csstv_encoder_t *encoder,
    csstv_mode_t mode,
    uint32_t sample_rate
);

/* ========================================================================== */
/* Image                                                                      */
/* ========================================================================== */


csstv_status_t csstv_encoder_set_image(
    csstv_encoder_t *encoder,
    const csstv_image_t *image
);

/* ========================================================================== */
/* Streaming output                                                           */
/* ========================================================================== */

csstv_status_t csstv_encoder_read(
    csstv_encoder_t *encoder,
    csstv_sample_t *samples,
    size_t capacity,
    size_t *written
);

/* ========================================================================== */
/* Encoder state                                                              */
/* ========================================================================== */


bool csstv_encoder_finished(
    const csstv_encoder_t *encoder
);


csstv_status_t csstv_encoder_reset(
    csstv_encoder_t *encoder
);

void csstv_encoder_deinit(
    csstv_encoder_t *encoder
);

#endif /* CSSTV_ENABLE_ENCODER */

/* ========================================================================== */
/* Mode information                                                           */
/* ========================================================================== */


bool csstv_mode_supported(
    csstv_mode_t mode
);


csstv_status_t csstv_mode_get_info(
    csstv_mode_t mode,
    csstv_mode_info_t *info
);

/* ========================================================================== */
/* Library information                                                        */
/* ========================================================================== */


const char *csstv_version_string(void);

/* ========================================================================== */
/* C++ ABI                                                                    */
/* ========================================================================== */

#ifdef __cplusplus
}
#endif

#endif /* CSSTV_H */

