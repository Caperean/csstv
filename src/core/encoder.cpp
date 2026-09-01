#include "csstv.h"

#include "csstv_internal.h"
#include "csstv_mode_driver.h"

#include <cstring>

using csstv::EncoderStage;
using csstv::EncoderState;
using csstv::ModeDriver;

namespace csstv {

EncoderState *encoder_state(csstv_encoder_t *encoder)
{
    return reinterpret_cast<EncoderState *>(encoder->storage);
}

const EncoderState *encoder_state(const csstv_encoder_t *encoder)
{
    return reinterpret_cast<const EncoderState *>(encoder->storage);
}

} /* namespace csstv */

namespace {

/*
 * Basic, format-aware validation of a caller-supplied image. This
 * does not know about any particular mode's required dimensions --
 * that check happens in csstv_encoder_set_image() once the mode's
 * csstv_mode_info_t has been looked up.
 */
bool image_is_valid(const csstv_image_t *image)
{
    if (image == NULL)
    {
        return false;
    }

    if (image->data == NULL)
    {
        return false;
    }

    if (image->width == 0U || image->height == 0U)
    {
        return false;
    }

    size_t bytes_per_pixel;

    switch (image->format)
    {
        case CSSTV_PIXEL_GRAY8:
            bytes_per_pixel = 1U;
            break;

        case CSSTV_PIXEL_RGB888:
        case CSSTV_PIXEL_BGR888:
            bytes_per_pixel = 3U;
            break;

        case CSSTV_PIXEL_RGB565:
            bytes_per_pixel = 2U;
            break;

        default:
            return false;
    }

    const size_t min_stride = static_cast<size_t>(image->width) * bytes_per_pixel;

    /* stride == 0 means "tightly packed"; anything else must be able
     * to hold at least one full row. */
    if (image->stride != 0U && image->stride < min_stride)
    {
        return false;
    }

    return true;
}

} /* namespace */

#if CSSTV_ENABLE_ENCODER

extern "C" {

csstv_status_t csstv_encoder_init(
    csstv_encoder_t *encoder,
    csstv_mode_t mode,
    uint32_t sample_rate)
{
    if (encoder == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    if (sample_rate == 0U)
    {
        return CSSTV_ERROR_INVALID_SAMPLE_RATE;
    }

    if (!csstv::mode_supported(mode))
    {
        return CSSTV_ERROR_UNSUPPORTED_MODE;
    }

    EncoderState *state = csstv::encoder_state(encoder);

    std::memset(state, 0, sizeof(EncoderState));

    state->mode = mode;
    state->sample_rate = sample_rate;
    state->has_image = false;
    state->stage = EncoderStage::kReady;

    state->driver = csstv::create_mode_driver(
        mode,
        state->driver_state,
        EncoderState::kDriverStateSize);

    if (state->driver == NULL)
    {
        state->stage = EncoderStage::kUninitialized;
        return CSSTV_ERROR_UNSUPPORTED_MODE;
    }

    return CSSTV_OK;
}

csstv_status_t csstv_encoder_set_image(
    csstv_encoder_t *encoder,
    const csstv_image_t *image)
{
    if (encoder == NULL || image == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    EncoderState *state = csstv::encoder_state(encoder);

    if (state->stage == EncoderStage::kUninitialized || state->driver == NULL)
    {
        return CSSTV_ERROR_NOT_INITIALIZED;
    }

    if (!image_is_valid(image))
    {
        return CSSTV_ERROR_INVALID_IMAGE;
    }

    csstv_mode_info_t info;
    const csstv_status_t info_status = csstv::mode_get_info(state->mode, &info);

    if (info_status != CSSTV_OK)
    {
        return info_status;
    }

    if (image->width != info.width || image->height != info.height)
    {
        return CSSTV_ERROR_INVALID_IMAGE;
    }

    const csstv_status_t begin_status = state->driver->begin(*image, state->sample_rate);

    if (begin_status != CSSTV_OK)
    {
        state->has_image = false;
        return begin_status;
    }

    state->image = *image;
    state->has_image = true;
    state->stage = EncoderStage::kEncoding;

    return CSSTV_OK;
}

csstv_status_t csstv_encoder_read(
    csstv_encoder_t *encoder,
    csstv_sample_t *samples,
    size_t capacity,
    size_t *written)
{
    if (written != NULL)
    {
        *written = 0U;
    }

    if (encoder == NULL || samples == NULL || written == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    EncoderState *state = csstv::encoder_state(encoder);

    if (state->stage == EncoderStage::kUninitialized || state->driver == NULL)
    {
        return CSSTV_ERROR_NOT_INITIALIZED;
    }

    if (!state->has_image)
    {
        return CSSTV_ERROR_NOT_READY;
    }

    if (capacity == 0U || state->stage == EncoderStage::kFinished)
    {
        return CSSTV_OK;
    }

    const csstv_status_t read_status = state->driver->read(samples, capacity, written);

    if (read_status != CSSTV_OK)
    {
        return read_status;
    }

    if (state->driver->finished())
    {
        state->stage = EncoderStage::kFinished;
    }

    return CSSTV_OK;
}

bool csstv_encoder_finished(const csstv_encoder_t *encoder)
{
    if (encoder == NULL)
    {
        return true;
    }

    const EncoderState *state = csstv::encoder_state(encoder);

    if (state->stage == EncoderStage::kUninitialized)
    {
        return true;
    }

    return state->stage == EncoderStage::kFinished;
}

csstv_status_t csstv_encoder_reset(csstv_encoder_t *encoder)
{
    if (encoder == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    EncoderState *state = csstv::encoder_state(encoder);

    if (state->stage == EncoderStage::kUninitialized || state->driver == NULL)
    {
        return CSSTV_ERROR_NOT_INITIALIZED;
    }

    if (!state->has_image)
    {
        return CSSTV_ERROR_NOT_READY;
    }

    const csstv_status_t reset_status = state->driver->reset();

    if (reset_status != CSSTV_OK)
    {
        return reset_status;
    }

    state->stage = EncoderStage::kEncoding;

    return CSSTV_OK;
}

void csstv_encoder_deinit(csstv_encoder_t *encoder)
{
    if (encoder == NULL)
    {
        return;
    }

    EncoderState *state = csstv::encoder_state(encoder);

    if (state->stage == EncoderStage::kUninitialized)
    {
        return;
    }

    if (state->driver != NULL)
    {
        state->driver->~ModeDriver();
    }

    std::memset(state, 0, sizeof(EncoderState));
}

} /* extern "C" */

#endif /* CSSTV_ENABLE_ENCODER */
