#include "test_framework.h"

#include "csstv.h"
#include "pd/pd.h" /* for csstv::pd::kPd120Params - independent ground truth */

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

using csstv::pd::kPd120Params;

constexpr uint32_t kSampleRate = 44100U;

/* -------------------------------------------------------------------
 * Reference model of the PD wire format, built independently from the
 * numbers documented in pd.cpp/pd.h (VIS timing is the SSTV standard;
 * PD120's own numbers are its published mode parameters). This is
 * used to locate where in the *generated audio* a given logical
 * segment (a VIS bit, the line sync, a color component) should live,
 * so we can measure its actual frequency and compare it to what the
 * spec requires -- i.e. we are checking the *output signal*, not the
 * driver's internal state.
 * ------------------------------------------------------------------- */

constexpr double kLeaderMs = 300.0;
constexpr double kBreakMs = 10.0;
constexpr double kVisBitMs = 30.0;
constexpr double kSyncMs = 20.0;
constexpr double kPorchMs = 2.08;

size_t ms_to_sample(double cumulative_ms, uint32_t sample_rate)
{
    return static_cast<size_t>(cumulative_ms * 0.001 * static_cast<double>(sample_rate) + 0.5);
}

/* Positive-going zero-crossing frequency estimate over a window. Only
 * meaningful when the window spans many cycles, which every window we
 * use here does (tens to hundreds of cycles). */
double estimate_frequency(const csstv_sample_t *samples, size_t count, uint32_t sample_rate)
{
    if (count < 2U)
    {
        return 0.0;
    }

    size_t crossings = 0U;
    for (size_t i = 1U; i < count; ++i)
    {
        if (samples[i - 1] <= 0 && samples[i] > 0)
        {
            ++crossings;
        }
    }

    const double duration_s = static_cast<double>(count) / static_cast<double>(sample_rate);
    return static_cast<double>(crossings) / duration_s;
}

/* Trims a fraction off both ends of [start,end) to avoid any sample
 * that might sit right on a boundary, then estimates frequency. */
double frequency_in_window(const std::vector<csstv_sample_t> &pcm, size_t start, size_t end,
                            uint32_t sample_rate, double trim_fraction = 0.1)
{
    const size_t len = end - start;
    const size_t trim = static_cast<size_t>(static_cast<double>(len) * trim_fraction);
    const size_t s = start + trim;
    const size_t e = end - trim;
    if (e <= s || e > pcm.size())
    {
        return -1.0;
    }
    return estimate_frequency(pcm.data() + s, e - s, sample_rate);
}

/* Goertzel power at a single target frequency. Unlike zero-crossing
 * counting, this stays accurate even for very short windows (a couple
 * of cycles), which is what the 2.08ms porch segment gives us. We use
 * it to confirm 1500 Hz dominates over the other tones that appear
 * elsewhere in the stream (1200/1900/2300 Hz), rather than trying to
 * read off an exact frequency from so few samples. */
double goertzel_power(const std::vector<csstv_sample_t> &pcm, size_t start, size_t end,
                       uint32_t sample_rate, double target_freq_hz)
{
    const size_t n = end - start;
    if (n == 0U || end > pcm.size())
    {
        return -1.0;
    }

    constexpr double kPi = 3.14159265358979323846;
    const double w = 2.0 * kPi * target_freq_hz / static_cast<double>(sample_rate);
    const double coeff = 2.0 * std::cos(w);
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;

    for (size_t i = 0U; i < n; ++i)
    {
        s0 = static_cast<double>(pcm[start + i]) + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }

    return s1 * s1 + s2 * s2 - coeff * s1 * s2;
}

/* Standard full-range RGB->YCbCr, independently re-derived here (same
 * public formula documented in pd.cpp) as the "expected" side of the
 * comparison, and the same level->frequency mapping. */
double expected_freq_for_level(double level_0_255)
{
    const double clamped = std::min(255.0, std::max(0.0, level_0_255));
    return 1500.0 + (clamped / 255.0) * 800.0;
}

struct RGB
{
    double r, g, b;
};

double expected_luma(const RGB &p) { return 0.299 * p.r + 0.587 * p.g + 0.114 * p.b; }
double expected_cr(const RGB &p) { return 128.0 + 0.5 * p.r - 0.418688 * p.g - 0.081312 * p.b; }
double expected_cb(const RGB &p) { return 128.0 - 0.168736 * p.r - 0.331264 * p.g + 0.5 * p.b; }

/* Encodes `image` fully into an in-memory PCM buffer via the public
 * C API, looping read() until the encoder reports finished(). */
