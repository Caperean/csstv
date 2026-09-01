```c
#ifndef CSSTV_CONFIG_H
#define CSSTV_CONFIG_H


/* ========================================================================== */
/* Core                                                                       */
/* ========================================================================== */


#ifndef CSSTV_ENABLE_ENCODER
#define CSSTV_ENABLE_ENCODER 1
#endif

/*for future versions
#ifndef CSSTV_ENABLE_DECODER
#define CSSTV_ENABLE_DECODER 0
#endif
*/
/* ========================================================================== */
/* PD modes                                                                   */
/* ========================================================================== */


#ifndef CSSTV_ENABLE_PD50
#define CSSTV_ENABLE_PD50 1
#endif

#ifndef CSSTV_ENABLE_PD90
#define CSSTV_ENABLE_PD90 1
#endif

#ifndef CSSTV_ENABLE_PD120
#define CSSTV_ENABLE_PD120 1
#endif

#ifndef CSSTV_ENABLE_PD160
#define CSSTV_ENABLE_PD160 1
#endif

#ifndef CSSTV_ENABLE_PD180
#define CSSTV_ENABLE_PD180 1
#endif

#ifndef CSSTV_ENABLE_PD240
#define CSSTV_ENABLE_PD240 1
#endif

#ifndef CSSTV_ENABLE_PD290
#define CSSTV_ENABLE_PD290 1
#endif

/* ========================================================================== */
/* Encoder storage                                                            */
/* ========================================================================== */


#ifndef CSSTV_ENCODER_STORAGE_SIZE
#define CSSTV_ENCODER_STORAGE_SIZE 256U
#endif

/* ========================================================================== */
/* Debug                                                                      */
/* ========================================================================== */

#ifndef CSSTV_ENABLE_ASSERT
#define CSSTV_ENABLE_ASSERT 0
#endif

#ifndef CSSTV_ENABLE_DEBUG
#define CSSTV_ENABLE_DEBUG 0
#endif

#endif /* CSSTV_CONFIG_H */
```
