#include "test_framework.h"
#include "pd_test_helpers.h"

#include "csstv.h"
#include "csstv_internal.h"
#include "pd/pd.h"
#include "pd/pd_decoder.h"

#include <cmath>
#include <cstdint>
#include <vector>

using pd_test::RGB;

namespace {

static_assert(sizeof(csstv::pd::DecoderDriver) <= csstv::DecoderState::kDriverStateSize,
              "csstv::pd::DecoderDriver no longer fits in DecoderState::driver_state -- "
              "increase DecoderState::kDriverStateSize in csstv_internal.h");

bool decode_fully(csstv_mode_t mode, const std::vector<csstv_sample_t> &pcm, uint32_t sample_rate,
                  std::vector<uint8_t> &out_rgb, size_t chunk_size = 8192U)
{
    csstv_mode_info_t info{};
    if (csstv_mode_get_info(mode, &info) != CSSTV_OK)
    {
        return false;
    }

    out_rgb.assign(static_cast<size_t>(info.width) * static_cast<size_t>(info.height) * 3U, 0U);

    csstv_image_t img{};
    img.data = out_rgb.data();
    img.width = info.width;
    img.height = info.height;
    img.format = CSSTV_PIXEL_RGB888;

    csstv_decoder_t dec{};
    if (csstv_decoder_init(&dec, mode, sample_rate) != CSSTV_OK)
    {
        return false;
    }
    if (csstv_decoder_set_image(&dec, &img) != CSSTV_OK)
    {
        csstv_decoder_deinit(&dec);
        return false;
    }

    size_t offset = 0U;
    while (offset < pcm.size() && !csstv_decoder_finished(&dec))
    {
        const size_t n = std::min(chunk_size, pcm.size() - offset);
        size_t consumed = 0U;
        const csstv_status_t st = csstv_decoder_write(&dec, pcm.data() + offset, n, &consumed);
        if (st != CSSTV_OK)
        {
            csstv_decoder_deinit(&dec);
            return false;
        }
        if (consumed == 0U && !csstv_decoder_finished(&dec))
        {
            csstv_decoder_deinit(&dec);
            return false;
        }
        offset += consumed;
    }

    const bool ok = csstv_decoder_finished(&dec);
    csstv_decoder_deinit(&dec);
    return ok;
}

void fill_solid_rgb(std::vector<uint8_t> &pixels, uint16_t width, uint16_t height, uint8_t r, uint8_t g,
                    uint8_t b)
{
    pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 3U);
    for (size_t i = 0U; i < pixels.size(); i += 3U)
    {
        pixels[i] = r;
        pixels[i + 1U] = g;
        pixels[i + 2U] = b;
    }
}

double mean_abs_err(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b)
{
    if (a.size() != b.size() || a.empty())
    {
        return 1.0e9;
    }
    double sum = 0.0;
    for (size_t i = 0U; i < a.size(); ++i)
    {
        sum += std::fabs(static_cast<double>(a[i]) - static_cast<double>(b[i]));
    }
    return sum / static_cast<double>(a.size());
}

void test_write_before_set_image_is_not_ready()
{
    csstv_decoder_t dec{};
    CHECK(csstv_decoder_init(&dec, CSSTV_MODE_PD50, 44100U) == CSSTV_OK);

    csstv_sample_t buf[16]{};
    size_t consumed = 123U;
    CHECK(csstv_decoder_write(&dec, buf, 16U, &consumed) == CSSTV_ERROR_NOT_READY);
    CHECK(consumed == 0U);

    csstv_decoder_deinit(&dec);
}

void test_get_image_before_finished_is_not_ready()
{
    std::vector<uint8_t> pixels;
    fill_solid_rgb(pixels, 320U, 256U, 128U, 128U, 128U);

    csstv_image_t img{};
    img.data = pixels.data();
    img.width = 320U;
    img.height = 256U;
    img.format = CSSTV_PIXEL_RGB888;

    csstv_decoder_t dec{};
    CHECK(csstv_decoder_init(&dec, CSSTV_MODE_PD50, 44100U) == CSSTV_OK);
    CHECK(csstv_decoder_set_image(&dec, &img) == CSSTV_OK);

    csstv_image_t out{};
    CHECK(csstv_decoder_get_image(&dec, &out) == CSSTV_ERROR_NOT_READY);

    csstv_decoder_deinit(&dec);
}

void test_double_deinit_is_safe()
{
    csstv_decoder_t dec{};
    CHECK(csstv_decoder_init(&dec, CSSTV_MODE_PD50, 44100U) == CSSTV_OK);
    csstv_decoder_deinit(&dec);
    csstv_decoder_deinit(&dec);
    CHECK(csstv_decoder_finished(&dec) == true);
}