bool encode_fully(const csstv_image_t &image, uint32_t sample_rate, std::vector<csstv_sample_t> &out)
{
    csstv_encoder_t enc{};

    if (csstv_encoder_init(&enc, CSSTV_MODE_PD120, sample_rate) != CSSTV_OK)
    {
        return false;
    }

    if (csstv_encoder_set_image(&enc, &image) != CSSTV_OK)
    {
        csstv_encoder_deinit(&enc);
        return false;
    }

    out.clear();
    csstv_sample_t chunk[4096];

    while (!csstv_encoder_finished(&enc))
    {
        size_t written = 0U;
        const csstv_status_t st = csstv_encoder_read(&enc, chunk, 4096U, &written);
        if (st != CSSTV_OK)
        {
            csstv_encoder_deinit(&enc);
            return false;
        }
        out.insert(out.end(), chunk, chunk + written);

        /* read() is allowed to return 0 samples before finishing, but
         * never allowed to spin forever -- guard the test itself. */
        if (written == 0U && !csstv_encoder_finished(&enc))
        {
            csstv_encoder_deinit(&enc);
            return false;
        }
    }

    csstv_encoder_deinit(&enc);
    return true;
}

/* -------------------------------------------------------------------
 * Tests
 * ------------------------------------------------------------------- */

void test_mode_supported_and_info()
{
    CHECK(csstv_mode_supported(CSSTV_MODE_PD120) == true);

    csstv_mode_info_t info{};
    CHECK(csstv_mode_get_info(CSSTV_MODE_PD120, &info) == CSSTV_OK);
    CHECK(info.width == 640U);
    CHECK(info.height == 496U);

    /* Hand-derived from PD120's published parameters, independent of
     * the implementation's own internal constants:
     * header = 300+10+300+300 = 910ms
     * line_pair = 20 (sync) + 2.08 (porch) + 4*121.6 (Y1,Cr,Cb,Y2) = 508.48ms
     * 248 line pairs (496/2) -> 910 + 248*508.48 = 126,993.04 ms... */
    const double expected_total_ms = 910.0 + 248.0 * (20.0 + 2.08 + 4.0 * 121.6);
    CHECK(std::abs(static_cast<double>(info.duration_ms) - expected_total_ms) < 2.0);
}

void test_rejects_wrong_dimensions()
{
    /* Mirrors the real-world failure mode: the user's actual test.png
     * is 250x200, not PD120's required 640x496. */
    std::vector<uint8_t> pixels(250U * 200U * 3U, 128U);

    csstv_image_t img{};
    img.data = pixels.data();
    img.width = 250U;
    img.height = 200U;
    img.stride = 0U;
    img.format = CSSTV_PIXEL_RGB888;

    csstv_encoder_t enc{};
    CHECK(csstv_encoder_init(&enc, CSSTV_MODE_PD120, kSampleRate) == CSSTV_OK);
    CHECK(csstv_encoder_set_image(&enc, &img) == CSSTV_ERROR_INVALID_IMAGE);
    csstv_encoder_deinit(&enc);
}

void test_rejects_null_and_bad_sample_rate()
{
    csstv_encoder_t enc{};
    CHECK(csstv_encoder_init(nullptr, CSSTV_MODE_PD120, kSampleRate) == CSSTV_ERROR_NULL);
    CHECK(csstv_encoder_init(&enc, CSSTV_MODE_PD120, 0U) == CSSTV_ERROR_INVALID_SAMPLE_RATE);
    CHECK(csstv_encoder_init(&enc, static_cast<csstv_mode_t>(0xFFFFU), kSampleRate) ==
          CSSTV_ERROR_UNSUPPORTED_MODE);
}

/* The core "does it actually work" test: encode a solid-color 640x496
 * image, then decode real properties of the *waveform* -- the VIS
 * code, the sync/porch tones, and the Y/Cr/Cb frequencies -- and check
 * they match what a real PD120 receiver expects. */
