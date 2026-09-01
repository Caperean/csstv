#include "pd.h"

/*
 * kPd160Params timing (standard PD family / hamradio SSTV constants):
 *   512x400, 195.584 ms per color-scan (Y, Cr, or Cb),
 *   VIS code 98.
 */

#if CSSTV_ENCODER_MODE_PD160

namespace csstv {
namespace pd {

const ModeParams kPd160Params = {
    CSSTV_MODE_PD160,
    512U,
    400U,
    195.584,
    98U
};

} /* namespace pd */
} /* namespace csstv */

#endif /* CSSTV_ENCODER_MODE_PD160 */
