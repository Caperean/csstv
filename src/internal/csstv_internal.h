#ifndef CSSTV_INTERNAL_H
#define CSSTV_INTERNAL_H

#include "csstv.h"
#include "csstv_mode_driver.h"

#include <cstddef>
#include <cstdint>

namespace csstv {

/*
 * Encoder lifecycle:
 *
 *   kUninitialized --init()--> kReady --set_image()--> kEncoding
 *                                            ^              |
 *                                            |  reset()     | driver finishes
 *                                            +--------------+
 *                                                           v
 *                                                      kFinished
 *
 * read() is a no-op (returns 0 samples, CSSTV_OK) once kFinished is
 * reached; reset() re-arms the current image without requiring the
 * caller to call set_image() again.
 */
enum class EncoderStage : uint8_t
{
    kUninitialized = 0,
    kReady,
    kEncoding,
    kFinished
};

/*
 * Internal encoder state. This is placement-constructed (via plain
 * memset + field assignment -- it is a trivial, standard-layout type
 * on purpose) inside csstv_encoder_t::storage.
 *
 * `driver_state` is a small inline buffer that holds the polymorphic
 * ModeDriver instance for whichever mode was selected at init time.
 * The driver itself *is* a real C++ object with a vtable and must be
 * placement-constructed / explicitly destroyed -- unlike EncoderState,
 * which is just plain data.
 */
struct EncoderState
{
    csstv_mode_t mode;
    uint32_t sample_rate;

    ModeDriver *driver;

    csstv_image_t image;
    bool has_image;

    EncoderStage stage;

    static constexpr size_t kDriverStateSize = 384U;
    alignas(alignof(std::max_align_t)) uint8_t driver_state[kDriverStateSize];
};

static_assert(sizeof(EncoderState) <= CSSTV_ENCODER_STORAGE_SIZE,
              "csstv::EncoderState does not fit in CSSTV_ENCODER_STORAGE_SIZE bytes -- "
              "increase CSSTV_ENCODER_STORAGE_SIZE in csstv_config.h");

static_assert(alignof(csstv_encoder_alignment_t) >= alignof(EncoderState),
              "csstv_encoder_alignment_t does not provide sufficient alignment for "
              "csstv::EncoderState");

/* Reinterpret an encoder handle's storage as the internal state. */
EncoderState *encoder_state(csstv_encoder_t *encoder);
const EncoderState *encoder_state(const csstv_encoder_t *encoder);

} /* namespace csstv */

#endif /* CSSTV_INTERNAL_H */
