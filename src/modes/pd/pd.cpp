#include "pd.h"

#include <algorithm>
#include <new>

namespace csstv {
namespace pd {

namespace {

/*
 * Standard analog SSTV tone constants (Martin/Scottie/PD family share
 * these): 1200 Hz sync, 1500 Hz porch/black, 2300 Hz white, VIS bits
 * at 1300/1100 Hz for 0/1. Luminance and averaged chroma samples are
 * all transmitted the same way -- as an 8-bit level linearly mapped
 * into the 1500-2300 Hz band.
 */
constexpr double kSyncFreqHz = 1200.0;
constexpr double kSyncDurationMs = 20.0;

constexpr double kPorchFreqHz = 1500.0;
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

double level_to_freq(double level_0_255)
{
    const double clamped = std::min(255.0, std::max(0.0, level_0_255));
    return kLevelToFreqMin + (clamped / 255.0) * kLevelToFreqSpan;
}

/* Full-range (JPEG-style) RGB -> YCbCr, the conversion used by
 * essentially every SSTV encoder for chroma-subsampled modes. */
double luma(const RGB8 &p)
{
    return 0.299 * p.r + 0.587 * p.g + 0.114 * p.b;
}

double chroma_cr(const RGB8 &p)
{
    return 128.0 + 0.500000 * p.r - 0.418688 * p.g - 0.081312 * p.b;
}

double chroma_cb(const RGB8 &p)
{
    return 128.0 - 0.168736 * p.r - 0.331264 * p.g + 0.500000 * p.b;
}

} /* namespace */

Driver::Driver(const ModeParams &params) : params_(params) {}

void Driver::build_vis_segments()
{
    size_t idx = 0U;

    vis_segments_[idx++] = {kVisLeaderFreqHz, kVisLeaderDurationMs};
    vis_segments_[idx++] = {kVisBreakFreqHz, kVisBreakDurationMs};
    vis_segments_[idx++] = {kVisLeaderFreqHz, kVisLeaderDurationMs};
    vis_segments_[idx++] = {kSyncFreqHz, kVisBitDurationMs}; /* start bit */

    uint8_t ones = 0U;
    for (uint8_t bit = 0U; bit < 7U; ++bit)
    {
        const bool one = ((params_.vis_code >> bit) & 0x01U) != 0U; /* LSB first */
        if (one)
        {
            ++ones;
        }
        vis_segments_[idx++] = {one ? kVisOneFreqHz : kVisZeroFreqHz, kVisBitDurationMs};
    }

    /* Even parity: the parity bit makes the total count of 1 bits even. */
    const bool parity_one = (ones % 2U) != 0U;
    vis_segments_[idx++] = {parity_one ? kVisOneFreqHz : kVisZeroFreqHz, kVisBitDurationMs};
    vis_segments_[idx++] = {kSyncFreqHz, kVisBitDurationMs}; /* stop bit */

    /* idx must land exactly on kVisSegmentCount; a mismatch means the
     * segment table above and its declared size have drifted apart. */
    (void)idx;
}

void Driver::restart_scan()
{
    vis_index_ = 0U;
    stage_ = Stage::kVisHeader;
    line_pair_ = 0U;
    pixel_index_ = 0U;
    segment_remaining_samples_ = 0U;
    current_freq_hz_ = 0.0;
    ideal_elapsed_samples_ = 0.0;
    rounded_elapsed_samples_ = 0U;
    finished_ = false;
}

csstv_status_t Driver::begin(const csstv_image_t &image, uint32_t sample_rate)
{
    if (sample_rate == 0U)
    {
        return CSSTV_ERROR_INVALID_SAMPLE_RATE;
    }

    if (image.width != params_.width || image.height != params_.height)
    {
        return CSSTV_ERROR_INVALID_IMAGE;
    }

    reader_ = ImageReader(image);
    sample_rate_ = sample_rate;
    tone_.init(sample_rate);
    tone_.reset_phase();

    pixel_duration_ms_ = params_.color_scan_ms / static_cast<double>(params_.width);
    line_pair_count_ = static_cast<uint32_t>(params_.height) / 2U;

    build_vis_segments();
    restart_scan();

    return CSSTV_OK;
}

csstv_status_t Driver::reset()
{
    tone_.reset_phase();
    restart_scan();
    return CSSTV_OK;
}

double Driver::y_frequency(uint16_t x, uint16_t y) const
{
    return level_to_freq(luma(reader_.pixel(x, y)));
}

double Driver::cr_frequency(uint16_t x, uint16_t y_first_of_pair) const
{
    const double cr0 = chroma_cr(reader_.pixel(x, y_first_of_pair));
    const double cr1 = chroma_cr(reader_.pixel(x, static_cast<uint16_t>(y_first_of_pair + 1U)));
    return level_to_freq(0.5 * (cr0 + cr1));
}

double Driver::cb_frequency(uint16_t x, uint16_t y_first_of_pair) const
{
    const double cb0 = chroma_cb(reader_.pixel(x, y_first_of_pair));
    const double cb1 = chroma_cb(reader_.pixel(x, static_cast<uint16_t>(y_first_of_pair + 1U)));
    return level_to_freq(0.5 * (cb0 + cb1));
}

bool Driver::next_segment(double &freq_hz, double &duration_ms)
{
    switch (stage_)
    {
        case Stage::kVisHeader:
        {
            if (vis_index_ >= kVisSegmentCount)
            {
                stage_ = Stage::kLineSync;
                return next_segment(freq_hz, duration_ms);
            }
            freq_hz = vis_segments_[vis_index_].freq_hz;
            duration_ms = vis_segments_[vis_index_].duration_ms;
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
            freq_hz = kSyncFreqHz;
            duration_ms = kSyncDurationMs;
            stage_ = Stage::kLinePorch;
            return true;
        }

        case Stage::kLinePorch:
        {
            freq_hz = kPorchFreqHz;
            duration_ms = kPorchDurationMs;
            pixel_index_ = 0U;
            stage_ = Stage::kLineY1;
            return true;
        }

        case Stage::kLineY1:
        {
            const uint16_t y = static_cast<uint16_t>(line_pair_ * 2U);
            freq_hz = y_frequency(pixel_index_, y);
            duration_ms = pixel_duration_ms_;
            if (++pixel_index_ >= params_.width)
            {
                pixel_index_ = 0U;
                stage_ = Stage::kLineCr;
            }
            return true;
        }

        case Stage::kLineCr:
        {
            const uint16_t y = static_cast<uint16_t>(line_pair_ * 2U);
            freq_hz = cr_frequency(pixel_index_, y);
            duration_ms = pixel_duration_ms_;
            if (++pixel_index_ >= params_.width)
            {
                pixel_index_ = 0U;
                stage_ = Stage::kLineCb;
            }
            return true;
        }

        case Stage::kLineCb:
        {
            const uint16_t y = static_cast<uint16_t>(line_pair_ * 2U);
            freq_hz = cb_frequency(pixel_index_, y);
            duration_ms = pixel_duration_ms_;
            if (++pixel_index_ >= params_.width)
            {
                pixel_index_ = 0U;
                stage_ = Stage::kLineY2;
            }
            return true;
        }

        case Stage::kLineY2:
        {
            const uint16_t y = static_cast<uint16_t>(line_pair_ * 2U + 1U);
            freq_hz = y_frequency(pixel_index_, y);
            duration_ms = pixel_duration_ms_;
            if (++pixel_index_ >= params_.width)
            {
                pixel_index_ = 0U;
                ++line_pair_;
                stage_ = Stage::kLineSync;
            }
            return true;
        }

        case Stage::kDone:
        default:
            return false;
    }
}

csstv_status_t Driver::read(csstv_sample_t *out, size_t capacity, size_t *written)
{
    size_t total = 0U;

    while (total < capacity)
    {
        if (segment_remaining_samples_ == 0U)
        {
            double freq_hz;
            double duration_ms;

            if (!next_segment(freq_hz, duration_ms))
            {
                finished_ = true;
                break;
            }

            current_freq_hz_ = freq_hz;

            /* Round the *cumulative* elapsed time, not this segment in
             * isolation -- see the comment on these fields in pd.h. */
            ideal_elapsed_samples_ += duration_ms * 0.001 * static_cast<double>(sample_rate_);
            const size_t target_rounded = static_cast<size_t>(ideal_elapsed_samples_ + 0.5);
            segment_remaining_samples_ = target_rounded - rounded_elapsed_samples_;
            rounded_elapsed_samples_ = target_rounded;

            if (segment_remaining_samples_ == 0U)
            {
                continue; /* degenerate (rounds-to-zero) segment; skip it */
            }
        }

        const size_t n = std::min(capacity - total, segment_remaining_samples_);
        tone_.generate(current_freq_hz_, n, out + total);

        total += n;
        segment_remaining_samples_ -= n;
    }

    *written = total;
    return CSSTV_OK;
}

bool Driver::finished() const
{
    return finished_;
}

/* -------------------------------------------------------------------- */
/* Mode table / dispatch                                                */
/* -------------------------------------------------------------------- */

const ModeParams *find_params(csstv_mode_t mode)
{
    switch (mode)
    {
#if CSSTV_ENCODER_MODE_PD50
        case CSSTV_MODE_PD50:
            return &kPd50Params;
#endif
#if CSSTV_ENCODER_MODE_PD90
        case CSSTV_MODE_PD90:
            return &kPd90Params;
#endif
#if CSSTV_ENCODER_MODE_PD120
        case CSSTV_MODE_PD120:
            return &kPd120Params;
#endif
#if CSSTV_ENCODER_MODE_PD160
        case CSSTV_MODE_PD160:
            return &kPd160Params;
#endif
#if CSSTV_ENCODER_MODE_PD180
        case CSSTV_MODE_PD180:
            return &kPd180Params;
#endif
#if CSSTV_ENCODER_MODE_PD240
        case CSSTV_MODE_PD240:
            return &kPd240Params;
#endif
#if CSSTV_ENCODER_MODE_PD290
        case CSSTV_MODE_PD290:
            return &kPd290Params;
#endif
        default:
            return nullptr;
    }
}

csstv_status_t make_mode_info(csstv_mode_t mode, csstv_mode_info_t *info)
{
    const ModeParams *params = find_params(mode);
    if (params == nullptr)
    {
        return CSSTV_ERROR_UNSUPPORTED_MODE;
    }

    /* leader + break + leader + (start + 7 bits + parity + stop) * 30ms */
    constexpr double kVisHeaderMs =
        kVisLeaderDurationMs + kVisBreakDurationMs + kVisLeaderDurationMs + 10.0 * kVisBitDurationMs;

    const double line_pair_ms = kSyncDurationMs + kPorchDurationMs + 4.0 * params->color_scan_ms;
    const uint32_t line_pair_count = static_cast<uint32_t>(params->height) / 2U;
    const double total_ms = kVisHeaderMs + static_cast<double>(line_pair_count) * line_pair_ms;

    info->mode = params->mode;
    info->width = params->width;
    info->height = params->height;
    info->duration_ms = static_cast<uint32_t>(total_ms + 0.5);

    return CSSTV_OK;
}

ModeDriver *create(csstv_mode_t mode, void *storage, size_t storage_size)
{
    const ModeParams *params = find_params(mode);
    if (params == nullptr)
    {
        return nullptr;
    }

    if (storage_size < sizeof(Driver))
    {
        return nullptr;
    }

    return new (storage) Driver(*params);
}

} /* namespace pd */
} /* namespace csstv */
