#include "pd.h"

/*
 * kPd90Params timing (standard PD family / hamradio SSTV constants):
 *   320x256, 170.24 ms per color-scan (Y, Cr, or Cb),
 *   VIS code 99.
 */

#if CSSTV_ENCODER_MODE_PD90

namespace csstv {
namespace pd {

const ModeParams kPd90Params = {
    CSSTV_MODE_PD90,
    320U,
    256U,
    170.24,
    99U
};

} /* namespace pd */
} /* namespace csstv */

#endif /* CSSTV_ENCODER_MODE_PD90 */
