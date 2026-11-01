#ifndef AA55_FRAME_H
#define AA55_FRAME_H

/*
 * AA55 binary frame codec
 *
 * Frame layout:
 *   [0xAA] [0x55] [LEN] [CMD] [PAYLOAD...] [CRC8]
 *
 * LEN is the payload size in bytes. CRC8 covers LEN, CMD, and PAYLOAD.
 * CRC parameters: polynomial 0x07, initial value 0x00, no reflection,
 * no final XOR.
 *
 * This header-only C99 library uses no dynamic memory allocation.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AA55_FRAME_HEADER_0       0xAAU
#define AA55_FRAME_HEADER_1       0x55U
#define AA55_FRAME_OVERHEAD       5U
#define AA55_FRAME_MAX_PAYLOAD    32U
#define AA55_FRAME_MAX_SIZE       (AA55_FRAME_MAX_PAYLOAD + AA55_FRAME_OVERHEAD)

typedef enum
{
    AA55_STATUS_OK = 0,
    AA55_STATUS_INVALID_ARGUMENT = -1,
    AA55_STATUS_PAYLOAD_TOO_LARGE = -2,
    AA55_STATUS_BUFFER_TOO_SMALL = -3
} aa55_status_t;

typedef enum
{
    AA55_RX_INVALID_ARGUMENT = -1,
    AA55_RX_INCOMPLETE = 0,
    AA55_RX_FRAME_READY = 1,
    AA55_RX_CRC_ERROR = 2,
    AA55_RX_LENGTH_ERROR = 3
} aa55_rx_result_t;

/* Update a CRC8 value with one byte. */
static inline uint8_t aa55_crc8_update(uint8_t crc, uint8_t byte)
{
    uint8_t i;

    crc ^= byte;
    for (i = 0; i < 8U; ++i)
    {
        crc = (crc & 0x80U) != 0U
                  ? (uint8_t)((uint8_t)(crc << 1) ^ 0x07U)
                  : (uint8_t)(crc << 1);
    }
    return crc;
}

/*
 * Calculate CRC8 over data. data must be valid when length is nonzero.
 * length is size_t so that sizeof() arguments are not silently truncated.
 */
static inline uint8_t aa55_crc8(const uint8_t *data, size_t length)
{
    uint8_t crc = 0U;
    size_t i;

    for (i = 0; i < length; ++i)
    {
        crc = aa55_crc8_update(crc, data[i]);
    }
    return crc;
}

/*
 * Encode one frame.
 *
 * out_capacity is the writable size of out. frame_size receives the encoded
 * size on success and is set to zero on failure. payload may be NULL only
 * when payload_length is zero. out and payload must not overlap.
 */
static inline aa55_status_t aa55_encode(uint8_t *out,
                                        size_t out_capacity,
                                        uint8_t command,
                                        const uint8_t *payload,
                                        uint8_t payload_length,
                                        size_t *frame_size)
{
    size_t required;
    uint8_t i;

    if (frame_size == NULL)
    {
        return AA55_STATUS_INVALID_ARGUMENT;
    }
    *frame_size = 0U;

    if ((out == NULL) || ((payload_length > 0U) && (payload == NULL)))
    {
        return AA55_STATUS_INVALID_ARGUMENT;
    }
    if (payload_length > AA55_FRAME_MAX_PAYLOAD)
    {
        return AA55_STATUS_PAYLOAD_TOO_LARGE;
    }

    required = (size_t)payload_length + AA55_FRAME_OVERHEAD;
    if (out_capacity < required)
    {
        return AA55_STATUS_BUFFER_TOO_SMALL;
    }

    out[0] = AA55_FRAME_HEADER_0;
    out[1] = AA55_FRAME_HEADER_1;
    out[2] = payload_length;
    out[3] = command;
    for (i = 0; i < payload_length; ++i)
    {
        out[4U + i] = payload[i];
    }
    out[4U + payload_length] =
        aa55_crc8(&out[2], (size_t)payload_length + 2U);

    *frame_size = required;
    return AA55_STATUS_OK;
}

/*
 * Streaming receiver state. Allocate one instance per input stream and call
 * aa55_rx_init before first use. command, payload_length, and payload are
 * valid after AA55_RX_FRAME_READY until the next aa55_rx_feed call.
 */
typedef struct
{
    uint8_t state;
    uint8_t payload_length;
    uint8_t command;
    uint8_t crc;
    uint8_t index;
    uint8_t payload[AA55_FRAME_MAX_PAYLOAD];
} aa55_rx_t;

static inline void aa55_rx_init(aa55_rx_t *rx)
{
    if (rx != NULL)
    {
        rx->state = 0U;
        rx->payload_length = 0U;
        rx->command = 0U;
        rx->crc = 0U;
        rx->index = 0U;
    }
}

/* Discard a partial frame and start looking for a new header. */
static inline void aa55_rx_reset(aa55_rx_t *rx)
{
    if (rx != NULL)
    {
        rx->state = 0U;
    }
}

/*
 * Feed one received byte to the decoder.
 *
 * AA55_RX_FRAME_READY: a complete, valid frame is available in rx.
 * AA55_RX_INCOMPLETE: more bytes are needed.
 * AA55_RX_CRC_ERROR / AA55_RX_LENGTH_ERROR: the frame was discarded and the
 * decoder has already started resynchronizing.
 */
static inline aa55_rx_result_t aa55_rx_feed(aa55_rx_t *rx, uint8_t byte)
{
    if (rx == NULL)
    {
        return AA55_RX_INVALID_ARGUMENT;
    }

    switch (rx->state)
    {
    case 0U: /* Wait for 0xAA. */
        if (byte == AA55_FRAME_HEADER_0)
        {
            rx->state = 1U;
        }
        return AA55_RX_INCOMPLETE;

    case 1U: /* Wait for 0x55; preserve repeated 0xAA bytes. */
        if (byte == AA55_FRAME_HEADER_1)
        {
            rx->state = 2U;
        }
        else if (byte != AA55_FRAME_HEADER_0)
        {
            rx->state = 0U;
        }
        return AA55_RX_INCOMPLETE;

    case 2U: /* Payload length. */
        if (byte > AA55_FRAME_MAX_PAYLOAD)
        {
            rx->state = (byte == AA55_FRAME_HEADER_0) ? 1U : 0U;
            return AA55_RX_LENGTH_ERROR;
        }
        rx->payload_length = byte;
        rx->crc = aa55_crc8_update(0U, byte);
        rx->index = 0U;
        rx->state = 3U;
        return AA55_RX_INCOMPLETE;

    case 3U: /* Command. */
        rx->command = byte;
        rx->crc = aa55_crc8_update(rx->crc, byte);
        rx->state = (rx->payload_length == 0U) ? 5U : 4U;
        return AA55_RX_INCOMPLETE;

    case 4U: /* Payload. */
        rx->payload[rx->index++] = byte;
        rx->crc = aa55_crc8_update(rx->crc, byte);
        if (rx->index >= rx->payload_length)
        {
            rx->state = 5U;
        }
        return AA55_RX_INCOMPLETE;

    default: /* CRC. */
        rx->state = (byte == AA55_FRAME_HEADER_0) ? 1U : 0U;
        return (byte == rx->crc) ? AA55_RX_FRAME_READY : AA55_RX_CRC_ERROR;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* AA55_FRAME_H */
