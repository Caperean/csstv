#include "pd.h" //pd_modes.cpp

namespace csstv {

namespace pd {

#if CSSTV_ENCODER_MODE_PD50

const ModeParams kPd50Params = {
    CSSTV_MODE_PD50,
    320U,
    256U,
    91.52,
    93U
};

#endif

#if CSSTV_ENCODER_MODE_PD90

const ModeParams kPd90Params = {
    CSSTV_MODE_PD90,
    320U,
    256U,
    170.24,
    99U
};

#endif

#if CSSTV_ENCODER_MODE_PD120

const ModeParams kPd120Params = {
    CSSTV_MODE_PD120,
    640U,
    496U,
    121.6,
    95U
};

#endif

#if CSSTV_ENCODER_MODE_PD160

const ModeParams kPd160Params = {
    CSSTV_MODE_PD160,
    512U,
    400U,
    195.584,
    98U
};

#endif

#if CSSTV_ENCODER_MODE_PD180

const ModeParams kPd180Params = {
    CSSTV_MODE_PD180,
    640U,
    496U,
    183.04,
    96U
};

#endif

#if CSSTV_ENCODER_MODE_PD240

const ModeParams kPd240Params = {
    CSSTV_MODE_PD240,
    640U,
    496U,
    244.48,
    97U
};

#endif

#if CSSTV_ENCODER_MODE_PD290

const ModeParams kPd290Params = {
    CSSTV_MODE_PD290,
    800U,
    616U,
    228.8,
    94U
};

#endif

}  // namespace pd

}  // namespace csstv

