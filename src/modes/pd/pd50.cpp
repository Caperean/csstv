#include "pd.h"

/*
 * kPd50Params timing (standard PD family / hamradio SSTV constants):
 *   320x256, 91.52 ms per color-scan (Y, Cr, or Cb),
 *   VIS code 93.
 */

#if CSSTV_ENCODER_MODE_PD50

namespace csstv {
namespace pd {

const ModeParams kPd50Params = {
    CSSTV_MODE_PD50,
    320U,
    256U,
    91.52,
    93U
};

} /* namespace pd */
} /* namespace csstv */

#endif /* CSSTV_ENCODER_MODE_PD50 */
