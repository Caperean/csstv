/*
 * CSSTV Example Program with Automatic Mode Selection
 * 
 * This example demonstrates the new automatic mode selection feature:
 * - Encoder: Automatically selects mode based on image dimensions
 * - Decoder: Automatically detects mode from image dimensions (or VIS header)
 * 
 * No need to specify CSSTV_MODE_PD50 manually anymore!
 */

#include "csstv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper function to create a test image */
void create_test_image(csstv_image_t *image, uint16_t width, uint16_t height, uint8_t r, uint8_t g, uint8_t b)
{
    static uint8_t buffer[800 * 616 * 3]; /* Max PD290 size */
    
    image->width = width;
    image->height = height;
    image->format = CSSTV_PIXEL_RGB888;
    image->stride = width * 3;
    image->data = buffer;
    
    /* Fill with solid color */
    for (size_t i = 0; i < width * height; i++) {
        buffer[i * 3 + 0] = r;
        buffer[i * 3 + 1] = g;
        buffer[i * 3 + 2] = b;
    }
}

int main(void)
{
    printf("CSSTV Example with Automatic Mode Selection\n");
    printf("=============================================\n\n");
    
    /* ==================== */
    /* Encoder with AUTO     */
    /* ==================== */
    printf("Step 1: Encoding with automatic mode selection\n");
    printf("------------------------------------------------\n");
    
    /* Initialize encoder with AUTO mode - no need to specify PD50 */
    csstv_encoder_t encoder;
    csstv_status_t status = csstv_encoder_init_auto(&encoder, 48000);
    
    if (status != CSSTV_OK) {
        printf("Failed to initialize encoder: %d\n", status);
        return 1;
    }
    printf("Encoder initialized with AUTO mode\n");
    
    /* Create a 320x256 image (PD50 dimensions) */
    csstv_image_t source_image;
    create_test_image(&source_image, 320, 256, 255, 0, 0);
    printf("Created test image: %dx%d RGB888 (solid red)\n", 
           source_image.width, source_image.height);
    
    /* Set image - mode will be auto-detected as PD50 */
    status = csstv_encoder_set_image(&encoder, &source_image);
    if (status != CSSTV_OK) {
        printf("Failed to set image: %d\n", status);
        csstv_encoder_deinit(&encoder);
        return 1;
    }
    printf("Image set - mode automatically detected\n");
    
    /* Get the detected mode info */
    csstv_mode_info_t mode_info;
    csstv_mode_get_info(CSSTV_MODE_PD50, &mode_info);
    printf("Using mode: %dx%d, duration: %u ms\n", 
           mode_info.width, mode_info.height, mode_info.duration_ms);
    
    /* Generate audio samples */
    const uint32_t total_samples = (mode_info.duration_ms * 48000) / 1000;
    csstv_sample_t *audio_buffer = (csstv_sample_t *)malloc(total_samples * sizeof(csstv_sample_t));
    
    size_t samples_written = 0;
    status = csstv_encoder_read(&encoder, audio_buffer, total_samples, &samples_written);
    
    if (status != CSSTV_OK) {
        printf("Failed to read samples: %d\n", status);
        free(audio_buffer);
        csstv_encoder_deinit(&encoder);
        return 1;
    }
    
    printf("Generated %zu audio samples\n\n", samples_written);
    csstv_encoder_deinit(&encoder);
    
    /* ==================== */
    /* Decoder with AUTO     */
    /* ==================== */
    printf("Step 2: Decoding with automatic mode selection\n");
    printf("------------------------------------------------\n");
    
    /* Initialize decoder with AUTO mode */
    csstv_decoder_t decoder;
    status = csstv_decoder_init_auto(&decoder, 48000);
    
    if (status != CSSTV_OK) {
        printf("Failed to initialize decoder: %d\n", status);
        free(audio_buffer);
        return 1;
    }
    printf("Decoder initialized with AUTO mode\n");
    
    /* Create output image buffer */
    static uint8_t output_buffer[800 * 616 * 3];
    csstv_image_t output_image;
    output_image.width = 320;
    output_image.height = 256;
    output_image.format = CSSTV_PIXEL_RGB888;
    output_image.stride = 320 * 3;
    output_image.data = output_buffer;
    
    /* Set output image - mode will be auto-detected from dimensions */
    status = csstv_decoder_set_image(&decoder, &output_image);
    if (status != CSSTV_OK) {
        printf("Failed to set output image: %d\n", status);
        csstv_decoder_deinit(&decoder);
        free(audio_buffer);
        return 1;
    }
    printf("Output image set - mode automatically detected\n");
    
    /* Get the detected mode */
    csstv_mode_t detected_mode = csstv_decoder_get_detected_mode(&decoder);
    printf("Detected mode: 0x%04X\n", detected_mode);
    
    /* Feed audio samples */
    size_t samples_consumed = 0;
    status = csstv_decoder_write(&decoder, audio_buffer, samples_written, &samples_consumed);
    
    if (status != CSSTV_OK) {
        printf("Failed to write samples: %d\n", status);
        csstv_decoder_deinit(&decoder);
        free(audio_buffer);
        return 1;
    }
    
    printf("Fed %zu samples to decoder\n", samples_consumed);
    
    if (csstv_decoder_finished(&decoder)) {
        printf("Decoding completed successfully\n");
    }
    
    /* Get decoded image */
    csstv_image_t decoded_image;
    status = csstv_decoder_get_image(&decoder, &decoded_image);
    
    if (status != CSSTV_OK) {
        printf("Failed to get decoded image: %d\n", status);
        csstv_decoder_deinit(&decoder);
        free(audio_buffer);
        return 1;
    }
    
    printf("Retrieved decoded image: %dx%d\n\n", decoded_image.width, decoded_image.height);
    csstv_decoder_deinit(&decoder);
    
    /* Cleanup */
    free(audio_buffer);
    
    printf("Example completed successfully!\n");
    printf("\nKey points:\n");
    printf("- No need to specify CSSTV_MODE_PD50 in init()\n");
    printf("- Encoder auto-selects mode from image dimensions\n");
    printf("- Decoder auto-detects mode from image dimensions\n");
    printf("- Makes the API simpler and more user-friendly\n");
    
    return 0;
}
