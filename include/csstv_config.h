#ifndef CSSTV_CONFIG_H
#define CSSTV_CONFIG_H

/*
 * ============================================================================
 * Core
 * ============================================================================
 */

#ifndef CSSTV_ENABLE_ENCODER
#define CSSTV_ENABLE_ENCODER 1
#endif

#ifndef CSSTV_ENABLE_DECODER
#define CSSTV_ENABLE_DECODER 1
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
 * ============================================================================
 */

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
 * Static decoder storage
 * ============================================================================
 */

#ifndef CSSTV_DECODER_STORAGE_SIZE
#define CSSTV_DECODER_STORAGE_SIZE 4096U
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

