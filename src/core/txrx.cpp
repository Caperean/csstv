#include "csstv.h"

#if CSSTV_ENABLE_TXRX

#include "csstv_internal.h"
#include "csstv_txrx_driver.h"

#include <string.h>

namespace csstv {

TxRxState *txrx_state(csstv_txrx_t *txrx)
{
    return reinterpret_cast<TxRxState *>(txrx->storage);
}

const TxRxState *txrx_state(const csstv_txrx_t *txrx)
{
    return reinterpret_cast<const TxRxState *>(txrx->storage);
}

/* Platform-specific factory function declarations */
#if defined(__AVR__)
extern TxRxDriver *create_pwm_tx_driver(void *driver_state, size_t driver_state_size);
extern TxRxDriver *create_adc_rx_driver(void *driver_state, size_t driver_state_size);
#elif defined(__arm__)
extern TxRxDriver *create_pwm_tx_driver(void *driver_state, size_t driver_state_size);
extern TxRxDriver *create_adc_rx_driver(void *driver_state, size_t driver_state_size);
#elif defined(__riscv)
extern TxRxDriver *create_pwm_tx_driver(void *driver_state, size_t driver_state_size);
extern TxRxDriver *create_adc_rx_driver(void *driver_state, size_t driver_state_size);
#else
extern TxRxDriver *create_pwm_tx_driver(void *driver_state, size_t driver_state_size);
extern TxRxDriver *create_adc_rx_driver(void *driver_state, size_t driver_state_size);
#endif

extern TxRxDriver *create_i2s_txrx_driver(void *driver_state, size_t driver_state_size);

} /* namespace csstv */