void test_full_encode_decodes_correctly()
{
    const RGB color{220.0, 60.0, 30.0}; /* arbitrary, chosen so Y/Cr/Cb differ clearly */

    std::vector<uint8_t> pixels(static_cast<size_t>(640U) * 496U * 3U);
    for (size_t i = 0U; i < pixels.size(); i += 3U)
    {
        pixels[i + 0U] = static_cast<uint8_t>(color.r);
        pixels[i + 1U] = static_cast<uint8_t>(color.g);
        pixels[i + 2U] = static_cast<uint8_t>(color.b);
    }

    csstv_image_t img{};
    img.data = pixels.data();
    img.width = 640U;
    img.height = 496U;
    img.stride = 0U;
    img.format = CSSTV_PIXEL_RGB888;

    std::vector<csstv_sample_t> pcm;
    CHECK(encode_fully(img, kSampleRate, pcm));
    if (pcm.empty())
    {
        std::fprintf(stderr, "  [SKIP] no samples produced, aborting decode checks\n");
        return;
    }

    /* --- Duration sanity: total samples ~= reported duration --- */
    csstv_mode_info_t info{};
    csstv_mode_get_info(CSSTV_MODE_PD120, &info);
    const double actual_ms = 1000.0 * static_cast<double>(pcm.size()) / static_cast<double>(kSampleRate);
    CHECK(std::abs(actual_ms - static_cast<double>(info.duration_ms)) < 5.0);

    /* --- Signal isn't silent/broken --- */
    csstv_sample_t peak = 0;
    for (csstv_sample_t s : pcm)
    {
        peak = std::max(peak, static_cast<csstv_sample_t>(std::abs(static_cast<int>(s))));
    }
    CHECK(peak > 20000); /* kAmplitude is 32000; expect it to actually reach near that */

    /* --- Decode the VIS header directly from the waveform --- */
    double t = 0.0;
    t += kLeaderMs; /* end of leader 1 */
    t += kBreakMs;  /* end of break */
    t += kLeaderMs; /* end of leader 2 */

    const double freq_leader = frequency_in_window(pcm, 0U, ms_to_sample(kLeaderMs, kSampleRate), kSampleRate);
    CHECK(std::abs(freq_leader - 1900.0) < 20.0);

    const size_t break_start = ms_to_sample(kLeaderMs, kSampleRate);
    const size_t break_end = ms_to_sample(kLeaderMs + kBreakMs, kSampleRate);
    const double freq_break = frequency_in_window(pcm, break_start, break_end, kSampleRate, 0.2);
    CHECK(std::abs(freq_break - 1200.0) < 60.0);

    const size_t start_bit_begin = ms_to_sample(t, kSampleRate);
    t += kVisBitMs;
    const size_t start_bit_end = ms_to_sample(t, kSampleRate);
    const double freq_start_bit = frequency_in_window(pcm, start_bit_begin, start_bit_end, kSampleRate);
    CHECK(std::abs(freq_start_bit - 1200.0) < 20.0); /* VIS start bit is always 1200 Hz */

    uint8_t decoded_code = 0U;
    uint8_t ones = 0U;
    for (uint8_t bit = 0U; bit < 7U; ++bit)
    {
        const size_t s = ms_to_sample(t, kSampleRate);
        t += kVisBitMs;
        const size_t e = ms_to_sample(t, kSampleRate);
        const double f = frequency_in_window(pcm, s, e, kSampleRate);

        /* 1300 Hz = '0', 1100 Hz = '1'; midpoint 1200 splits them. */
        const bool one = f < 1200.0;
        if (one)
        {
            decoded_code |= static_cast<uint8_t>(1U << bit);
            ++ones;
        }
    }

    const size_t parity_begin = ms_to_sample(t, kSampleRate);
    t += kVisBitMs;
    const size_t parity_end = ms_to_sample(t, kSampleRate);
    const double freq_parity = frequency_in_window(pcm, parity_begin, parity_end, kSampleRate);
    const bool parity_bit_one = freq_parity < 1200.0;
    const bool expected_parity_one = (ones % 2U) != 0U;

    CHECK(decoded_code == kPd120Params.vis_code);
    CHECK(parity_bit_one == expected_parity_one);

    const size_t stop_begin = ms_to_sample(t, kSampleRate);
    t += kVisBitMs;
    const size_t stop_end = ms_to_sample(t, kSampleRate);
    const double freq_stop = frequency_in_window(pcm, stop_begin, stop_end, kSampleRate);
    CHECK(std::abs(freq_stop - 1200.0) < 20.0);

    /* --- First line: sync, porch, then Y1/Cr/Cb/Y2 for our color --- */
    const size_t sync_begin = ms_to_sample(t, kSampleRate);
    t += kSyncMs;
    const size_t sync_end = ms_to_sample(t, kSampleRate);
    const double freq_sync = frequency_in_window(pcm, sync_begin, sync_end, kSampleRate);
    CHECK(std::abs(freq_sync - 1200.0) < 20.0);

    const size_t porch_begin = ms_to_sample(t, kSampleRate);
    t += kPorchMs;
    const size_t porch_end = ms_to_sample(t, kSampleRate);
    /* Porch window is only ~92 samples at 44.1kHz -- too short for a
     * reliable zero-crossing frequency read, so instead confirm 1500Hz
     * dominates over the other tones the stream uses elsewhere. */
    const double p1200 = goertzel_power(pcm, porch_begin, porch_end, kSampleRate, 1200.0);
    const double p1500 = goertzel_power(pcm, porch_begin, porch_end, kSampleRate, 1500.0);
    const double p1900 = goertzel_power(pcm, porch_begin, porch_end, kSampleRate, 1900.0);
    const double p2300 = goertzel_power(pcm, porch_begin, porch_end, kSampleRate, 2300.0);
    std::printf("  porch Goertzel power: 1200=%.3e 1500=%.3e 1900=%.3e 2300=%.3e\n", p1200, p1500,
                 p1900, p2300);
    CHECK(p1500 > p1200 && p1500 > p1900 && p1500 > p2300);

    const size_t y1_begin = ms_to_sample(t, kSampleRate);
    t += kPd120Params.color_scan_ms;
    const size_t y1_end = ms_to_sample(t, kSampleRate);
    const double freq_y1 = frequency_in_window(pcm, y1_begin, y1_end, kSampleRate);
    const double expected_y_freq = expected_freq_for_level(expected_luma(color));
    CHECK(std::abs(freq_y1 - expected_y_freq) < 15.0);

    const size_t cr_begin = ms_to_sample(t, kSampleRate);
    t += kPd120Params.color_scan_ms;
    const size_t cr_end = ms_to_sample(t, kSampleRate);
    const double freq_cr = frequency_in_window(pcm, cr_begin, cr_end, kSampleRate);
    const double expected_cr_freq = expected_freq_for_level(expected_cr(color));
    CHECK(std::abs(freq_cr - expected_cr_freq) < 15.0);

    const size_t cb_begin = ms_to_sample(t, kSampleRate);
    t += kPd120Params.color_scan_ms;
    const size_t cb_end = ms_to_sample(t, kSampleRate);
    const double freq_cb = frequency_in_window(pcm, cb_begin, cb_end, kSampleRate);
    const double expected_cb_freq = expected_freq_for_level(expected_cb(color));
    CHECK(std::abs(freq_cb - expected_cb_freq) < 15.0);

    const size_t y2_begin = ms_to_sample(t, kSampleRate);
    t += kPd120Params.color_scan_ms;
    const size_t y2_end = ms_to_sample(t, kSampleRate);
    const double freq_y2 = frequency_in_window(pcm, y2_begin, y2_end, kSampleRate);
    CHECK(std::abs(freq_y2 - expected_y_freq) < 15.0); /* uniform image: Y2 == Y1 */

    std::printf("  decoded VIS code = %u (expected %u)\n", decoded_code, kPd120Params.vis_code);
    std::printf("  Y=%.1fHz (exp %.1f)  Cr=%.1fHz (exp %.1f)  Cb=%.1fHz (exp %.1f)\n", freq_y1,
                 expected_y_freq, freq_cr, expected_cr_freq, freq_cb, expected_cb_freq);
}

