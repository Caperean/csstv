/*
 * Host-side harness for the CSSTV_NO_CXX_STDLIB placement-new path.
 *
 * Built twice by CI: once with the real <new>, once with
 * CSSTV_NO_CXX_STDLIB=1 + freestanding runtime support. Both binaries
 * must emit identical sample streams on stdout.
 */

#include "pd/pd.h"

#include "csstv_freestanding_runtime_support.h"

#if !CSSTV_NO_CXX_STDLIB
#include <new>
#endif

#include <cstdio>
#include <stddef.h>
#include <stdint.h>

namespace {

constexpr uint32_t kSampleRate = 8000U;
constexpr uint16_t kWidth = 16U;
constexpr uint16_t kHeight = 8U;
constexpr double kColorScanMs = 12.61; /* non-round at common rates */

alignas(csstv::pd::Driver) unsigned char g_driver_storage[sizeof(csstv::pd::Driver)];

uint8_t g_pixels[static_cast<size_t>(kWidth) * static_cast<size_t>(kHeight) * 3U];
csstv_sample_t g_chunk[512];

void fill_test_image()
{
    for (size_t i = 0U; i < sizeof(g_pixels); ++i)
    {
        g_pixels[i] = static_cast<uint8_t>(i * 17U + 3U);
    }
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
        if (std::fwrite(g_chunk, sizeof(csstv_sample_t), written, stdout) != written)
        {
            driver->~Driver();
            return 1;
        }
    }

    driver->~Driver();
    return 0;
}
