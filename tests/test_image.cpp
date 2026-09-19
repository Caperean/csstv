#include "test_framework.h"

#include "image_reader.h"

#include <cstdint>
#include <vector>

using csstv::ImageReader;
using csstv::RGB8;

namespace {

void test_gray8()
{
    const uint8_t data[4] = {0U, 64U, 200U, 255U};

    csstv_image_t img{};
    img.data = data;
    img.width = 4U;
    img.height = 1U;
    img.stride = 0U; /* tightly packed */
    img.format = CSSTV_PIXEL_GRAY8;

    ImageReader reader(img);

    for (uint16_t x = 0U; x < 4U; ++x)
    {
        const RGB8 p = reader.pixel(x, 0U);
        CHECK(p.r == data[x]);
        CHECK(p.g == data[x]);
        CHECK(p.b == data[x]);
    }
}

void test_rgb888()
{
    /* 2x1 image: pixel0 = (10,20,30), pixel1 = (200,150,100) */
    const uint8_t data[6] = {10U, 20U, 30U, 200U, 150U, 100U};

    csstv_image_t img{};
    img.data = data;
    img.width = 2U;
    img.height = 1U;
    img.stride = 0U;
    img.format = CSSTV_PIXEL_RGB888;

    ImageReader reader(img);

    const RGB8 p0 = reader.pixel(0U, 0U);
    CHECK(p0.r == 10U);
    CHECK(p0.g == 20U);
    CHECK(p0.b == 30U);

    const RGB8 p1 = reader.pixel(1U, 0U);
    CHECK(p1.r == 200U);
    CHECK(p1.g == 150U);
    CHECK(p1.b == 100U);
}

void test_bgr888_channel_swap()
{
    /* Same bytes as test_rgb888, but interpreted as BGR888: channels
     * must come out swapped relative to the RGB888 case above. */
    const uint8_t data[3] = {10U, 20U, 30U}; /* B=10 G=20 R=30 */

    csstv_image_t img{};
    img.data = data;
    img.width = 1U;
    img.height = 1U;
    img.stride = 0U;
    img.format = CSSTV_PIXEL_BGR888;

    ImageReader reader(img);
    const RGB8 p = reader.pixel(0U, 0U);

    CHECK(p.r == 30U);
    CHECK(p.g == 20U);
    CHECK(p.b == 10U);
}

void test_rgb565_extremes_and_midtone()
{
    /* RGB565 packed value helper: bits [15:11]=R5 [10:5]=G6 [4:0]=B5 */
    auto pack565 = [](uint8_t r5, uint8_t g6, uint8_t b5) -> uint16_t {
        return static_cast<uint16_t>((static_cast<uint16_t>(r5) << 11) |
                                      (static_cast<uint16_t>(g6) << 5) |
                                      static_cast<uint16_t>(b5));
    };

    /* Black (0,0,0) -> exactly (0,0,0); white (max,max,max) -> exactly
     * (255,255,255). This is the "replicate high bits" expansion the
     * implementation deliberately uses instead of a plain shift, and
     * it's the part most likely to regress silently, so pin both ends
     * of the range exactly. */
    const uint16_t black = pack565(0U, 0U, 0U);
    const uint16_t white = pack565(31U, 63U, 31U);
    /* Pure red at full 5-bit intensity, no green/blue. */
    const uint16_t red = pack565(31U, 0U, 0U);

    const uint16_t pixels[3] = {black, white, red};

    csstv_image_t img{};
    img.data = pixels;
    img.width = 3U;
    img.height = 1U;
    img.stride = 0U;
    img.format = CSSTV_PIXEL_RGB565;

    ImageReader reader(img);

    const RGB8 p_black = reader.pixel(0U, 0U);
    CHECK(p_black.r == 0U);
    CHECK(p_black.g == 0U);
    CHECK(p_black.b == 0U);

    const RGB8 p_white = reader.pixel(1U, 0U);
    CHECK(p_white.r == 255U);
    CHECK(p_white.g == 255U);
    CHECK(p_white.b == 255U);

    const RGB8 p_red = reader.pixel(2U, 0U);
    CHECK(p_red.r == 255U);
    CHECK(p_red.g == 0U);
    CHECK(p_red.b == 0U);
}

void test_explicit_stride_padding()
{
    /* 2-wide RGB888 image but each row is padded to 8 bytes (2 pixels
     * = 6 bytes + 2 bytes junk). Row 1 must be read starting at byte
     * offset 8, not byte offset 6 -- this is exactly the bug a
     * hard-coded "width * bpp" stride assumption would produce. */
    uint8_t data[16] = {
        /* row 0: pixel(0,0)=(1,2,3) pixel(1,0)=(4,5,6), then 2 pad bytes */
        1U, 2U, 3U, 4U, 5U, 6U, 0xAAU, 0xAAU,
        /* row 1: pixel(0,1)=(7,8,9) pixel(1,1)=(10,11,12), then 2 pad bytes */
        7U, 8U, 9U, 10U, 11U, 12U, 0xAAU, 0xAAU};

    csstv_image_t img{};
    img.data = data;
    img.width = 2U;
    img.height = 2U;
    img.stride = 8U; /* explicit, larger than the tightly-packed 6 */
    img.format = CSSTV_PIXEL_RGB888;

    ImageReader reader(img);

    const RGB8 r0 = reader.pixel(0U, 1U);
    CHECK(r0.r == 7U);
    CHECK(r0.g == 8U);
    CHECK(r0.b == 9U);

    const RGB8 r1 = reader.pixel(1U, 1U);
    CHECK(r1.r == 10U);
    CHECK(r1.g == 11U);
    CHECK(r1.b == 12U);
}

} /* namespace */

void run_image_tests()
{
    RUN_TEST(test_gray8);
    RUN_TEST(test_rgb888);
    RUN_TEST(test_bgr888_channel_swap);
    RUN_TEST(test_rgb565_extremes_and_midtone);
    RUN_TEST(test_explicit_stride_padding);
}