/* reset() + re-read must reproduce the exact same PCM (phase is
 * explicitly zeroed), which matters for reproducible test fixtures
 * and for any caller relying on deterministic output. */
void test_reset_is_deterministic()
{
    std::vector<uint8_t> pixels(static_cast<size_t>(640U) * 496U * 3U, 77U);

    csstv_image_t img{};
    img.data = pixels.data();
    img.width = 640U;
    img.height = 496U;
    img.stride = 0U;
    img.format = CSSTV_PIXEL_RGB888;

    csstv_encoder_t enc{};
    CHECK(csstv_encoder_init(&enc, CSSTV_MODE_PD120, 8000U) == CSSTV_OK); /* low rate: keep this test fast */
    CHECK(csstv_encoder_set_image(&enc, &img) == CSSTV_OK);

    std::vector<csstv_sample_t> first_pass;
    csstv_sample_t chunk[4096];
    while (!csstv_encoder_finished(&enc))
    {
        size_t written = 0U;
        csstv_encoder_read(&enc, chunk, 4096U, &written);
        first_pass.insert(first_pass.end(), chunk, chunk + written);
        if (written == 0U && !csstv_encoder_finished(&enc))
        {
            break;
        }
    }

    CHECK(csstv_encoder_reset(&enc) == CSSTV_OK);

    std::vector<csstv_sample_t> second_pass;
    while (!csstv_encoder_finished(&enc))
    {
        size_t written = 0U;
        csstv_encoder_read(&enc, chunk, 4096U, &written);
        second_pass.insert(second_pass.end(), chunk, chunk + written);
        if (written == 0U && !csstv_encoder_finished(&enc))
        {
            break;
        }
    }

    csstv_encoder_deinit(&enc);

    CHECK(first_pass.size() == second_pass.size());
    CHECK(first_pass == second_pass);
}

} /* namespace */

void run_modes_tests()
{
    RUN_TEST(test_mode_supported_and_info);
    RUN_TEST(test_rejects_wrong_dimensions);
    RUN_TEST(test_rejects_null_and_bad_sample_rate);
    RUN_TEST(test_full_encode_decodes_correctly);
    RUN_TEST(test_reset_is_deterministic);
}
