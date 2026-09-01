#ifndef CSSTV_IMAGE_READER_H
#define CSSTV_IMAGE_READER_H

#include "csstv.h"

#include <cstdint>

namespace csstv {

struct RGB8
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

/*
 * Thin, format-aware view over a csstv_image_t. Holds a copy of the
 * (small, pointer-plus-dimensions) image descriptor -- not the pixel
 * data itself -- so it is cheap to store by value.
 */
class ImageReader
{
public:
    ImageReader() = default;
    explicit ImageReader(const csstv_image_t &image);

    /* Caller is responsible for keeping x < width() and y < height(). */
    RGB8 pixel(uint16_t x, uint16_t y) const;

    uint16_t width() const { return image_.width; }
    uint16_t height() const { return image_.height; }

private:
    static size_t bytes_per_pixel(csstv_pixel_format_t format);
    size_t row_stride() const;

    csstv_image_t image_{};
};

} /* namespace csstv */

#endif /* CSSTV_IMAGE_READER_H */
