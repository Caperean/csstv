#include "pd.h"

/*
 * kPd120Params timing (standard PD family / hamradio SSTV constants):
 *   640x496, 121.6 ms per color-scan (Y, Cr, or Cb),
 *   VIS code 95.
 */

#if CSSTV_ENCODER_MODE_PD120

namespace csstv {
namespace pd {

const ModeParams kPd120Params = {
    CSSTV_MODE_PD120,
    640U,
    496U,
    121.6,
    95U
};

} /* namespace pd */
} /* namespace csstv */

#endif /* CSSTV_ENCODER_MODE_PD120 */
