#include "pd.h"

/*
 * kPd290Params timing (standard PD family / hamradio SSTV constants):
 *   800x616, 228.8 ms per color-scan (Y, Cr, or Cb),
 *   VIS code 94.
 */

#if CSSTV_ENCODER_MODE_PD290

namespace csstv {
namespace pd {

const ModeParams kPd290Params = {
    CSSTV_MODE_PD290,
    800U,
    616U,
    228.8,
    94U
};

} /* namespace pd */
} /* namespace csstv */

#endif /* CSSTV_ENCODER_MODE_PD290 */
