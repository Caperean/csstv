#ifndef CSSTV_TEST_PD_TEST_HELPERS_H
#define CSSTV_TEST_PD_TEST_HELPERS_H

#include "csstv.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace pd_test {

/* SSTV VIS header timing (standard, not PD-specific). */
constexpr double kLeaderMs = 300.0;
constexpr double kBreakMs = 10.0;
constexpr double kVisBitMs = 30.0;
constexpr double kSyncMs = 20.0;
constexpr double kPorchMs = 2.08;

inline size_t ms_to_sample(double cumulative_ms, uint32_t sample_rate)
{
    return static_cast<size_t>(cumulative_ms * 0.001 * static_cast<double>(sample_rate) + 0.5);
}

/* Positive-going zero-crossing frequency estimate. Reliable when the
 * window spans many cycles (tens+); the caller is responsible for
 * only using it on windows long enough for that to hold. */
inline double estimate_frequency(const csstv_sample_t *samples, size_t count, uint32_t sample_rate)
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

inline double frequency_in_window(const std::vector<csstv_sample_t> &pcm, size_t start, size_t end,
                                   uint32_t sample_rate, double trim_fraction = 0.1)
{
    const size_t len = (end > start) ? (end - start) : 0U;
    const size_t trim = static_cast<size_t>(static_cast<double>(len) * trim_fraction);
    const size_t s = start + trim;
    const size_t e = (end >= trim) ? (end - trim) : end;
    if (e <= s || e > pcm.size())
    {
        return -1.0;
    }
    return estimate_frequency(pcm.data() + s, e - s, sample_rate);
}

/* Goertzel power at one target frequency -- accurate even for very
 * short windows (a couple of cycles), unlike zero-crossing counting. */
inline double goertzel_power(const std::vector<csstv_sample_t> &pcm, size_t start, size_t end,
                              uint32_t sample_rate, double target_freq_hz)
{
    const size_t n = (end > start) ? (end - start) : 0U;
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

/* Standard full-range RGB -> YCbCr (same public formula every PD/Scottie/
 * Martin SSTV encoder uses) and the level->frequency mapping. Kept here,
 * independent of the library's own (private, anonymous-namespace) copies,
 * so the test is checking the output against the spec, not against
 * itself. */
inline double expected_freq_for_level(double level_0_255)
{
    const double clamped = std::min(255.0, std::max(0.0, level_0_255));
    return 1500.0 + (clamped / 255.0) * 800.0;
}

struct RGB
{
    double r, g, b;
};

inline double expected_luma(const RGB &p) { return 0.299 * p.r + 0.587 * p.g + 0.114 * p.b; }
inline double expected_cr(const RGB &p) { return 128.0 + 0.5 * p.r - 0.418688 * p.g - 0.081312 * p.b; }
inline double expected_cb(const RGB &p) { return 128.0 - 0.168736 * p.r - 0.331264 * p.g + 0.5 * p.b; }

/* Encodes `image` fully into an in-memory PCM buffer via the public C
 * API, looping read() in `chunk_size`-sample steps until finished(). */
inline bool encode_fully(csstv_mode_t mode, const csstv_image_t &image, uint32_t sample_rate,
                          std::vector<csstv_sample_t> &out, size_t chunk_size = 8192U)
{
    csstv_encoder_t enc{};

    if (csstv_encoder_init(&enc, mode, sample_rate) != CSSTV_OK)
    {
        return false;
    }
    if (csstv_encoder_set_image(&enc, &image) != CSSTV_OK)
    {
        csstv_encoder_deinit(&enc);
        return false;
    }

    out.clear();
    std::vector<csstv_sample_t> chunk(chunk_size);

    while (!csstv_encoder_finished(&enc))
    {
        size_t written = 0U;
        const csstv_status_t st = csstv_encoder_read(&enc, chunk.data(), chunk_size, &written);
        if (st != CSSTV_OK)
        {
            csstv_encoder_deinit(&enc);
            return false;
        }
        out.insert(out.end(), chunk.data(), chunk.data() + written);

        if (written == 0U && !csstv_encoder_finished(&enc))
        {
            csstv_encoder_deinit(&enc); /* would spin forever otherwise */
            return false;
        }
    }

    csstv_encoder_deinit(&enc);
    return true;
}

} /* namespace pd_test */

#endif /* CSSTV_TEST_PD_TEST_HELPERS_H */
