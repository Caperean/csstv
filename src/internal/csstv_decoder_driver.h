#ifndef CSSTV_DECODER_DRIVER_H
#define CSSTV_DECODER_DRIVER_H

#include "csstv.h"

#include <stddef.h>

namespace csstv {

/*
 * Per-mode sample decoder. One concrete subclass exists per mode
 * (see src/modes/pd), placement-constructed into a small fixed
 * buffer owned by DecoderState -- no heap allocation.
 *
 * Usage from the decoder core:
 *
 *   init()       once, when the decoder is initialized with a mode
 *   set_image()  once per frame, providing the caller-owned output buffer
 *   write()      repeatedly, feeding samples in caller-sized chunks
 *   finished()   to know when decoding has completed
 *   get_image()  to retrieve a descriptor for the filled output buffer
 */
class DecoderDriver
{
public:
    virtual ~DecoderDriver() = default;

    /*
     * Initialize the decoder for a specific mode and sample rate.
     * Returns CSSTV_ERROR_UNSUPPORTED_MODE if the mode is not supported.
     */
    virtual csstv_status_t init(csstv_mode_t mode, uint32_t sample_rate) = 0;

    /*
     * Bind a caller-owned output image. Must be called before write().
     * Only CSSTV_PIXEL_RGB888 is accepted; dimensions must match the mode.
     */
    virtual csstv_status_t set_image(const csstv_image_t &image) = 0;

    /*
     * Write up to `count` samples from `in`, advancing internal
     * state. `*consumed` is set to the number of samples actually
     * processed, which may be less than `count` (including zero)
     * even when not yet finished.
     */
    virtual csstv_status_t write(const csstv_sample_t *in, size_t count, size_t *consumed) = 0;

    /* True once all samples have been processed and image is ready. */
    virtual bool finished() const = 0;

    /*
     * Fill `image` with a descriptor for the decoded buffer bound via
     * set_image(). Must only be called after finished() returns true.
     */
    virtual csstv_status_t get_image(csstv_image_t *image) = 0;

    /* Reset the decoder to re-decode into the current output buffer. */
    virtual csstv_status_t reset() = 0;
};

/*
 * Placement-construct a DecoderDriver for `mode` into `storage`
 * (at least `storage_size` bytes, suitably aligned). Returns nullptr
 * if `mode` is unsupported or its driver does not fit in `storage`.
 * Ownership is not implied -- the caller manages the object's
 * lifetime and must call its destructor explicitly.
 */
DecoderDriver *create_decoder_driver(csstv_mode_t mode, void *storage, size_t storage_size);

} /* namespace csstv */

#endif /* CSSTV_DECODER_DRIVER_H */