#include "image_reader.h"

#include <cstring>

namespace csstv {

ImageReader::ImageReader(const csstv_image_t &image) : image_(image) {}

size_t ImageReader::bytes_per_pixel(csstv_pixel_format_t format)
{
    switch (format)
    {
        case CSSTV_PIXEL_GRAY8:
            return 1U;

        case CSSTV_PIXEL_RGB888:
        case CSSTV_PIXEL_BGR888:
            return 3U;

        case CSSTV_PIXEL_RGB565:
            return 2U;

        default:
            return 0U;
    }
}

size_t ImageReader::row_stride() const
{
    if (image_.stride != 0U)
    {
        return image_.stride;
    }

    return static_cast<size_t>(image_.width) * bytes_per_pixel(image_.format);
}

RGB8 ImageReader::pixel(uint16_t x, uint16_t y) const
{
    const uint8_t *base = static_cast<const uint8_t *>(image_.data);
    const uint8_t *row = base + static_cast<size_t>(y) * row_stride();

    RGB8 out{};

    switch (image_.format)
    {
        case CSSTV_PIXEL_GRAY8:
        {
            const uint8_t v = row[x];
            out.r = v;
            out.g = v;
            out.b = v;
            break;
        }

        case CSSTV_PIXEL_RGB888:
        {
            const uint8_t *p = row + static_cast<size_t>(x) * 3U;
            out.r = p[0];
            out.g = p[1];
            out.b = p[2];
            break;
        }

        case CSSTV_PIXEL_BGR888:
        {
            const uint8_t *p = row + static_cast<size_t>(x) * 3U;
            out.b = p[0];
            out.g = p[1];
            out.r = p[2];
            break;
        }

        case CSSTV_PIXEL_RGB565:
        {
            const uint8_t *p = row + static_cast<size_t>(x) * 2U;

            uint16_t packed;
            std::memcpy(&packed, p, sizeof(packed)); /* avoid alignment UB */

            const uint8_t r5 = static_cast<uint8_t>((packed >> 11) & 0x1FU);
            const uint8_t g6 = static_cast<uint8_t>((packed >> 5) & 0x3FU);
            const uint8_t b5 = static_cast<uint8_t>(packed & 0x1FU);

            /* Expand N-bit channels to 8-bit by replicating the high bits
             * into the vacated low bits, rather than a plain left-shift,
             * so 0x00 -> 0x00 and the max value -> 0xFF exactly. */
            out.r = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
            out.g = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
            out.b = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
            break;
        }

        default:
            break;
    }

    return out;
}

} /* namespace csstv */
