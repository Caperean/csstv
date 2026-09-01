#ifndef CSSTV_MODE_DRIVER_H
#define CSSTV_MODE_DRIVER_H

#include "csstv.h"

#include <cstddef>

namespace csstv {

/*
 * Per-mode sample generator. One concrete subclass exists per mode
 * (see src/modes/pd), placement-constructed into a small fixed
 * buffer owned by EncoderState -- no heap allocation.
 *
 * Usage from the encoder core:
 *
 *   begin()  once, when an image is bound
 *   read()   repeatedly, pulling samples in caller-sized chunks
 *   finished() to know when read() has no more samples to give
 *   reset()  to re-run the same image from the start
 */
class ModeDriver
{
public:
    virtual ~ModeDriver() = default;

    /*
     * Bind an image and (re)start generation from the first sample.
     * `image` is guaranteed by the caller to match the mode's
     * required width/height and to have already passed basic
     * validation (non-null data, supported pixel format, adequate
     * stride).
     */
    virtual csstv_status_t begin(const csstv_image_t &image, uint32_t sample_rate) = 0;

    /*
     * Write up to `capacity` samples into `out`, advancing internal
     * state. `*written` is set to the number of samples actually
     * produced, which may be less than `capacity` (including zero)
     * even when not yet finished -- callers must loop until
     * finished() is true. Must not be called before begin().
     */
    virtual csstv_status_t read(csstv_sample_t *out, size_t capacity, size_t *written) = 0;

    /* True once all samples for the current image have been read. */
    virtual bool finished() const = 0;

    /* Rewind to the start of the currently bound image. */
    virtual csstv_status_t reset() = 0;
};

/* True if `mode` is both a recognized mode and enabled in this build. */
bool mode_supported(csstv_mode_t mode);

/* Fill `info` for `mode`. Returns CSSTV_ERROR_UNSUPPORTED_MODE if not supported. */
csstv_status_t mode_get_info(csstv_mode_t mode, csstv_mode_info_t *info);

/*
 * Placement-construct a ModeDriver for `mode` into `storage`
 * (at least `storage_size` bytes, suitably aligned). Returns nullptr
 * if `mode` is unsupported or its driver does not fit in `storage`.
 * Ownership is not implied -- the caller manages the object's
 * lifetime and must call its destructor explicitly.
 */
ModeDriver *create_mode_driver(csstv_mode_t mode, void *storage, size_t storage_size);

} /* namespace csstv */

#endif /* CSSTV_MODE_DRIVER_H */
