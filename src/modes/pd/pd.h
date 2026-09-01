#ifndef CSSTV_MODES_PD_H
#define CSSTV_MODES_PD_H

#include "csstv.h"
#include "csstv_mode_driver.h"
#include "image_reader.h"
#include "tone_generator.h"

#include <cstddef>
#include <cstdint>

namespace csstv {
namespace pd {

/*
 * PD family frame parameters. Every PD-xxx mode shares the same wire
 * format (VIS header, then sync/porch/Y/Cr/Cb/Y per line pair) and
 * differs only in these numbers -- see pd50.cpp..pd290.cpp.
 */
struct ModeParams
{
    csstv_mode_t mode;
    uint16_t width;
    uint16_t height; /* must be even: PD transmits two lines per pair */

    /* Time to scan one full-width component (Y, Cr, or Cb), in ms.
     * Each line pair sends four of these: Y1, Cr, Cb, Y2. */
    double color_scan_ms;

    uint8_t vis_code; /* 7-bit VIS code, sent LSB-first with even parity */
};

/* One declaration per mode, defined in the matching pdXXX.cpp, and
 * only compiled in when that mode is enabled in csstv_config.h. */
#if CSSTV_ENCODER_MODE_PD50
extern const ModeParams kPd50Params;
#endif
#if CSSTV_ENCODER_MODE_PD90
extern const ModeParams kPd90Params;
#endif
#if CSSTV_ENCODER_MODE_PD120
extern const ModeParams kPd120Params;
#endif
#if CSSTV_ENCODER_MODE_PD160
extern const ModeParams kPd160Params;
#endif
#if CSSTV_ENCODER_MODE_PD180
extern const ModeParams kPd180Params;
#endif
#if CSSTV_ENCODER_MODE_PD240
extern const ModeParams kPd240Params;
#endif
#if CSSTV_ENCODER_MODE_PD290
extern const ModeParams kPd290Params;
#endif

/* Looks up the params for `mode` among the modes enabled in this
 * build. Returns nullptr if `mode` isn't a PD mode or isn't enabled. */
const ModeParams *find_params(csstv_mode_t mode);

/* Fills `info` for `mode` (dimensions + nominal transmission time). */
csstv_status_t make_mode_info(csstv_mode_t mode, csstv_mode_info_t *info);

/* Placement-constructs a Driver for `mode` into `storage`. */
ModeDriver *create(csstv_mode_t mode, void *storage, size_t storage_size);

/*
 * Generic PD encoder. One instance handles any PD variant; behavior
 * is entirely driven by the ModeParams it's constructed with.
 *
 * Generation is a pull-based state machine (see next_segment() in
 * pd.cpp) so read() can be called with buffers of any size and will
 * resume mid-tone across calls; ToneGenerator's own phase continuity
 * keeps the synthesized audio glitch-free across those calls too.
 */
class Driver final : public ModeDriver
{
public:
    explicit Driver(const ModeParams &params);

    csstv_status_t begin(const csstv_image_t &image, uint32_t sample_rate) override;
    csstv_status_t read(csstv_sample_t *out, size_t capacity, size_t *written) override;
    bool finished() const override;
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
        double freq_hz;
        double duration_ms;
    };

    static constexpr size_t kVisSegmentCount = 13U; /* 3 fixed + start + 7 bits + parity + stop */

    void build_vis_segments();
    void restart_scan();

    /* Produces the next (frequency, duration) segment and advances
     * internal cursors. Returns false once nothing remains. */
    bool next_segment(double &freq_hz, double &duration_ms);

    double y_frequency(uint16_t x, uint16_t y) const;
    double cr_frequency(uint16_t x, uint16_t y_first_of_pair) const;
    double cb_frequency(uint16_t x, uint16_t y_first_of_pair) const;

    ModeParams params_;
    ImageReader reader_;
    ToneGenerator tone_;

    uint32_t sample_rate_ = 0U;
    double pixel_duration_ms_ = 0.0;
    uint32_t line_pair_count_ = 0U;

    /* Cumulative-time sample counting: each segment's length is the
     * *change* in round(ideal elapsed samples), not an independent
     * round() of that segment's own (often sub-sample) duration.
     * Rounding every one of ~10^6 short segments in isolation would
     * push the same fractional remainder the same direction every
     * time -- e.g. a PD50 pixel is ~12.61 samples at 44.1kHz, which
     * always rounds up to 13 -- producing a steady drift that adds
     * up to whole seconds of error and, on real hardware, visible
     * picture skew. Tracking the running total keeps the cumulative
     * error bounded to under one sample for the entire transmission. */
    double ideal_elapsed_samples_ = 0.0;
    size_t rounded_elapsed_samples_ = 0U;

    Segment vis_segments_[kVisSegmentCount]{};
    uint8_t vis_index_ = 0U;

    Stage stage_ = Stage::kDone;
    uint32_t line_pair_ = 0U;
    uint16_t pixel_index_ = 0U;

    double current_freq_hz_ = 0.0;
    size_t segment_remaining_samples_ = 0U;

    bool finished_ = true;
};

} /* namespace pd */
} /* namespace csstv */

#endif /* CSSTV_MODES_PD_H */
