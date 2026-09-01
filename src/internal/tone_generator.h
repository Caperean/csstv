#ifndef CSSTV_TONE_GENERATOR_H
#define CSSTV_TONE_GENERATOR_H

#include "csstv.h"

#include <cstddef>
#include <cstdint>

namespace csstv {

/*
 * Continuous-phase sine oscillator. SSTV audio is a single unbroken
 * FM tone -- every sync pulse, VIS bit, and pixel is just a change in
 * instantaneous frequency, never a phase reset -- so ToneGenerator
 * carries its phase across calls to generate(). Any discontinuity
 * here would show up as an audible click and, worse, as a false edge
 * to a receiving decoder.
 */
class ToneGenerator
{
public:
    void init(uint32_t sample_rate);

    /* Number of samples needed to cover `duration_ms` at this sample
     * rate, rounded to the nearest sample. */
    size_t samples_for_ms(double duration_ms) const;

    /* Append `count` samples of a continuous tone at `frequency_hz`
     * into `out`, continuing phase from the previous call. */
    void generate(double frequency_hz, size_t count, csstv_sample_t *out);

    /* Explicitly zero the running phase (used by reset() so repeated
     * encodes of the same image are bit-for-bit identical). */
    void reset_phase();

private:
    uint32_t sample_rate_ = 0U;
    double phase_radians_ = 0.0;
};

} /* namespace csstv */

#endif /* CSSTV_TONE_GENERATOR_H */
