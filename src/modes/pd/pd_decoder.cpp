#include "pd_decoder.h"

#if CSSTV_ENABLE_DECODER

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

namespace csstv {
namespace pd {

namespace {

constexpr double kTwoPi = 6.283185307179586476925286766559;
constexpr size_t kHilbertOrder = 15U;
constexpr size_t kHilbertDelay = kHilbertOrder / 2U; /* 7 */

constexpr double kSyncFreqHz = 1200.0;
constexpr double kSyncDurationMs = 20.0;
constexpr double kPorchDurationMs = 2.08;

constexpr double kVisLeaderFreqHz = 1900.0;
constexpr double kVisLeaderDurationMs = 300.0;
constexpr double kVisBreakFreqHz = 1200.0;
constexpr double kVisBreakDurationMs = 10.0;
constexpr double kVisBitDurationMs = 30.0;
constexpr double kVisZeroFreqHz = 1300.0;
constexpr double kVisOneFreqHz = 1100.0;

constexpr double kLevelToFreqMin = 1500.0;
constexpr double kLevelToFreqSpan = 800.0; /* 2300 - 1500 */
constexpr double kLevelToFreqMax = kLevelToFreqMin + kLevelToFreqSpan;

/*
 * Type-III Hilbert FIR (order 15): h[k]=2/(kπ) for odd k, antisymmetric
 * about the centre tap. Produces the quadrature arm of the analytic signal.
 */
constexpr double kHilbertTaps[kHilbertOrder] = {
    -2.0 / (7.0 * 3.14159265358979323846),
    0.0,
    -2.0 / (5.0 * 3.14159265358979323846),
    0.0,
    -2.0 / (3.0 * 3.14159265358979323846),
    0.0,
    -2.0 / (1.0 * 3.14159265358979323846),
    0.0,
    2.0 / (1.0 * 3.14159265358979323846),
    0.0,
    2.0 / (3.0 * 3.14159265358979323846),
    0.0,
    2.0 / (5.0 * 3.14159265358979323846),
    0.0,
    2.0 / (7.0 * 3.14159265358979323846),
};

double freq_to_level(double freq_hz)
{
    const double clamped = std::min(kLevelToFreqMax, std::max(kLevelToFreqMin, freq_hz));
    return ((clamped - kLevelToFreqMin) / kLevelToFreqSpan) * 255.0;
}

uint8_t level_to_u8(double level)
{
    const double clamped = std::min(255.0, std::max(0.0, level + 0.5));
    return static_cast<uint8_t>(clamped);
}

void ycbcr_to_rgb(uint8_t y, uint8_t cr, uint8_t cb, uint8_t *r, uint8_t *g, uint8_t *b)
{
    const double Y = static_cast<double>(y);
    const double Cr = static_cast<double>(cr) - 128.0;
    const double Cb = static_cast<double>(cb) - 128.0;

    const double R = Y + 1.402 * Cr;
    const double G = Y - 0.344136 * Cb - 0.714136 * Cr;
    const double B = Y + 1.772 * Cb;

    *r = level_to_u8(R);
    *g = level_to_u8(G);
    *b = level_to_u8(B);
}

} /* namespace */

DecoderDriver::DecoderDriver(const ModeParams &params) : params_(params) {}

void DecoderDriver::build_vis_segments()
{
    size_t idx = 0U;

    vis_segments_[idx++] = {kVisLeaderFreqHz, kVisLeaderDurationMs, false};
    vis_segments_[idx++] = {kVisBreakFreqHz, kVisBreakDurationMs, false};
    vis_segments_[idx++] = {kVisLeaderFreqHz, kVisLeaderDurationMs, false};
    vis_segments_[idx++] = {kSyncFreqHz, kVisBitDurationMs, false}; /* start */

    uint8_t ones = 0U;
    for (uint8_t bit = 0U; bit < 7U; ++bit)
    {
        const bool one = ((params_.vis_code >> bit) & 0x01U) != 0U;
        if (one)
        {
            ++ones;
        }
        vis_segments_[idx++] = {
            one ? kVisOneFreqHz : kVisZeroFreqHz,
            kVisBitDurationMs,
            false};
    }

    const bool parity_one = (ones % 2U) != 0U;
    vis_segments_[idx++] = {
        parity_one ? kVisOneFreqHz : kVisZeroFreqHz,
        kVisBitDurationMs,
        false};
    vis_segments_[idx++] = {kSyncFreqHz, kVisBitDurationMs, false}; /* stop */

    (void)idx;
}

void DecoderDriver::demod_reset()
{
    std::memset(hilbert_x_, 0, sizeof(hilbert_x_));
    hilbert_pos_ = 0U;
    prev_i_ = 0.0;
    prev_q_ = 0.0;
    demod_primed_ = false;
}

double DecoderDriver::demod_sample(csstv_sample_t sample)
{
    const double x = static_cast<double>(sample);

    hilbert_x_[hilbert_pos_] = x;
    hilbert_pos_ = (hilbert_pos_ + 1U) % kHilbertOrder;

    double q = 0.0;
    for (size_t k = 0U; k < kHilbertOrder; ++k)
    {
        const size_t idx = (hilbert_pos_ + k) % kHilbertOrder;
        q += kHilbertTaps[k] * hilbert_x_[idx];
    }

    /* Match the FIR group delay so I and Q are time-aligned. */
    const size_t i_idx = (hilbert_pos_ + kHilbertDelay) % kHilbertOrder;
    const double i = hilbert_x_[i_idx];

    if (!demod_primed_)
    {
        prev_i_ = i;
        prev_q_ = q;
        demod_primed_ = true;
        return 1900.0;
    }

    const double re = i * prev_i_ + q * prev_q_;
    const double im = q * prev_i_ - i * prev_q_;
    prev_i_ = i;
    prev_q_ = q;

    const double dphi = std::atan2(im, re);
    return std::fabs(dphi) * static_cast<double>(sample_rate_) / kTwoPi;
}

void DecoderDriver::restart_scan()
{
    vis_index_ = 0U;
    stage_ = Stage::kVisHeader;
    line_pair_ = 0U;
    pixel_index_ = 0U;
    ideal_elapsed_samples_ = 0.0;
    rounded_elapsed_samples_ = 0U;
    segment_remaining_samples_ = 0U;
    segment_total_samples_ = 0U;
    segment_seen_samples_ = 0U;
    segment_recover_level_ = false;
    segment_freq_sum_ = 0.0;
    segment_freq_count_ = 0U;
    finished_ = false;
    demod_reset();
    std::memset(y1_line_, 0, sizeof(y1_line_));
    std::memset(cr_line_, 0, sizeof(cr_line_));
    std::memset(cb_line_, 0, sizeof(cb_line_));
}

csstv_status_t DecoderDriver::init(csstv_mode_t mode, uint32_t sample_rate)
{
    if (sample_rate == 0U)
    {
        return CSSTV_ERROR_INVALID_SAMPLE_RATE;
    }

    if (mode != params_.mode)
    {
        return CSSTV_ERROR_UNSUPPORTED_MODE;
    }

    if (params_.width > kMaxWidth || (params_.height % 2U) != 0U)
    {
        return CSSTV_ERROR_UNSUPPORTED_MODE;
    }

    sample_rate_ = sample_rate;
    pixel_duration_ms_ = params_.color_scan_ms / static_cast<double>(params_.width);
    line_pair_count_ = static_cast<uint32_t>(params_.height) / 2U;
    has_image_ = false;
    image_data_ = nullptr;
    image_stride_ = 0U;

    build_vis_segments();
    restart_scan();
    finished_ = true; /* idle until set_image() */

    return CSSTV_OK;
}

csstv_status_t DecoderDriver::set_image(const csstv_image_t &image)
{
    if (image.data == nullptr)
    {
        return CSSTV_ERROR_INVALID_IMAGE;
    }

    if (image.width != params_.width || image.height != params_.height)
    {
        return CSSTV_ERROR_INVALID_IMAGE;
    }

    if (image.format != CSSTV_PIXEL_RGB888)
    {
        return CSSTV_ERROR_INVALID_IMAGE;
    }

    const size_t min_stride = static_cast<size_t>(params_.width) * 3U;
    const size_t stride = (image.stride == 0U) ? min_stride : image.stride;
    if (stride < min_stride)
    {
        return CSSTV_ERROR_INVALID_IMAGE;
    }

    /* Output buffer is caller-owned and must be mutable; the public
     * csstv_image_t uses const void * so the encoder can share the type. */
    image_data_ = const_cast<uint8_t *>(static_cast<const uint8_t *>(image.data));
    image_stride_ = stride;
    has_image_ = true;

    std::memset(image_data_, 0, image_stride_ * static_cast<size_t>(params_.height));
    restart_scan();

    return CSSTV_OK;
}

bool DecoderDriver::next_segment(double &duration_ms, bool &recover_level)
{
    switch (stage_)
    {
        case Stage::kVisHeader:
        {
            if (vis_index_ >= kVisSegmentCount)
            {
                stage_ = Stage::kLineSync;
                return next_segment(duration_ms, recover_level);
            }
            duration_ms = vis_segments_[vis_index_].duration_ms;
            recover_level = false;
            ++vis_index_;
            return true;
        }

        case Stage::kLineSync:
        {
            if (line_pair_ >= line_pair_count_)
            {
                stage_ = Stage::kDone;
                return false;
            }
            duration_ms = kSyncDurationMs;
            recover_level = false;
            stage_ = Stage::kLinePorch;
            return true;
        }

        case Stage::kLinePorch:
        {
            duration_ms = kPorchDurationMs;
            recover_level = false;
            pixel_index_ = 0U;
            stage_ = Stage::kLineY1;
            return true;
        }

        case Stage::kLineY1:
        case Stage::kLineCr:
        case Stage::kLineCb:
        case Stage::kLineY2:
        {
            duration_ms = pixel_duration_ms_;
            recover_level = true;
            return true;
        }

        case Stage::kDone:
        default:
            return false;
    }
}

void DecoderDriver::write_rgb_pair(uint16_t x, uint8_t y1, uint8_t y2, uint8_t cr, uint8_t cb) const
{
    const uint16_t row0 = static_cast<uint16_t>(line_pair_ * 2U);
    const uint16_t row1 = static_cast<uint16_t>(row0 + 1U);

    uint8_t *p0 = image_data_ + static_cast<size_t>(row0) * image_stride_ + static_cast<size_t>(x) * 3U;
    uint8_t *p1 = image_data_ + static_cast<size_t>(row1) * image_stride_ + static_cast<size_t>(x) * 3U;

    ycbcr_to_rgb(y1, cr, cb, p0, p0 + 1, p0 + 2);
    ycbcr_to_rgb(y2, cr, cb, p1, p1 + 1, p1 + 2);
}

void DecoderDriver::store_level(double level)
{
    const uint8_t value = level_to_u8(level);

    switch (stage_)
    {
        case Stage::kLineY1:
            y1_line_[pixel_index_] = value;
            if (++pixel_index_ >= params_.width)
            {
                pixel_index_ = 0U;
                stage_ = Stage::kLineCr;
            }
            break;

        case Stage::kLineCr:
            cr_line_[pixel_index_] = value;
            if (++pixel_index_ >= params_.width)
            {
                pixel_index_ = 0U;
                stage_ = Stage::kLineCb;
            }
            break;

        case Stage::kLineCb:
            cb_line_[pixel_index_] = value;
            if (++pixel_index_ >= params_.width)
            {
                pixel_index_ = 0U;
                stage_ = Stage::kLineY2;
            }
            break;

        case Stage::kLineY2:
            write_rgb_pair(
                pixel_index_,
                y1_line_[pixel_index_],
                value,
                cr_line_[pixel_index_],
                cb_line_[pixel_index_]);
            if (++pixel_index_ >= params_.width)
            {
                pixel_index_ = 0U;
                ++line_pair_;
                stage_ = Stage::kLineSync;
            }
            break;

        default:
            break;
    }
}

void DecoderDriver::finish_segment(double mean_freq_hz)
{
    if (segment_recover_level_)
    {
        store_level(freq_to_level(mean_freq_hz));
    }
}

csstv_status_t DecoderDriver::write(const csstv_sample_t *in, size_t count, size_t *consumed)
{
    if (!has_image_)
    {
        *consumed = 0U;
        return CSSTV_ERROR_NOT_READY;
    }

    if (finished_)
    {
        *consumed = 0U;
        return CSSTV_OK;
    }

    size_t total = 0U;

    while (!finished_)
    {
        if (segment_remaining_samples_ == 0U)
        {
            double duration_ms = 0.0;
            bool recover_level = false;

            if (!next_segment(duration_ms, recover_level))
            {
                finished_ = true;
                break;
            }

            segment_recover_level_ = recover_level;
            segment_freq_sum_ = 0.0;
            segment_freq_count_ = 0U;
            segment_seen_samples_ = 0U;

            ideal_elapsed_samples_ += duration_ms * 0.001 * static_cast<double>(sample_rate_);
            const size_t target_rounded = static_cast<size_t>(ideal_elapsed_samples_ + 0.5);
            segment_remaining_samples_ = target_rounded - rounded_elapsed_samples_;
            rounded_elapsed_samples_ = target_rounded;
            segment_total_samples_ = segment_remaining_samples_;

            if (segment_remaining_samples_ == 0U)
            {
                if (segment_recover_level_)
                {
                    finish_segment(1900.0);
                }
                continue;
            }
        }

        if (total >= count)
        {
            break;
        }

        const size_t n = std::min(count - total, segment_remaining_samples_);

        /* Skip the Hilbert settling / tone-transition region when
         * averaging; keep at least one sample when the segment is short. */
        const size_t skip = std::min(kHilbertDelay, segment_total_samples_ / 4U);

        for (size_t i = 0U; i < n; ++i)
        {
            const double freq = demod_sample(in[total + i]);
            if (segment_recover_level_ && segment_seen_samples_ >= skip)
            {
                segment_freq_sum_ += freq;
                ++segment_freq_count_;
            }
            ++segment_seen_samples_;
        }

        total += n;
        segment_remaining_samples_ -= n;

        if (segment_remaining_samples_ == 0U)
        {
            double mean_freq = 1900.0;
            if (segment_freq_count_ > 0U)
            {
                mean_freq = segment_freq_sum_ / static_cast<double>(segment_freq_count_);
            }
            finish_segment(mean_freq);
            /* Loop again so a completed final segment can mark finished
             * even when this write() had no spare input samples. */
        }
    }

    *consumed = total;
    return CSSTV_OK;
}

bool DecoderDriver::finished() const
{
    return finished_;
}

csstv_status_t DecoderDriver::get_image(csstv_image_t *image)
{
    if (!finished_ || !has_image_)
    {
        return CSSTV_ERROR_NOT_READY;
    }

    if (image == nullptr)
    {
        return CSSTV_ERROR_NULL;
    }

    image->data = image_data_;
    image->width = params_.width;
    image->height = params_.height;
    image->stride = image_stride_;
    image->format = CSSTV_PIXEL_RGB888;

    return CSSTV_OK;
}

csstv_status_t DecoderDriver::reset()
{
    if (!has_image_)
    {
        return CSSTV_ERROR_NOT_READY;
    }

    std::memset(image_data_, 0, image_stride_ * static_cast<size_t>(params_.height));
    restart_scan();
    return CSSTV_OK;
}

DecoderDriver *create_decoder(csstv_mode_t mode, void *storage, size_t storage_size)
{
    const ModeParams *params = find_decoder_params(mode);
    if (params == nullptr)
    {
        return nullptr;
    }

    if (storage_size < sizeof(DecoderDriver))
    {
        return nullptr;
    }

    return new (storage) DecoderDriver(*params);
}

} /* namespace pd */
} /* namespace csstv */

#endif /* CSSTV_ENABLE_DECODER */
