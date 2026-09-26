/*
 * Determinism test for the PD encoder's cumulative sample-rounding.
 *
 * Re-encoding the same image via reset() must produce byte-for-byte
 * identical output to the first pass. Uses synthetic ModeParams with
 * deliberately non-round timing so fractional sample remainders are
 * exercised (see ideal_elapsed_samples_ / rounded_elapsed_samples_
 * in pd.h).
 */

#include "pd/pd.h"

#include <cstdio>
#include <cstring>
#include <new>

namespace {

constexpr uint32_t kSampleRate = 8000U;
constexpr uint16_t kWidth = 16U;
constexpr uint16_t kHeight = 8U; /* even: PD transmits line pairs */

/* Non-round color scan so per-pixel durations leave a fractional sample. */
constexpr double kColorScanMs = 12.61;

alignas(csstv::pd::Driver) unsigned char g_driver_storage[sizeof(csstv::pd::Driver)];

uint8_t g_pixels[static_cast<size_t>(kWidth) * static_cast<size_t>(kHeight) * 3U];

csstv_sample_t g_pass_a[32768];
csstv_sample_t g_pass_b[32768];

void fill_test_image()
{
    for (uint16_t y = 0U; y < kHeight; ++y)
    {
        for (uint16_t x = 0U; x < kWidth; ++x)
        {
            const size_t i = (static_cast<size_t>(y) * kWidth + x) * 3U;
            g_pixels[i + 0U] = static_cast<uint8_t>(x * 7U + y * 3U);
            g_pixels[i + 1U] = static_cast<uint8_t>(x * 5U + y * 11U);
            g_pixels[i + 2U] = static_cast<uint8_t>(x * 13U + y * 2U);
        }
    }
}

size_t encode_all(csstv::pd::Driver &driver, csstv_sample_t *out, size_t capacity)
{
    size_t total = 0U;
    while (!driver.finished())
    {
        if (total >= capacity)
        {
            std::fprintf(stderr, "output buffer exhausted\n");
            return 0U;
        }

        size_t written = 0U;
        const csstv_status_t st =
            driver.read(out + total, capacity - total, &written);
        if (st != CSSTV_OK)
        {
            std::fprintf(stderr, "read failed: %d\n", static_cast<int>(st));
            return 0U;
        }
        if (written == 0U)
        {
            break;
        }
        total += written;
    }
    return total;
}

} /* namespace */

int main()
{
    fill_test_image();

    const csstv::pd::ModeParams params = {
        CSSTV_MODE_PD50,
        kWidth,
        kHeight,
        kColorScanMs,
        0x5DU,
    };

    csstv_image_t image{};
    image.data = g_pixels;
    image.width = kWidth;
    image.height = kHeight;
    image.stride = 0U;
    image.format = CSSTV_PIXEL_RGB888;

    auto *driver = new (g_driver_storage) csstv::pd::Driver(params);

    csstv_status_t st = driver->begin(image, kSampleRate);
    if (st != CSSTV_OK)
    {
        std::fprintf(stderr, "begin failed: %d\n", static_cast<int>(st));
        driver->~Driver();
        return 1;
    }

    const size_t n_a = encode_all(*driver, g_pass_a, sizeof(g_pass_a) / sizeof(g_pass_a[0]));
    if (n_a == 0U)
    {
        driver->~Driver();
        return 1;
    }

    st = driver->reset();
    if (st != CSSTV_OK)
    {
        std::fprintf(stderr, "reset failed: %d\n", static_cast<int>(st));
        driver->~Driver();
        return 1;
    }

    const size_t n_b = encode_all(*driver, g_pass_b, sizeof(g_pass_b) / sizeof(g_pass_b[0]));
    if (n_b == 0U)
    {
        driver->~Driver();
        return 1;
    }

    driver->~Driver();

    if (n_a != n_b)
    {
        std::fprintf(stderr, "length mismatch: %zu vs %zu\n", n_a, n_b);
        return 1;
    }

    if (std::memcmp(g_pass_a, g_pass_b, n_a * sizeof(csstv_sample_t)) != 0)
    {
        std::fprintf(stderr, "sample mismatch after reset() (%zu samples)\n", n_a);
        return 1;
    }

    std::printf("determinism ok: %zu identical samples across reset()\n", n_a);
    return 0;
}
