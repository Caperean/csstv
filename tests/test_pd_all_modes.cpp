#include "test_framework.h"
#include "pd_test_helpers.h"

#include "csstv.h"
#include "pd/pd.h"

#include <cstdio>
#include <vector>

using pd_test::RGB;

namespace {

struct ModeCase
{
    csstv_mode_t mode;
    const csstv::pd::ModeParams *params;
    const char *name;
};

/* Every mode this build has enabled (see CMakeLists.txt). If a mode is
 * later disabled, its entry here should be removed too -- there's no
 * way to reference a params struct that doesn't exist in this build. */
const ModeCase kAllModes[] = {
    {CSSTV_MODE_PD50, &csstv::pd::kPd50Params, "PD50"},
    {CSSTV_MODE_PD90, &csstv::pd::kPd90Params, "PD90"},
    {CSSTV_MODE_PD120, &csstv::pd::kPd120Params, "PD120"},
    {CSSTV_MODE_PD160, &csstv::pd::kPd160Params, "PD160"},
    {CSSTV_MODE_PD180, &csstv::pd::kPd180Params, "PD180"},
    {CSSTV_MODE_PD240, &csstv::pd::kPd240Params, "PD240"},
    {CSSTV_MODE_PD290, &csstv::pd::kPd290Params, "PD290"},
};

constexpr uint32_t kSampleRate = 44100U;

/* Full encode-then-decode check for one mode: builds a solid-color
 * image of the mode's exact required dimensions, encodes it, then
 * reads the VIS code and the Y/Cr/Cb tones back out of the actual PCM
 * and compares them to the spec. This is the same technique used for
 * the PD120 deep-dive, generalized over every mode's own dimensions,
 * color_scan_ms and vis_code -- a mode-specific timing bug (e.g. an
 * off-by-one in a mode's line-pair count) would show up here even
 * though it wouldn't in a PD120-only test. */
void verify_mode(const ModeCase &mc)
{
    std::printf(" -- %s (mode 0x%04X, %ux%u, vis=%u) --\n", mc.name,
                static_cast<unsigned>(mc.mode), mc.params->width, mc.params->height,
                mc.params->vis_code);

    const RGB color{220.0, 60.0, 30.0};

    std::vector<uint8_t> pixels(static_cast<size_t>(mc.params->width) * mc.params->height * 3U);
    for (size_t i = 0U; i < pixels.size(); i += 3U)
    {
        pixels[i + 0U] = static_cast<uint8_t>(color.r);
        pixels[i + 1U] = static_cast<uint8_t>(color.g);
        pixels[i + 2U] = static_cast<uint8_t>(color.b);
    }

    csstv_image_t img{};
    img.data = pixels.data();
    img.width = mc.params->width;
    img.height = mc.params->height;
    img.stride = 0U;
    img.format = CSSTV_PIXEL_RGB888;

    /* mode_get_info must agree with the mode's own published params. */
    csstv_mode_info_t info{};
    CHECK(csstv_mode_get_info(mc.mode, &info) == CSSTV_OK);
    CHECK(info.width == mc.params->width);
    CHECK(info.height == mc.params->height);

    const double expected_line_pair_ms =
        pd_test::kSyncMs + pd_test::kPorchMs + 4.0 * mc.params->color_scan_ms;
    const uint32_t expected_line_pairs = static_cast<uint32_t>(mc.params->height) / 2U;
    const double expected_header_ms = 910.0; /* 300+10+300+300, fixed for every PD mode */
    const double expected_total_ms =
        expected_header_ms + static_cast<double>(expected_line_pairs) * expected_line_pair_ms;
    CHECK(std::abs(static_cast<double>(info.duration_ms) - expected_total_ms) < 2.0);

    std::vector<csstv_sample_t> pcm;
    const bool encoded_ok = pd_test::encode_fully(mc.mode, img, kSampleRate, pcm);
    CHECK(encoded_ok);
    if (!encoded_ok)
    {
        return;
    }

    const double actual_ms = 1000.0 * static_cast<double>(pcm.size()) / static_cast<double>(kSampleRate);
    CHECK(std::abs(actual_ms - static_cast<double>(info.duration_ms)) < 5.0);

    /* --- Decode the VIS header from the real waveform --- */
    double t = 0.0;
    t += pd_test::kLeaderMs;
    t += pd_test::kBreakMs;
    t += pd_test::kLeaderMs;

    const size_t start_bit_begin = pd_test::ms_to_sample(t, kSampleRate);
    t += pd_test::kVisBitMs;
    const size_t start_bit_end = pd_test::ms_to_sample(t, kSampleRate);
    const double freq_start_bit =
        pd_test::frequency_in_window(pcm, start_bit_begin, start_bit_end, kSampleRate);
    CHECK(std::abs(freq_start_bit - 1200.0) < 20.0);

    uint8_t decoded_code = 0U;
    uint8_t ones = 0U;
    for (uint8_t bit = 0U; bit < 7U; ++bit)
    {
        const size_t s = pd_test::ms_to_sample(t, kSampleRate);
        t += pd_test::kVisBitMs;
        const size_t e = pd_test::ms_to_sample(t, kSampleRate);
        const double f = pd_test::frequency_in_window(pcm, s, e, kSampleRate);
        const bool one = f < 1200.0;
        if (one)
        {
            decoded_code |= static_cast<uint8_t>(1U << bit);
            ++ones;
        }
    }
    CHECK(decoded_code == mc.params->vis_code);

    const size_t parity_begin = pd_test::ms_to_sample(t, kSampleRate);
    t += pd_test::kVisBitMs;
    const size_t parity_end = pd_test::ms_to_sample(t, kSampleRate);
    const double freq_parity = pd_test::frequency_in_window(pcm, parity_begin, parity_end, kSampleRate);
    const bool parity_bit_one = freq_parity < 1200.0;
    CHECK(parity_bit_one == ((ones % 2U) != 0U));

    /* --- First line: sync + Y1/Cr/Cb/Y2 for our test color --- */
    t += pd_test::kVisBitMs; /* stop bit */

    const size_t sync_begin = pd_test::ms_to_sample(t, kSampleRate);
    t += pd_test::kSyncMs;
    const size_t sync_end = pd_test::ms_to_sample(t, kSampleRate);
    const double freq_sync = pd_test::frequency_in_window(pcm, sync_begin, sync_end, kSampleRate);
    CHECK(std::abs(freq_sync - 1200.0) < 20.0);

    t += pd_test::kPorchMs; /* porch: too short to measure reliably at every mode's rate, skip */

    const double expected_y_freq = pd_test::expected_freq_for_level(pd_test::expected_luma(color));
    const double expected_cr_freq = pd_test::expected_freq_for_level(pd_test::expected_cr(color));
    const double expected_cb_freq = pd_test::expected_freq_for_level(pd_test::expected_cb(color));

    const size_t y1_begin = pd_test::ms_to_sample(t, kSampleRate);
    t += mc.params->color_scan_ms;
    const size_t y1_end = pd_test::ms_to_sample(t, kSampleRate);
    const double freq_y1 = pd_test::frequency_in_window(pcm, y1_begin, y1_end, kSampleRate);
    CHECK(std::abs(freq_y1 - expected_y_freq) < 15.0);

    const size_t cr_begin = pd_test::ms_to_sample(t, kSampleRate);
    t += mc.params->color_scan_ms;
    const size_t cr_end = pd_test::ms_to_sample(t, kSampleRate);
    const double freq_cr = pd_test::frequency_in_window(pcm, cr_begin, cr_end, kSampleRate);
    CHECK(std::abs(freq_cr - expected_cr_freq) < 15.0);

    const size_t cb_begin = pd_test::ms_to_sample(t, kSampleRate);
    t += mc.params->color_scan_ms;
    const size_t cb_end = pd_test::ms_to_sample(t, kSampleRate);
    const double freq_cb = pd_test::frequency_in_window(pcm, cb_begin, cb_end, kSampleRate);
    CHECK(std::abs(freq_cb - expected_cb_freq) < 15.0);

    const size_t y2_begin = pd_test::ms_to_sample(t, kSampleRate);
    t += mc.params->color_scan_ms;
    const size_t y2_end = pd_test::ms_to_sample(t, kSampleRate);
    const double freq_y2 = pd_test::frequency_in_window(pcm, y2_begin, y2_end, kSampleRate);
    CHECK(std::abs(freq_y2 - expected_y_freq) < 15.0);

    std::printf("    VIS=%u/%u  Y=%.1f/%.1f  Cr=%.1f/%.1f  Cb=%.1f/%.1f  dur=%.0fms/%ums\n",
                decoded_code, mc.params->vis_code, freq_y1, expected_y_freq, freq_cr,
                expected_cr_freq, freq_cb, expected_cb_freq, actual_ms, info.duration_ms);
}

void test_all_pd_modes()
{
    for (const ModeCase &mc : kAllModes)
    {
        verify_mode(mc);
    }
}

} /* namespace */

void run_pd_all_modes_tests()
{
    RUN_TEST(test_all_pd_modes);
}
