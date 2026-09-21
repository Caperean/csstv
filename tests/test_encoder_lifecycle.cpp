#include "test_framework.h"
#include "pd_test_helpers.h"

#include "csstv.h"
#include "csstv_internal.h" /* to sanity-check the driver fits its storage */
#include "pd/pd.h"

#include <cstdio>
#include <vector>

using pd_test::RGB;

namespace {

/* csstv::pd::Driver is placement-constructed into a fixed
 * EncoderState::kDriverStateSize buffer; pd::create() only checks
 * this at *runtime*, so a future field added to Driver could silently
 * turn every PD encoder init into CSSTV_ERROR_UNSUPPORTED_MODE
 * without failing the build. Catch that here at compile time instead. */
static_assert(sizeof(csstv::pd::Driver) <= csstv::EncoderState::kDriverStateSize,
              "csstv::pd::Driver no longer fits in EncoderState::driver_state -- "
              "increase EncoderState::kDriverStateSize in csstv_internal.h");

void test_read_before_set_image_is_not_ready()
{
    csstv_encoder_t enc{};
    CHECK(csstv_encoder_init(&enc, CSSTV_MODE_PD50, 8000U) == CSSTV_OK);

    csstv_sample_t buf[16];
    size_t written = 123U; /* poison, to check it gets reset to 0 */
    CHECK(csstv_encoder_read(&enc, buf, 16U, &written) == CSSTV_ERROR_NOT_READY);
    CHECK(written == 0U);

    csstv_encoder_deinit(&enc);
}

void test_reset_before_set_image_is_not_ready()
{
    csstv_encoder_t enc{};
    CHECK(csstv_encoder_init(&enc, CSSTV_MODE_PD50, 8000U) == CSSTV_OK);
    CHECK(csstv_encoder_reset(&enc) == CSSTV_ERROR_NOT_READY);
    csstv_encoder_deinit(&enc);
}

void test_zero_capacity_read_is_noop()
{
    std::vector<uint8_t> pixels(static_cast<size_t>(320U) * 256U * 3U, 100U);
    csstv_image_t img{};
    img.data = pixels.data();
    img.width = 320U;
    img.height = 256U;
    img.format = CSSTV_PIXEL_RGB888;

    csstv_encoder_t enc{};
    CHECK(csstv_encoder_init(&enc, CSSTV_MODE_PD50, 8000U) == CSSTV_OK);
    CHECK(csstv_encoder_set_image(&enc, &img) == CSSTV_OK);

    csstv_sample_t dummy_buf[1];
    size_t written = 999U;
    CHECK(csstv_encoder_read(&enc, dummy_buf, 0U, &written) == CSSTV_OK);
    CHECK(written == 0U);
    CHECK(csstv_encoder_finished(&enc) == false); /* zero-capacity read must not fake completion */

    csstv_encoder_deinit(&enc);
}

/* A null sample buffer must always be rejected, even with capacity 0
 * -- capacity is not a license to skip the null check. */
void test_null_sample_buffer_is_rejected_even_at_zero_capacity()
{
    std::vector<uint8_t> pixels(static_cast<size_t>(320U) * 256U * 3U, 100U);
    csstv_image_t img{};
    img.data = pixels.data();
    img.width = 320U;
    img.height = 256U;
    img.format = CSSTV_PIXEL_RGB888;

    csstv_encoder_t enc{};
    CHECK(csstv_encoder_init(&enc, CSSTV_MODE_PD50, 8000U) == CSSTV_OK);
    CHECK(csstv_encoder_set_image(&enc, &img) == CSSTV_OK);

    size_t written = 999U;
    CHECK(csstv_encoder_read(&enc, nullptr, 0U, &written) == CSSTV_ERROR_NULL);
    CHECK(written == 0U); /* *written is still reset to 0 before the null check runs */

    csstv_encoder_deinit(&enc);
}

void test_read_after_finished_is_harmless()
{
    std::vector<uint8_t> pixels(static_cast<size_t>(320U) * 256U * 3U, 100U);
    csstv_image_t img{};
    img.data = pixels.data();
    img.width = 320U;
    img.height = 256U;
    img.format = CSSTV_PIXEL_RGB888;

    csstv_encoder_t enc{};
    CHECK(csstv_encoder_init(&enc, CSSTV_MODE_PD50, 8000U) == CSSTV_OK);
    CHECK(csstv_encoder_set_image(&enc, &img) == CSSTV_OK);

    std::vector<csstv_sample_t> pcm;
    csstv_sample_t chunk[4096];
    while (!csstv_encoder_finished(&enc))
    {
        size_t written = 0U;
        csstv_encoder_read(&enc, chunk, 4096U, &written);
        pcm.insert(pcm.end(), chunk, chunk + written);
    }
    CHECK(csstv_encoder_finished(&enc) == true);

    /* Calling read() again after finished must stay well-behaved. */
    size_t written = 111U;
    CHECK(csstv_encoder_read(&enc, chunk, 4096U, &written) == CSSTV_OK);
    CHECK(written == 0U);

    csstv_encoder_deinit(&enc);
}

void test_double_deinit_is_safe()
{
    csstv_encoder_t enc{};
    CHECK(csstv_encoder_init(&enc, CSSTV_MODE_PD50, 8000U) == CSSTV_OK);
    csstv_encoder_deinit(&enc);
    csstv_encoder_deinit(&enc); /* must not crash or double-free */
    CHECK(csstv_encoder_finished(&enc) == true);
}

void test_finished_on_null_encoder()
{
    CHECK(csstv_encoder_finished(nullptr) == true);
}

/* Re-using one encoder instance for two different images back-to-back
 * (a realistic pattern for e.g. a live slow-scan camera feed) must not
 * leak any state -- phase, pixel cursor, VIS bits -- from the first
 * image into the second. */
void test_reuse_encoder_for_second_image()
{
    std::vector<uint8_t> black(static_cast<size_t>(320U) * 256U * 3U, 0U);
    std::vector<uint8_t> white(static_cast<size_t>(320U) * 256U * 3U, 255U);

    csstv_image_t img_black{};
    img_black.data = black.data();
    img_black.width = 320U;
    img_black.height = 256U;
    img_black.format = CSSTV_PIXEL_RGB888;

    csstv_image_t img_white{};
    img_white.data = white.data();
    img_white.width = 320U;
    img_white.height = 256U;
    img_white.format = CSSTV_PIXEL_RGB888;

    csstv_encoder_t enc{};
    CHECK(csstv_encoder_init(&enc, CSSTV_MODE_PD50, 44100U) == CSSTV_OK);

    CHECK(csstv_encoder_set_image(&enc, &img_black) == CSSTV_OK);
    std::vector<csstv_sample_t> pcm1;
    csstv_sample_t chunk[8192];
    while (!csstv_encoder_finished(&enc))
    {
        size_t written = 0U;
        csstv_encoder_read(&enc, chunk, 8192U, &written);
        pcm1.insert(pcm1.end(), chunk, chunk + written);
    }

    /* Re-arm with a *different* image without destroying the encoder. */
    CHECK(csstv_encoder_set_image(&enc, &img_white) == CSSTV_OK);
    std::vector<csstv_sample_t> pcm2;
    while (!csstv_encoder_finished(&enc))
    {
        size_t written = 0U;
        csstv_encoder_read(&enc, chunk, 8192U, &written);
        pcm2.insert(pcm2.end(), chunk, chunk + written);
    }

    csstv_encoder_deinit(&enc);

    CHECK(!pcm1.empty());
    CHECK(!pcm2.empty());
    /* Same mode -> same total length both times. */
    CHECK(pcm1.size() == pcm2.size());

    /* Measure the first line's Y1 tone in each run and confirm it
     * reflects *that run's* image, not a leftover from the other one. */
    const double header_ms = 910.0;
    const double sync_porch_ms = 20.0 + 2.08;
    const double y1_start_ms = header_ms + sync_porch_ms;
    const double y1_end_ms = y1_start_ms + csstv::pd::kPd50Params.color_scan_ms;

    const size_t s = pd_test::ms_to_sample(y1_start_ms, 44100U);
    const size_t e = pd_test::ms_to_sample(y1_end_ms, 44100U);

    const double freq1 = pd_test::frequency_in_window(pcm1, s, e, 44100U);
    const double freq2 = pd_test::frequency_in_window(pcm2, s, e, 44100U);

    CHECK(std::abs(freq1 - 1500.0) < 15.0); /* first run: black */
    CHECK(std::abs(freq2 - 2300.0) < 15.0); /* second run: white, not contaminated by black */
}

/* GRAY8 exercises a different bytes-per-pixel/stride path through
 * ImageReader than the RGB888 tests elsewhere, and has a notable
 * property worth pinning down: for r==g==b, the Cr/Cb formulas'
 * coefficients sum to exactly zero, so a grayscale pixel must always
 * produce *exactly* neutral chroma (level 128), regardless of how
 * bright or dark it is. */
void test_gray8_full_pipeline_neutral_chroma()
{
    const uint8_t gray_level = 90U;
    std::vector<uint8_t> pixels(static_cast<size_t>(320U) * 256U, gray_level);

    csstv_image_t img{};
    img.data = pixels.data();
    img.width = 320U;
    img.height = 256U;
    img.stride = 0U;
    img.format = CSSTV_PIXEL_GRAY8;

    std::vector<csstv_sample_t> pcm;
    CHECK(pd_test::encode_fully(CSSTV_MODE_PD50, img, 44100U, pcm));
    if (pcm.empty())
    {
        return;
    }

    const double header_ms = 910.0;
    const double sync_porch_ms = 20.0 + 2.08;
    double t = header_ms + sync_porch_ms;

    const size_t y1_s = pd_test::ms_to_sample(t, 44100U);
    t += csstv::pd::kPd50Params.color_scan_ms;
    const size_t y1_e = pd_test::ms_to_sample(t, 44100U);

    const size_t cr_s = pd_test::ms_to_sample(t, 44100U);
    t += csstv::pd::kPd50Params.color_scan_ms;
    const size_t cr_e = pd_test::ms_to_sample(t, 44100U);

    const size_t cb_s = pd_test::ms_to_sample(t, 44100U);
    t += csstv::pd::kPd50Params.color_scan_ms;
    const size_t cb_e = pd_test::ms_to_sample(t, 44100U);

    const double freq_y = pd_test::frequency_in_window(pcm, y1_s, y1_e, 44100U);
    const double freq_cr = pd_test::frequency_in_window(pcm, cr_s, cr_e, 44100U);
    const double freq_cb = pd_test::frequency_in_window(pcm, cb_s, cb_e, 44100U);

    const double expected_y = pd_test::expected_freq_for_level(gray_level);
    const double expected_neutral = pd_test::expected_freq_for_level(128.0); /* == 1901.57 Hz */

    CHECK(std::abs(freq_y - expected_y) < 15.0);
    CHECK(std::abs(freq_cr - expected_neutral) < 15.0);
    CHECK(std::abs(freq_cb - expected_neutral) < 15.0);
}

/* A padded-stride image must be read correctly all the way through a
 * real encode, not just by ImageReader in isolation. */
void test_custom_stride_full_pipeline()
{
    const size_t width = 320U;
    const size_t height = 256U;
    const size_t tight_row_bytes = width * 3U;
    const size_t padded_row_bytes = tight_row_bytes + 64U; /* arbitrary padding */

    std::vector<uint8_t> pixels(padded_row_bytes * height, 0U);
    /* Fill every real pixel with the same known color; leave padding
     * as zero so a stride bug (reading into the padding) would shift
     * every subsequent pixel and corrupt the measured tone. */
    for (size_t y = 0U; y < height; ++y)
    {
        uint8_t *row = pixels.data() + y * padded_row_bytes;
        for (size_t x = 0U; x < width; ++x)
        {
            row[x * 3U + 0U] = 180U;
            row[x * 3U + 1U] = 40U;
            row[x * 3U + 2U] = 200U;
        }
    }

    csstv_image_t img{};
    img.data = pixels.data();
    img.width = static_cast<uint16_t>(width);
    img.height = static_cast<uint16_t>(height);
    img.stride = padded_row_bytes;
    img.format = CSSTV_PIXEL_RGB888;

    std::vector<csstv_sample_t> pcm;
    CHECK(pd_test::encode_fully(CSSTV_MODE_PD50, img, 44100U, pcm));
    if (pcm.empty())
    {
        return;
    }

    const double header_ms = 910.0;
    const double sync_porch_ms = 20.0 + 2.08;
    const double y1_start_ms = header_ms + sync_porch_ms;
    const double y1_end_ms = y1_start_ms + csstv::pd::kPd50Params.color_scan_ms;

    const size_t s = pd_test::ms_to_sample(y1_start_ms, 44100U);
    const size_t e = pd_test::ms_to_sample(y1_end_ms, 44100U);
    const double freq_y1 = pd_test::frequency_in_window(pcm, s, e, 44100U);

    const RGB color{180.0, 40.0, 200.0};
    const double expected_y = pd_test::expected_freq_for_level(pd_test::expected_luma(color));
    CHECK(std::abs(freq_y1 - expected_y) < 15.0);
}

/* Reading in pathologically tiny chunks (1 sample at a time) must
 * still produce the exact same total length -- and the exact same
 * bytes -- as one large read loop. This is the strongest test of the
 * "resume mid-tone across calls" contract described in pd.h, since it
 * forces the state machine to pause and resume inside nearly every
 * single tone segment, not just at chunk-sized intervals. */
void test_tiny_buffer_resumption_matches_bulk_read()
{
    std::vector<uint8_t> pixels(static_cast<size_t>(320U) * 256U * 3U, 60U);
    csstv_image_t img{};
    img.data = pixels.data();
    img.width = 320U;
    img.height = 256U;
    img.format = CSSTV_PIXEL_RGB888;

    const uint32_t sample_rate = 8000U; /* keep the 1-sample-at-a-time loop fast */

    std::vector<csstv_sample_t> bulk;
    CHECK(pd_test::encode_fully(CSSTV_MODE_PD50, img, sample_rate, bulk, 8192U));

    std::vector<csstv_sample_t> tiny;
    CHECK(pd_test::encode_fully(CSSTV_MODE_PD50, img, sample_rate, tiny, 1U));

    CHECK(!bulk.empty());
    CHECK(bulk.size() == tiny.size());
    CHECK(bulk == tiny);
}

} /* namespace */

void run_encoder_lifecycle_tests()
{
    RUN_TEST(test_read_before_set_image_is_not_ready);
    RUN_TEST(test_reset_before_set_image_is_not_ready);
    RUN_TEST(test_zero_capacity_read_is_noop);
    RUN_TEST(test_null_sample_buffer_is_rejected_even_at_zero_capacity);
    RUN_TEST(test_read_after_finished_is_harmless);
    RUN_TEST(test_double_deinit_is_safe);
    RUN_TEST(test_finished_on_null_encoder);
    RUN_TEST(test_reuse_encoder_for_second_image);
    RUN_TEST(test_gray8_full_pipeline_neutral_chroma);
    RUN_TEST(test_custom_stride_full_pipeline);
    RUN_TEST(test_tiny_buffer_resumption_matches_bulk_read);
}
