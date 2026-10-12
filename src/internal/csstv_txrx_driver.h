#ifndef CSSTV_TXRX_DRIVER_H
#define CSSTV_TXRX_DRIVER_H

#include "csstv.h"
#include <stddef.h>
#include <stdint.h>

namespace csstv {

/*
 * TX/RX Driver Abstract Interface
 *
 * This defines the hardware abstraction layer for audio input/output
 * on embedded systems. The library provides platform-specific
 * implementations for common embedded peripherals (PWM, ADC, I2S).
 *
 * Users can also provide custom implementations for their specific
 * hardware by implementing this interface.
 */

enum class TxRxDirection : uint8_t
{
    kTx = 0,    /* Transmit (output) */
    kRx = 1     /* Receive (input) */
};

/*
 * Abstract TX/RX driver interface.
 * Platform-specific implementations inherit from this.
 */
class TxRxDriver
{
public:
    virtual ~TxRxDriver() = default;
    
    /*
     * Initialize the TX/RX driver.
     * direction: TX or RX
     * sample_rate: Audio sample rate (e.g., 8000, 16000, 48000 Hz)
     * buffer_size: Number of samples in the buffer
     */
    virtual csstv_status_t init(
        TxRxDirection direction,
        uint32_t sample_rate,
        uint32_t buffer_size) = 0;
    
    /*
     * Start the TX/RX operation.
     * For TX: begins outputting audio samples
     * For RX: begins capturing audio samples
     */
    virtual csstv_status_t start() = 0;
    
    /*
     * Stop the TX/RX operation.
     */
    virtual csstv_status_t stop() = 0;
    
    /*
     * Write samples for TX (only valid for TX direction).
     * samples: pointer to audio samples
     * count: number of samples to write
     * written: number of samples actually written
     */
    virtual csstv_status_t write(
        const csstv_sample_t *samples,
        size_t count,
        size_t *written) = 0;
    
    /*
     * Read samples for RX (only valid for RX direction).
     * samples: pointer to buffer for audio samples
     * capacity: buffer capacity
     * consumed: number of samples actually read
     */
    virtual csstv_status_t read(
        csstv_sample_t *samples,
        size_t capacity,
        size_t *consumed) = 0;
    
    /*
     * Check if the driver is currently running.
     */
    virtual bool is_running() const = 0;
    
    /*
     * Get the number of samples available (RX) or space available (TX).
     */
    virtual size_t available() const = 0;
    
    /*
     * Cleanup and release resources.
     */
    virtual void deinit() = 0;
};

/*
 * TX/RX driver state. This is placement-constructed into
 * csstv_txrx_t::storage for zero dynamic allocation.
 */
struct TxRxState
{
    TxRxDirection direction;
    uint32_t sample_rate;
    uint32_t buffer_size;
    bool initialized;
    bool running;
    
    /* Platform-specific driver implementation */
    TxRxDriver *driver;
    uint8_t driver_state[256];  /* Driver-specific state */
};

/*
 * Platform-specific driver factory functions.
 * These are implemented per-platform (AVR, ARM, RISC-V, etc.).
 */

/*
 * Create a PWM-based TX driver.
 * This uses a timer with PWM output to generate audio.
 * Available on: AVR, ARM, RISC-V (most MCUs)
 */
TxRxDriver *create_pwm_tx_driver(
    void *driver_state,
    size_t driver_state_size);

/*
 * Create an ADC-based RX driver.
 * This uses an ADC to sample audio input.
 * Available on: AVR, ARM, RISC-V (most MCUs)
 */
TxRxDriver *create_adc_rx_driver(
    void *driver_state,
    size_t driver_state_size);

/*
 * Create an I2S-based TX/RX driver.
 * This uses I2S audio interface for high-quality audio.
 * Available on: ARM, RISC-V (advanced MCUs)
 */
TxRxDriver *create_i2s_txrx_driver(
    void *driver_state,
    size_t driver_state_size);

} /* namespace csstv */

#endif /* CSSTV_TXRX_DRIVER_H */
