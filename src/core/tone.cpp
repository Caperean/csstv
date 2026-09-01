#include "tone_generator.h"

#include <cmath>

namespace csstv {

namespace {
constexpr double kTwoPi = 6.283185307179586476925286766559;
constexpr double kAmplitude = 32000.0; /* leave headroom below INT16_MAX */
} /* namespace */

void ToneGenerator::init(uint32_t sample_rate)
{
    sample_rate_ = sample_rate;
    phase_radians_ = 0.0;
}

void ToneGenerator::reset_phase()
{
    phase_radians_ = 0.0;
}

size_t ToneGenerator::samples_for_ms(double duration_ms) const
{
    if (sample_rate_ == 0U || duration_ms <= 0.0)
    {
        return 0U;
    }

    const double samples = duration_ms * 0.001 * static_cast<double>(sample_rate_);

    return static_cast<size_t>(samples + 0.5);
}

void ToneGenerator::generate(double frequency_hz, size_t count, csstv_sample_t *out)
{
    if (sample_rate_ == 0U)
    {
        for (size_t i = 0U; i < count; ++i)
        {
            out[i] = 0;
        }
        return;
    }

    const double increment = kTwoPi * frequency_hz / static_cast<double>(sample_rate_);

    for (size_t i = 0U; i < count; ++i)
    {
        out[i] = static_cast<csstv_sample_t>(kAmplitude * std::sin(phase_radians_));

        phase_radians_ += increment;

        if (phase_radians_ >= kTwoPi)
        {
            phase_radians_ -= kTwoPi;
        }
    }
}

} /* namespace csstv */
