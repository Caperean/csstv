#include "csstv_txrx_driver.h"

namespace csstv {

/*
 * Platform-specific factory function implementations
 * These are compiled conditionally based on architecture
 */

#if defined(__AVR__)
#include "platform/avr/txrx_pwm_avr.cpp"
#include "platform/avr/txrx_adc_avr.cpp"
#elif defined(__arm__)
#include "platform/arm/txrx_pwm_arm.cpp"
#include "platform/arm/txrx_adc_arm.cpp"
#elif defined(__riscv)
#include "platform/riscv/txrx_pwm_riscv.cpp"
#include "platform/riscv/txrx_adc_riscv.cpp"
#else
/* Default stub implementations for x86 and other platforms */
TxRxDriver *create_pwm_tx_driver(void *driver_state, size_t driver_state_size)
{
    (void)driver_state;
    (void)driver_state_size;
    return nullptr;
}

TxRxDriver *create_adc_rx_driver(void *driver_state, size_t driver_state_size)
{
    (void)driver_state;
    (void)driver_state_size;
    return nullptr;
}
#endif

/*
 * I2S driver stub (not yet implemented for any platform)
 */
TxRxDriver *create_i2s_txrx_driver(void *driver_state, size_t driver_state_size)
{
    (void)driver_state;
    (void)driver_state_size;
    return nullptr;
}

} /* namespace csstv */