void test_roundtrip_solid_gray_pd50()
{
    const uint8_t level = 90U;
    std::vector<uint8_t> src;
    fill_solid_rgb(src, 320U, 256U, level, level, level);

    csstv_image_t img{};
    img.data = src.data();
    img.width = 320U;
    img.height = 256U;
    img.format = CSSTV_PIXEL_RGB888;

    std::vector<csstv_sample_t> pcm;
    CHECK(pd_test::encode_fully(CSSTV_MODE_PD50, img, 44100U, pcm));
    CHECK(!pcm.empty());

    std::vector<uint8_t> decoded;
    CHECK(decode_fully(CSSTV_MODE_PD50, pcm, 44100U, decoded));
    CHECK(decoded.size() == src.size());

    /* Chroma-subsampled round-trip of a neutral gray should land close. */
    const double mae = mean_abs_err(src, decoded);
    CHECK(mae < 12.0);
}

void test_roundtrip_solid_color_pd50()
{
    const uint8_t r = 180U;
    const uint8_t g = 40U;
    const uint8_t b = 200U;
    std::vector<uint8_t> src;
    fill_solid_rgb(src, 320U, 256U, r, g, b);

    csstv_image_t img{};
    img.data = src.data();
    img.width = 320U;
    img.height = 256U;
    img.format = CSSTV_PIXEL_RGB888;

    std::vector<csstv_sample_t> pcm;
    CHECK(pd_test::encode_fully(CSSTV_MODE_PD50, img, 44100U, pcm));

    std::vector<uint8_t> decoded;
    CHECK(decode_fully(CSSTV_MODE_PD50, pcm, 44100U, decoded));
    CHECK(decoded.size() == src.size());

    const double mae = mean_abs_err(src, decoded);
    CHECK(mae < 20.0);
}

void test_tiny_write_chunks_match_bulk()
{
    std::vector<uint8_t> src;
    fill_solid_rgb(src, 320U, 256U, 60U, 60U, 60U);

    csstv_image_t img{};
    img.data = src.data();
    img.width = 320U;
    img.height = 256U;
    img.format = CSSTV_PIXEL_RGB888;

    std::vector<csstv_sample_t> pcm;
    CHECK(pd_test::encode_fully(CSSTV_MODE_PD50, img, 44100U, pcm));

    std::vector<uint8_t> bulk;
    std::vector<uint8_t> tiny;
    CHECK(decode_fully(CSSTV_MODE_PD50, pcm, 44100U, bulk, 8192U));
    CHECK(decode_fully(CSSTV_MODE_PD50, pcm, 44100U, tiny, 1U));
    CHECK(bulk == tiny);
}

void test_reset_redecodes()
{
    std::vector<uint8_t> src;
    fill_solid_rgb(src, 320U, 256U, 100U, 100U, 100U);

    csstv_image_t img{};
    img.data = src.data();
    img.width = 320U;
    img.height = 256U;
    img.format = CSSTV_PIXEL_RGB888;

    std::vector<csstv_sample_t> pcm;
    CHECK(pd_test::encode_fully(CSSTV_MODE_PD50, img, 44100U, pcm));

    std::vector<uint8_t> out(src.size(), 0U);
    csstv_image_t out_img{};
    out_img.data = out.data();
    out_img.width = 320U;
    out_img.height = 256U;
    out_img.format = CSSTV_PIXEL_RGB888;

    csstv_decoder_t dec{};
    CHECK(csstv_decoder_init(&dec, CSSTV_MODE_PD50, 44100U) == CSSTV_OK);
    CHECK(csstv_decoder_set_image(&dec, &out_img) == CSSTV_OK);

    size_t offset = 0U;
    while (offset < pcm.size() && !csstv_decoder_finished(&dec))
    {
        size_t consumed = 0U;
        CHECK(csstv_decoder_write(&dec, pcm.data() + offset, pcm.size() - offset, &consumed) == CSSTV_OK);
        offset += consumed;
    }
    CHECK(csstv_decoder_finished(&dec) == true);

    const double mae1 = mean_abs_err(src, out);

    CHECK(csstv_decoder_reset(&dec) == CSSTV_OK);
    CHECK(csstv_decoder_finished(&dec) == false);

    offset = 0U;
    while (offset < pcm.size() && !csstv_decoder_finished(&dec))
    {
        size_t consumed = 0U;
        CHECK(csstv_decoder_write(&dec, pcm.data() + offset, pcm.size() - offset, &consumed) == CSSTV_OK);
        offset += consumed;
    }
    CHECK(csstv_decoder_finished(&dec) == true);

    const double mae2 = mean_abs_err(src, out);
    CHECK(mae1 < 12.0);
    CHECK(mae2 < 12.0);

    csstv_decoder_deinit(&dec);
}

} /* namespace */

void run_decoder_lifecycle_tests()
{
    RUN_TEST(test_write_before_set_image_is_not_ready);
    RUN_TEST(test_get_image_before_finished_is_not_ready);
    RUN_TEST(test_double_deinit_is_safe);
    RUN_TEST(test_roundtrip_solid_gray_pd50);
    RUN_TEST(test_roundtrip_solid_color_pd50);
    RUN_TEST(test_tiny_write_chunks_match_bulk);
    RUN_TEST(test_reset_redecodes);
}
