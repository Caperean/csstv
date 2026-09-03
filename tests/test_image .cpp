#include "csstv.h"
#include "test_common.h"

#include <cassert>

void test_modes()
{
    struct ExpectedMode
    {
        csstv_mode_t mode;
        uint16_t width;
        uint16_t height;
    };

    const ExpectedMode modes[] = {
        {CSSTV_MODE_PD50, 320, 256},
        {CSSTV_MODE_PD90, 320, 256},
        {CSSTV_MODE_PD120, 640, 496},
        {CSSTV_MODE_PD160, 512, 400},
        {CSSTV_MODE_PD180, 640, 496},
        {CSSTV_MODE_PD240, 640, 496},
        {CSSTV_MODE_PD290, 800, 616},
    };

    for (const ExpectedMode &expected : modes)
    {
        assert(csstv_mode_supported(expected.mode));

        csstv_mode_info_t info{};
        assert(csstv_mode_get_info(expected.mode, &info) == CSSTV_OK);
        assert(info.mode == expected.mode);
        assert(info.width == expected.width);
        assert(info.height == expected.height);
        assert(info.duration_ms > 0U);
    }

    const csstv_mode_t unknown = static_cast<csstv_mode_t>(0xFFFFU);
    assert(!csstv_mode_supported(unknown));

    csstv_mode_info_t info{};
    assert(csstv_mode_get_info(unknown, &info) == CSSTV_ERROR_UNSUPPORTED_MODE);
    assert(csstv_mode_get_info(CSSTV_MODE_PD120, nullptr) == CSSTV_ERROR_NULL);
}
