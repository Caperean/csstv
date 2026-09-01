#include "pd.h"

/*
 * kPd240Params timing (standard PD family / hamradio SSTV constants):
 *   640x496, 244.48 ms per color-scan (Y, Cr, or Cb),
 *   VIS code 97.
 */

#if CSSTV_ENCODER_MODE_PD240

namespace csstv {
namespace pd {

const ModeParams kPd240Params = {
    CSSTV_MODE_PD240,
    640U,
    496U,
    244.48,
    97U
};

} /* namespace pd */
} /* namespace csstv */

#endif /* CSSTV_ENCODER_MODE_PD240 */
