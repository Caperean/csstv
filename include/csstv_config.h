#ifndef CSSTV_CONFIG_H
#define CSSTV_CONFIG_H

/*
 * ============================================================================
 * Core
 * ============================================================================
 */

#ifndef CSSTV_ENABLE_ENCODER
#define CSSTV_ENABLE_ENCODER 0
#endif

#ifndef CSSTV_ENABLE_DECODER
#define CSSTV_ENABLE_DECODER 0
#endif


/*
 * ============================================================================
 * Encoder modes
 * ============================================================================
 */

#ifndef CSSTV_ENCODER_MODE_PD50
#define CSSTV_ENCODER_MODE_PD50 0
#endif

#ifndef CSSTV_ENCODER_MODE_PD90
#define CSSTV_ENCODER_MODE_PD90 0
#endif

#ifndef CSSTV_ENCODER_MODE_PD120
#define CSSTV_ENCODER_MODE_PD120 0
#endif

#ifndef CSSTV_ENCODER_MODE_PD160
#define CSSTV_ENCODER_MODE_PD160 0
#endif

#ifndef CSSTV_ENCODER_MODE_PD180
#define CSSTV_ENCODER_MODE_PD180 0
#endif

#ifndef CSSTV_ENCODER_MODE_PD240
#define CSSTV_ENCODER_MODE_PD240 0
#endif

#ifndef CSSTV_ENCODER_MODE_PD290
#define CSSTV_ENCODER_MODE_PD290 0
#endif


/*
 * ============================================================================
 * Decoder modes
 *
 * Decoder support is not implemented yet.
 * These options are reserved for future decoder implementations.
 * ============================================================================
 */
/*
#ifndef CSSTV_DECODER_MODE_PD50
#define CSSTV_DECODER_MODE_PD50 0
#endif

#ifndef CSSTV_DECODER_MODE_PD90
#define CSSTV_DECODER_MODE_PD90 0
#endif

#ifndef CSSTV_DECODER_MODE_PD120
#define CSSTV_DECODER_MODE_PD120 0
#endif

#ifndef CSSTV_DECODER_MODE_PD160
#define CSSTV_DECODER_MODE_PD160 0
#endif

#ifndef CSSTV_DECODER_MODE_PD180
#define CSSTV_DECODER_MODE_PD180 0
#endif

#ifndef CSSTV_DECODER_MODE_PD240
#define CSSTV_DECODER_MODE_PD240 0
#endif

#ifndef CSSTV_DECODER_MODE_PD290
#define CSSTV_DECODER_MODE_PD290 0
#endif

 */
/*
 * ============================================================================
 * Static encoder storage
 * ============================================================================
 */

#ifndef CSSTV_ENCODER_STORAGE_SIZE
#define CSSTV_ENCODER_STORAGE_SIZE 512U
#endif


/*
 * ============================================================================
 * Debug
 * ============================================================================
 */

#ifndef CSSTV_ENABLE_ASSERT
#define CSSTV_ENABLE_ASSERT 0
#endif

#ifndef CSSTV_ENABLE_DEBUG
#define CSSTV_ENABLE_DEBUG 0
#endif


#endif /* CSSTV_CONFIG_H */

