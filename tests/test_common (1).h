#include "image_reader.h"
#include "test_common.h"

#include <cassert>
#include <cstdint>

void test_image_reader()
{
    {
        const uint8_t data[] = {0x2A};
        const csstv_image_t image{data, 1, 1, 0, CSSTV_PIXEL_GRAY8};
        const csstv::RGB8 p = csstv::ImageReader(image).pixel(0, 0);
        assert(p.r == 0x2A && p.g == 0x2A && p.b == 0x2A);
    }

    {
        const uint8_t data[] = {10, 20, 30};
        const csstv_image_t image{data, 1, 1, 0, CSSTV_PIXEL_RGB888};
        const csstv::RGB8 p = csstv::ImageReader(image).pixel(0, 0);
        assert(p.r == 10 && p.g == 20 && p.b == 30);
    }

    {
        const uint8_t data[] = {30, 20, 10};
        const csstv_image_t image{data, 1, 1, 0, CSSTV_PIXEL_BGR888};
        const csstv::RGB8 p = csstv::ImageReader(image).pixel(0, 0);
        assert(p.r == 10 && p.g == 20 && p.b == 30);
    }

    {
        const uint16_t red565 = 0xF800U;
        const csstv_image_t image{&red565, 1, 1, 0, CSSTV_PIXEL_RGB565};
        const csstv::RGB8 p = csstv::ImageReader(image).pixel(0, 0);
        assert(p.r == 255 && p.g == 0 && p.b == 0);
    }

    {
        const uint8_t data[] = {
            1, 2, 3, 0xEE,
            4, 5, 6, 0xEE,
        };
        const csstv_image_t image{data, 1, 2, 4, CSSTV_PIXEL_RGB888};
        const csstv::ImageReader reader(image);
        const csstv::RGB8 p0 = reader.pixel(0, 0);
        const csstv::RGB8 p1 = reader.pixel(0, 1);
        assert(p0.r == 1 && p0.g == 2 && p0.b == 3);
        assert(p1.r == 4 && p1.g == 5 && p1.b == 6);
    }
}
