#ifndef CSSTV_PD_DECODER_H
#define CSSTV_PD_DECODER_H

#include "pd.h"

#if CSSTV_ENABLE_DECODER

#include "csstv_decoder_driver.h"

#include <cstddef>
#include <cstdint>

namespace csstv {
namespace pd {

/*
 * Widest PD mode currently supported (PD290). Line buffers are sized
 * to this so the decoder never allocates on the heap.
 */
constexpr uint16_t kMaxWidth = 800U;

/*
 * PD decoder driver. Mirrors the encoder's segment timing (including
 * cumulative sample rounding) and recovers levels with a continuous
 * FM demodulator. The caller supplies the output image buffer via
 * set_image(); no heap is used.
 */
class DecoderDriver final : public ::csstv::DecoderDriver
{
public:
    explicit DecoderDriver(const ModeParams &params);

    csstv_status_t init(csstv_mode_t mode, uint32_t sample_rate) override;
    csstv_status_t set_image(const csstv_image_t &image) override;
    csstv_status_t write(const csstv_sample_t *in, size_t count, size_t *consumed) override;
    bool finished() const override;
    csstv_status_t get_image(csstv_image_t *image) override;
    csstv_status_t reset() override;

private:
    enum class Stage : uint8_t
    {
        kVisHeader,
        kLineSync,
        kLinePorch,
        kLineY1,
        kLineCr,
        kLineCb,
        kLineY2,
        kDone
    };

    struct Segment
    {
        double freq_hz; /* nominal; unused for level recovery */
        double duration_ms;
        bool recover_level;
    };

    static constexpr size_t kVisSegmentCount = 13U;

    void build_vis_segments();
    void restart_scan();
    bool next_segment(double &duration_ms, bool &recover_level);
    void finish_segment(double mean_freq_hz);
    void store_level(double level);
    void write_rgb_pair(uint16_t x, uint8_t y1, uint8_t y2, uint8_t cr, uint8_t cb);
    void demod_reset();
    double demod_sample(csstv_sample_t sample);

    const ModeParams &params_;
    uint32_t sample_rate_ = 0U;
    double pixel_duration_ms_ = 0.0;
    uint32_t line_pair_count_ = 0U;

    uint8_t *image_data_ = nullptr;
    size_t image_stride_ = 0U;
    bool has_image_ = false;

    Segment vis_segments_[kVisSegmentCount]{};
    uint8_t vis_index_ = 0U;

    Stage stage_ = Stage::kDone;
    uint32_t line_pair_ = 0U;
    uint16_t pixel_index_ = 0U;

    /* Same cumulative-rounding model as the encoder. */
    double ideal_elapsed_samples_ = 0.0;
    size_t rounded_elapsed_samples_ = 0U;
    size_t segment_remaining_samples_ = 0U;
    size_t segment_total_samples_ = 0U;
    size_t segment_seen_samples_ = 0U;
    bool segment_recover_level_ = false;
    double segment_freq_sum_ = 0.0;
    size_t segment_freq_count_ = 0U;

    bool finished_ = true;

    /* Per-line-pair scratch (no heap). */
    uint8_t y1_line_[kMaxWidth]{};
    uint8_t cr_line_[kMaxWidth]{};
    uint8_t cb_line_[kMaxWidth]{};

    /* Hilbert + phase-difference FM demodulator state. */
    static constexpr size_t kDemodOrder = 15U;
    double hilbert_x_[kDemodOrder]{};
    size_t hilbert_pos_ = 0U;
    double prev_i_ = 0.0;
    double prev_q_ = 0.0;
    bool demod_primed_ = false;
};

DecoderDriver *create_decoder(csstv_mode_t mode, void *storage, size_t storage_size);

} /* namespace pd */
} /* namespace csstv */

#endif /* CSSTV_ENABLE_DECODER */

#endif /* CSSTV_PD_DECODER_H */
