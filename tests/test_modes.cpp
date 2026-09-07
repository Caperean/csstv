#include "csstv.h"
#include "test_common.h"

#include <cassert>
#include <cstdint>
#include <vector>

void test_encoder()
{
    constexpr uint32_t sample_rate = 8000U;

    assert(csstv_encoder_init(nullptr, CSSTV_MODE_PD120, sample_rate) == CSSTV_ERROR_NULL);

    csstv_encoder_t encoder{};
    assert(csstv_encoder_init(&encoder, CSSTV_MODE_PD120, 0U) == CSSTV_ERROR_INVALID_SAMPLE_RATE);
    assert(csstv_encoder_init(&encoder, static_cast<csstv_mode_t>(0xFFFFU), sample_rate) == CSSTV_ERROR_UNSUPPORTED_MODE);
    assert(csstv_encoder_init(&encoder, CSSTV_MODE_PD120, sample_rate) == CSSTV_OK);
    assert(!csstv_encoder_finished(&encoder));

    csstv_sample_t samples[32]{};
    size_t written = 123U;
    assert(csstv_encoder_read(&encoder, samples, 32, &written) == CSSTV_ERROR_NOT_READY);
    assert(written == 0U);
    assert(csstv_encoder_reset(&encoder) == CSSTV_ERROR_NOT_READY);

    std::vector<uint8_t> pixels(640U * 496U, 128U);
    csstv_image_t image{};
    image.data = pixels.data();
    image.width = 640;
    image.height = 496;
    image.stride = 0;
    image.format = CSSTV_PIXEL_GRAY8;

    csstv_image_t bad_image = image;
    bad_image.width = 639;
    assert(csstv_encoder_set_image(&encoder, &bad_image) == CSSTV_ERROR_INVALID_IMAGE);

    bad_image = image;
    bad_image.data = nullptr;
    assert(csstv_encoder_set_image(&encoder, &bad_image) == CSSTV_ERROR_INVALID_IMAGE);

    assert(csstv_encoder_set_image(&encoder, &image) == CSSTV_OK);

    written = 0U;
    assert(csstv_encoder_read(&encoder, samples, 32, &written) == CSSTV_OK);
    assert(written == 32U);

    bool any_nonzero = false;
    for (size_t i = 0; i < written; ++i)
        any_nonzero = any_nonzero || samples[i] != 0;
    assert(any_nonzero);

    assert(csstv_encoder_reset(&encoder) == CSSTV_OK);

    csstv_sample_t after_reset[32]{};
    size_t reset_written = 0U;
    assert(csstv_encoder_read(&encoder, after_reset, 32, &reset_written) == CSSTV_OK);
    assert(reset_written == written);

    for (size_t i = 0; i < reset_written; ++i)
        assert(after_reset[i] == samples[i]);

    csstv_encoder_deinit(&encoder);
}
