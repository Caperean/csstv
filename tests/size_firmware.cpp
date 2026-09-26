/*
 * Minimal firmware used by the RAM-usage and binary-size CI jobs.
 *
 * Instantiates csstv::pd::Driver with synthetic parameters (no
 * pd_modes.cpp) and runs a short encode so the linker retains the
 * encoder path under measurement.
 */

#include "pd/pd.h"

#include "csstv_freestanding_runtime_support.h"

#if !CSSTV_NO_CXX_STDLIB
#include <new>
#endif

#include <stddef.h>
#include <stdint.h>

namespace {

constexpr uint32_t kSampleRate = 8000U;
constexpr uint16_t kWidth = 8U;
constexpr uint16_t kHeight = 4U;
constexpr double kColorScanMs = 10.0;

alignas(csstv::pd::Driver) unsigned char g_driver_storage[sizeof(csstv::pd::Driver)];

uint8_t g_pixels[static_cast<size_t>(kWidth) * static_cast<size_t>(kHeight) * 3U];
csstv_sample_t g_chunk[64];

volatile csstv_sample_t g_sink;

} /* namespace */

int main()
{
    for (size_t i = 0U; i < sizeof(g_pixels); ++i)
    {
        g_pixels[i] = static_cast<uint8_t>(i);
    }

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

    if (driver->begin(image, kSampleRate) != CSSTV_OK)
    {
        driver->~Driver();
        return 1;
    }

    while (!driver->finished())
    {
        size_t written = 0U;
        if (driver->read(g_chunk, sizeof(g_chunk) / sizeof(g_chunk[0]), &written) != CSSTV_OK)
        {
            driver->~Driver();
            return 1;
        }
        if (written == 0U)
        {
            break;
        }
        for (size_t i = 0U; i < written; ++i)
        {
            g_sink = g_chunk[i];
        }
    }

    driver->~Driver();
    return 0;
}
