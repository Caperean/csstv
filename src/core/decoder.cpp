#include "csstv.h"

#if CSSTV_ENABLE_DECODER

#include "csstv_internal.h"
#include "csstv_decoder_driver.h"

#include <string.h>

using csstv::DecoderStage;
using csstv::DecoderState;
using csstv::DecoderDriver;

namespace csstv {

DecoderState *decoder_state(csstv_decoder_t *decoder)
{
    return reinterpret_cast<DecoderState *>(decoder->storage);
}

const DecoderState *decoder_state(const csstv_decoder_t *decoder)
{
    return reinterpret_cast<const DecoderState *>(decoder->storage);
}

} /* namespace csstv */

namespace {

bool image_is_valid_rgb888(const csstv_image_t *image)
{
    if (image == NULL || image->data == NULL)
    {
        return false;
    }

    if (image->width == 0U || image->height == 0U)
    {
        return false;
    }

    if (image->format != CSSTV_PIXEL_RGB888)
    {
        return false;
    }

    const size_t min_stride = static_cast<size_t>(image->width) * 3U;
    if (image->stride != 0U && image->stride < min_stride)
    {
        return false;
    }

    return true;
}

} /* namespace */

extern "C" {

csstv_status_t csstv_decoder_init(
    csstv_decoder_t *decoder,
    csstv_mode_t mode,
    uint32_t sample_rate)
{
    if (decoder == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    if (sample_rate == 0U)
    {
        return CSSTV_ERROR_INVALID_SAMPLE_RATE;
    }

    if (!csstv::decoder_mode_supported(mode))
    {
        return CSSTV_ERROR_UNSUPPORTED_MODE;
    }

    DecoderState *state = csstv::decoder_state(decoder);

    memset(state, 0, sizeof(DecoderState));

    state->mode = mode;
    state->sample_rate = sample_rate;
    state->has_image = false;
    state->stage = DecoderStage::kReady;

    state->driver = csstv::create_decoder_driver(
        mode,
        state->driver_state,
        DecoderState::kDriverStateSize);

    if (state->driver == NULL)
    {
        state->stage = DecoderStage::kUninitialized;
        return CSSTV_ERROR_UNSUPPORTED_MODE;
    }

    const csstv_status_t init_status = state->driver->init(mode, sample_rate);

    if (init_status != CSSTV_OK)
    {
        state->driver->~DecoderDriver();
        state->driver = NULL;
        state->stage = DecoderStage::kUninitialized;
        return init_status;
    }

    return CSSTV_OK;
}

csstv_status_t csstv_decoder_set_image(
    csstv_decoder_t *decoder,
    const csstv_image_t *image)
{
    if (decoder == NULL || image == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    DecoderState *state = csstv::decoder_state(decoder);

    if (state->stage == DecoderStage::kUninitialized || state->driver == NULL)
    {
        return CSSTV_ERROR_NOT_INITIALIZED;
    }

    if (!image_is_valid_rgb888(image))
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

    const csstv_status_t set_status = state->driver->set_image(*image);

    if (set_status != CSSTV_OK)
    {
        state->has_image = false;
        return set_status;
    }

    state->image = *image;
    state->has_image = true;
    state->stage = DecoderStage::kDecoding;

    return CSSTV_OK;
}

csstv_status_t csstv_decoder_write(
    csstv_decoder_t *decoder,
    const csstv_sample_t *samples,
    size_t count,
    size_t *consumed)
{
    if (consumed != NULL)
    {
        *consumed = 0U;
    }

    if (decoder == NULL || samples == NULL || consumed == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    DecoderState *state = csstv::decoder_state(decoder);

    if (state->stage == DecoderStage::kUninitialized || state->driver == NULL)
    {
        return CSSTV_ERROR_NOT_INITIALIZED;
    }

    if (!state->has_image)
    {
        return CSSTV_ERROR_NOT_READY;
    }

    if (state->stage == DecoderStage::kFinished)
    {
        return CSSTV_OK;
    }

    state->stage = DecoderStage::kDecoding;

    const csstv_status_t write_status = state->driver->write(samples, count, consumed);

    if (write_status != CSSTV_OK)
    {
        return write_status;
    }

    if (state->driver->finished())
    {
        state->stage = DecoderStage::kFinished;
    }

    return CSSTV_OK;
}

bool csstv_decoder_finished(const csstv_decoder_t *decoder)
{
    if (decoder == NULL)
    {
        return true;
    }

    const DecoderState *state = csstv::decoder_state(decoder);

    if (state->stage == DecoderStage::kUninitialized)
    {
        return true;
    }

    return state->stage == DecoderStage::kFinished;
}

csstv_status_t csstv_decoder_get_image(
    csstv_decoder_t *decoder,
    csstv_image_t *image)
{
    if (decoder == NULL || image == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    DecoderState *state = csstv::decoder_state(decoder);

    if (state->stage == DecoderStage::kUninitialized || state->driver == NULL)
    {
        return CSSTV_ERROR_NOT_INITIALIZED;
    }

    if (!state->driver->finished())
    {
        return CSSTV_ERROR_NOT_READY;
    }

    return state->driver->get_image(image);
}

csstv_status_t csstv_decoder_reset(csstv_decoder_t *decoder)
{
    if (decoder == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    DecoderState *state = csstv::decoder_state(decoder);

    if (state->stage == DecoderStage::kUninitialized || state->driver == NULL)
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

    state->stage = DecoderStage::kDecoding;

    return CSSTV_OK;
}

void csstv_decoder_deinit(csstv_decoder_t *decoder)
{
    if (decoder == NULL)
    {
        return;
    }

    DecoderState *state = csstv::decoder_state(decoder);

    if (state->stage == DecoderStage::kUninitialized)
    {
        return;
    }

    if (state->driver != NULL)
    {
        state->driver->~DecoderDriver();
    }

    memset(state, 0, sizeof(DecoderState));
}

} /* extern "C" */

#endif /* CSSTV_ENABLE_DECODER */