extern "C" {

csstv_status_t csstv_txrx_init(
    csstv_txrx_t *txrx,
    csstv_txrx_driver_type_t driver_type,
    int direction,
    uint32_t sample_rate,
    uint32_t buffer_size)
{
    using csstv::TxRxState;
    using csstv::TxRxDirection;
    using csstv::TxRxDriver;

    if (txrx == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    if (sample_rate == 0U || buffer_size == 0U)
    {
        return CSSTV_ERROR_INVALID_SAMPLE_RATE;
    }

    if (direction != 0 && direction != 1)
    {
        return CSSTV_ERROR_INVALID_IMAGE;
    }

    TxRxState *state = csstv::txrx_state(txrx);

    memset(state, 0, sizeof(TxRxState));

    state->direction = (direction == 0) ? TxRxDirection::kTx : TxRxDirection::kRx;
    state->sample_rate = sample_rate;
    state->buffer_size = buffer_size;
    state->initialized = false;
    state->running = false;

    /* Create platform-specific driver based on type */
    switch (driver_type)
    {
        case CSSTV_TXRX_DRIVER_PWM:
            if (state->direction == TxRxDirection::kTx)
            {
                state->driver = csstv::create_pwm_tx_driver(
                    state->driver_state,
                    sizeof(state->driver_state));
            }
            else
            {
                return CSSTV_ERROR_UNSUPPORTED_MODE;
            }
            break;

        case CSSTV_TXRX_DRIVER_ADC:
            if (state->direction == TxRxDirection::kRx)
            {
                state->driver = csstv::create_adc_rx_driver(
                    state->driver_state,
                    sizeof(state->driver_state));
            }
            else
            {
                return CSSTV_ERROR_UNSUPPORTED_MODE;
            }
            break;

        case CSSTV_TXRX_DRIVER_I2S:
            state->driver = csstv::create_i2s_txrx_driver(
                state->driver_state,
                sizeof(state->driver_state));
            break;

        case CSSTV_TXRX_DRIVER_CUSTOM:
            /* User must provide custom driver implementation */
            state->driver = NULL;
            break;

        default:
            return CSSTV_ERROR_UNSUPPORTED_MODE;
    }

    if (state->driver == NULL && driver_type != CSSTV_TXRX_DRIVER_CUSTOM)
    {
        return CSSTV_ERROR_UNSUPPORTED_MODE;
    }

    if (state->driver != NULL)
    {
        const csstv_status_t init_status = state->driver->init(
            state->direction,
            sample_rate,
            buffer_size);

        if (init_status != CSSTV_OK)
        {
            state->driver->~TxRxDriver();
            state->driver = NULL;
            return init_status;
        }
    }

    state->initialized = true;

    return CSSTV_OK;
}

csstv_status_t csstv_txrx_start(csstv_txrx_t *txrx)
{
    using csstv::TxRxState;

    if (txrx == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    TxRxState *state = csstv::txrx_state(txrx);

    if (!state->initialized || state->driver == NULL)
    {
        return CSSTV_ERROR_NOT_INITIALIZED;
    }

    if (state->running)
    {
        return CSSTV_OK;
    }

    const csstv_status_t start_status = state->driver->start();

    if (start_status != CSSTV_OK)
    {
        return start_status;
    }

    state->running = true;

    return CSSTV_OK;
}

csstv_status_t csstv_txrx_stop(csstv_txrx_t *txrx)
{
    using csstv::TxRxState;

    if (txrx == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    TxRxState *state = csstv::txrx_state(txrx);

    if (!state->initialized || state->driver == NULL)
    {
        return CSSTV_ERROR_NOT_INITIALIZED;
    }

    if (!state->running)
    {
        return CSSTV_OK;
    }

    const csstv_status_t stop_status = state->driver->stop();

    if (stop_status != CSSTV_OK)
    {
        return stop_status;
    }

    state->running = false;

    return CSSTV_OK;
}

csstv_status_t csstv_txrx_write(
    csstv_txrx_t *txrx,
    const csstv_sample_t *samples,
    size_t count,
    size_t *written)
{
    using csstv::TxRxState;
    using csstv::TxRxDirection;

    if (written != NULL)
    {
        *written = 0U;
    }

    if (txrx == NULL || samples == NULL || written == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    TxRxState *state = csstv::txrx_state(txrx);

    if (!state->initialized || state->driver == NULL)
    {
        return CSSTV_ERROR_NOT_INITIALIZED;
    }

    if (state->direction != TxRxDirection::kTx)
    {
        return CSSTV_ERROR_NOT_READY;
    }

    if (!state->running)
    {
        return CSSTV_ERROR_NOT_READY;
    }

    return state->driver->write(samples, count, written);
}

csstv_status_t csstv_txrx_read(
    csstv_txrx_t *txrx,
    csstv_sample_t *samples,
    size_t capacity,
    size_t *consumed)
{
    using csstv::TxRxState;
    using csstv::TxRxDirection;

    if (consumed != NULL)
    {
        *consumed = 0U;
    }

    if (txrx == NULL || samples == NULL || consumed == NULL)
    {
        return CSSTV_ERROR_NULL;
    }

    TxRxState *state = csstv::txrx_state(txrx);

    if (!state->initialized || state->driver == NULL)
    {
        return CSSTV_ERROR_NOT_INITIALIZED;
    }

    if (state->direction != TxRxDirection::kRx)
    {
        return CSSTV_ERROR_NOT_READY;
    }

    if (!state->running)
    {
        return CSSTV_ERROR_NOT_READY;
    }

    return state->driver->read(samples, capacity, consumed);
}

bool csstv_txrx_is_running(const csstv_txrx_t *txrx)
{
    if (txrx == NULL)
    {
        return false;
    }

    const csstv::TxRxState *state = csstv::txrx_state(txrx);

    if (!state->initialized || state->driver == NULL)
    {
        return false;
    }

    return state->driver->is_running();
}

size_t csstv_txrx_available(const csstv_txrx_t *txrx)
{
    if (txrx == NULL)
    {
        return 0U;
    }

    const csstv::TxRxState *state = csstv::txrx_state(txrx);

    if (!state->initialized || state->driver == NULL)
    {
        return 0U;
    }

    return state->driver->available();
}

void csstv_txrx_deinit(csstv_txrx_t *txrx)
{
    using csstv::TxRxState;

    if (txrx == NULL)
    {
        return;
    }

    TxRxState *state = csstv::txrx_state(txrx);

    if (!state->initialized || state->driver == NULL)
    {
        return;
    }

    if (state->running)
    {
        state->driver->stop();
    }

    state->driver->deinit();
    state->driver->~TxRxDriver();
    state->driver = NULL;

    memset(state, 0, sizeof(TxRxState));
}

} /* extern "C" */

#endif /* CSSTV_ENABLE_TXRX */
