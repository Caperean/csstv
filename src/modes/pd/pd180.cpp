#include "pd.h"

/*
 * kPd180Params timing (standard PD family / hamradio SSTV constants):
 *   640x496, 183.04 ms per color-scan (Y, Cr, or Cb),
 *   VIS code 96.
 */

#if CSSTV_ENCODER_MODE_PD180

namespace csstv {
namespace pd {

const ModeParams kPd180Params = {
    CSSTV_MODE_PD180,
    640U,
    496U,
    183.04,
    96U
};

} /* namespace pd */
} /* namespace csstv */

#endif /* CSSTV_ENCODER_MODE_PD180 */
