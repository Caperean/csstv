#include "csstv.h"
#include "csstv_mode_driver.h"
#include "modes/pd/pd.h"

namespace csstv {

namespace {

/* csstv_mode_t values are family-tagged by their high byte
 * (0x01xx == PD, see csstv.h). This is the one seam a future mode
 * family -- Scottie, Martin, Robot, etc. -- would plug into. */
constexpr csstv_mode_t kFamilyMask = 0xFF00U;
constexpr csstv_mode_t kFamilyPd = 0x0100U;

} /* namespace */

bool mode_supported(csstv_mode_t mode)
{
    if ((mode & kFamilyMask) == kFamilyPd)
    {
        return pd::find_params(mode) != nullptr;
    }

    return false;
}

csstv_status_t mode_get_info(csstv_mode_t mode, csstv_mode_info_t *info)
{
    if (info == nullptr)
    {
        return CSSTV_ERROR_NULL;
    }

    if ((mode & kFamilyMask) == kFamilyPd)
    {
        return pd::make_mode_info(mode, info);
    }

    return CSSTV_ERROR_UNSUPPORTED_MODE;
}

ModeDriver *create_mode_driver(csstv_mode_t mode, void *storage, size_t storage_size)
{
    if ((mode & kFamilyMask) == kFamilyPd)
    {
        return pd::create(mode, storage, storage_size);
    }

    return nullptr;
}

} /* namespace csstv */

/* ---------------------------------------------------------------- */
/* Public C API                                                     */
/* ---------------------------------------------------------------- */

extern "C" {

bool csstv_mode_supported(csstv_mode_t mode)
{
    return csstv::mode_supported(mode);
}

csstv_status_t csstv_mode_get_info(csstv_mode_t mode, csstv_mode_info_t *info)
{
    return csstv::mode_get_info(mode, info);
}

const char *csstv_version_string(void)
{
    return "0.1.0";
}

} /* extern "C" */
