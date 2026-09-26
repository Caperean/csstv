/*
 * CSSTV Example Program
 * 
 * This example demonstrates how to use the CSSTV library to:
 * 1. Encode an RGB image to SSTV audio samples
 * 2. Decode SSTV audio samples back to an RGB image
 * 
 * Compile with:
 *   g++ -std=c++17 -Iinclude -Isrc/internal -Isrc/modes example.cpp src/*.cpp -o example
 * 
 * Or with CMake:
 *   cmake -S . -B build
 *   cmake --build build
 */

#include "csstv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper function to create a simple test image */
void create_test_image(csstv_image_t *image, uint8_t r, uint8_t g, uint8_t b)
{
    /* Create a 320x256 solid color image (PD50 dimensions) */
    static uint8_t buffer[320 * 256 * 3];
    
    image->width = 320;
    image->height = 256;
    image->format = CSSTV_PIXEL_RGB888;
    image->stride = 320 * 3;
    image->data = buffer;
    
    /* Fill with solid color */
    for (size_t i = 0; i < image->width * image->height; i++) {
        buffer[i * 3 + 0] = r;
        buffer[i * 3 + 1] = g;
        buffer[i * 3 + 2] = b;
    }
}

/* Helper function to compare two images */
int compare_images(const csstv_image_t *img1, const csstv_image_t *img2)
{
    if (img1->width != img2->width || img1->height != img2->height) {
        printf("Image dimensions don't match\n");
        return 0;
    }
    
    uint32_t diff_count = 0;
    for (size_t y = 0; y < img1->height; y++) {
        for (size_t x = 0; x < img1->width; x++) {
            const uint8_t *p1 = img1->data + y * img1->stride + x * 3;
            const uint8_t *p2 = img2->data + y * img2->stride + x * 3;
            
            for (int c = 0; c < 3; c++) {
                int diff = abs((int)p1[c] - (int)p2[c]);
                if (diff > 5) { /* Allow small quantization differences */
                    diff_count++;
                }
            }
        }
    }
    
    printf("Pixel differences: %u / %zu\n", diff_count, img1->width * img1->height);
    return diff_count < (img1->width * img1->height) / 10; /* Allow up to 10% differences */
}

int main(void)
{
    printf("CSSTV Example Program\n");
    printf("=====================\n\n");
    
    /* ==================== */
    /* Encoder Example      */
    /* ==================== */
    printf("Step 1: Encoding image to SSTV audio\n");
    printf("--------------------------------------\n");
    
    /* Initialize encoder */
    csstv_encoder_t encoder;
    csstv_status_t status = csstv_encoder_init(&encoder, CSSTV_MODE_PD50, 48000);
    
    if (status != CSSTV_OK) {
        printf("Failed to initialize encoder: %d\n", status);
        return 1;
    }
    printf("Encoder initialized for PD50 mode at 48000 Hz\n");
    
    /* Create and set test image (solid red) */
    csstv_image_t source_image;
    create_test_image(&source_image, 255, 0, 0);
    printf("Created test image: %dx%d RGB888 (solid red)\n", 
           source_image.width, source_image.height);
    
    status = csstv_encoder_set_image(&encoder, &source_image);
    if (status != CSSTV_OK) {
        printf("Failed to set image: %d\n", status);
        csstv_encoder_deinit(&encoder);
        return 1;
    }
    printf("Image set for encoding\n");
    
    /* Calculate total samples needed */
    csstv_mode_info_t mode_info;
    csstv_mode_get_info(CSSTV_MODE_PD50, &mode_info);
    printf("Expected duration: %u ms\n", mode_info.duration_ms);
    
    const uint32_t total_samples = (mode_info.duration_ms * 48000) / 1000;
    printf("Total samples to generate: %u\n", total_samples);
    
    /* Allocate buffer for audio samples */
    csstv_sample_t *audio_buffer = (csstv_sample_t *)malloc(total_samples * sizeof(csstv_sample_t));
    if (!audio_buffer) {
        printf("Failed to allocate audio buffer\n");
        csstv_encoder_deinit(&encoder);
        return 1;
    }
    
    /* Generate audio samples */
    size_t samples_written = 0;
    status = csstv_encoder_read(&encoder, audio_buffer, total_samples, &samples_written);
    
    if (status != CSSTV_OK) {
        printf("Failed to read samples: %d\n", status);
        free(audio_buffer);
        csstv_encoder_deinit(&encoder);
        return 1;
    }
    
    printf("Generated %zu audio samples\n", samples_written);
    printf("Sample range: [%d, %d]\n", 
           audio_buffer[0], audio_buffer[samples_written - 1]);
    
    /* Cleanup encoder */
    csstv_encoder_deinit(&encoder);
    printf("Encoder cleaned up\n\n");
    
    /* ==================== */
    /* Decoder Example      */
    /* ==================== */
    printf("Step 2: Decoding SSTV audio back to image\n");
    printf("-------------------------------------------\n");
    
    /* Initialize decoder */
    csstv_decoder_t decoder;
    status = csstv_decoder_init(&decoder, CSSTV_MODE_PD50, 48000);
    
    if (status != CSSTV_OK) {
        printf("Failed to initialize decoder: %d\n", status);
        free(audio_buffer);
        return 1;
    }
    printf("Decoder initialized for PD50 mode at 48000 Hz\n");
    
    /* Create output image buffer */
    static uint8_t output_buffer[320 * 256 * 3];
    csstv_image_t output_image;
    output_image.width = 320;
    output_image.height = 256;
    output_image.format = CSSTV_PIXEL_RGB888;
    output_image.stride = 320 * 3;
    output_image.data = output_buffer;
    
    status = csstv_decoder_set_image(&decoder, &output_image);
    if (status != CSSTV_OK) {
        printf("Failed to set output image: %d\n", status);
        csstv_decoder_deinit(&decoder);
        free(audio_buffer);
        return 1;
    }
    printf("Output image buffer set\n");
    
    /* Feed audio samples to decoder */
    size_t samples_consumed = 0;
    status = csstv_decoder_write(&decoder, audio_buffer, samples_written, &samples_consumed);
    
    if (status != CSSTV_OK) {
        printf("Failed to write samples: %d\n", status);
        csstv_decoder_deinit(&decoder);
        free(audio_buffer);
        return 1;
    }
    
    printf("Fed %zu samples to decoder\n", samples_consumed);
    
    /* Check if decoding is complete */
    if (!csstv_decoder_finished(&decoder)) {
        printf("Warning: Decoder not finished\n");
    } else {
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
    
    printf("Retrieved decoded image: %dx%d\n", decoded_image.width, decoded_image.height);
    
    /* Cleanup decoder */
    csstv_decoder_deinit(&decoder);
    printf("Decoder cleaned up\n\n");
    
    /* ==================== */
    /* Verification         */
    /* ==================== */
    printf("Step 3: Verifying roundtrip\n");
    printf("----------------------------\n");
    
    if (compare_images(&source_image, &decoded_image)) {
        printf("✓ Roundtrip successful: images match within tolerance\n");
    } else {
        printf("✗ Roundtrip failed: images differ significantly\n");
    }
    
    /* Cleanup */
    free(audio_buffer);
    
    printf("\nExample completed successfully!\n");
    return 0;
}
