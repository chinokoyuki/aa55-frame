#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "aa55_frame.h"

#define CHECK(expression)                                                     \
    do                                                                        \
    {                                                                         \
        if (!(expression))                                                    \
        {                                                                     \
            fprintf(stderr, "%s:%d: check failed: %s\n",                    \
                    __FILE__, __LINE__, #expression);                         \
            return 0;                                                         \
        }                                                                     \
    } while (0)

static const uint8_t k_golden[] = {
    0xAAU, 0x55U, 0x03U, 0x70U, 0x01U, 0x02U, 0x03U, 0xDCU
};
static const uint8_t k_empty_golden[] = {
    0xAAU, 0x55U, 0x00U, 0x42U, 0xC9U
};
static const uint8_t k_max_golden[] = {
    0xAAU, 0x55U, 0x20U, 0x10U,
    0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U,
    0x08U, 0x09U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU, 0x0FU,
    0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U,
    0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
    0x77U
};

static int decode_exact(const uint8_t *frame, size_t size,
                        uint8_t expected_command,
                        const uint8_t *expected_payload,
                        uint8_t expected_size)
{
    aa55_rx_t rx;
    size_t i;

    aa55_rx_init(&rx);
    for (i = 0U; i < size; ++i)
    {
        aa55_rx_result_t result = aa55_rx_feed(&rx, frame[i]);
        if (i + 1U < size)
        {
            CHECK(result == AA55_RX_INCOMPLETE);
        }
        else
        {
            CHECK(result == AA55_RX_FRAME_READY);
        }
    }

    CHECK(rx.command == expected_command);
    CHECK(rx.payload_length == expected_size);
    if (expected_size > 0U)
    {
        CHECK(memcmp(rx.payload, expected_payload, expected_size) == 0);
    }
    return 1;
}

static int test_crc8_check_vector(void)
{
    static const uint8_t input[] = "123456789";
    CHECK(aa55_crc8(input, 9U) == 0xF4U);
    CHECK(aa55_crc8_update(0U, 0U) == 0U);
    return 1;
}

static int test_golden_encode_decode(void)
{
    static const uint8_t payload[] = {0x01U, 0x02U, 0x03U};
    uint8_t out[AA55_FRAME_MAX_SIZE];
    size_t size = 0U;

    CHECK(aa55_encode(out, sizeof(out), 0x70U, payload, 3U, &size) == AA55_STATUS_OK);
    CHECK(size == sizeof(k_golden));
    CHECK(memcmp(out, k_golden, size) == 0);
    CHECK(decode_exact(k_golden, sizeof(k_golden), 0x70U, payload, 3U));
    return 1;
}

static int test_empty_payload(void)
{
    uint8_t out[AA55_FRAME_OVERHEAD];
    size_t size = 0U;

    CHECK(aa55_encode(out, sizeof(out), 0x42U, NULL, 0U, &size) == AA55_STATUS_OK);
    CHECK(size == sizeof(k_empty_golden));
    CHECK(memcmp(out, k_empty_golden, size) == 0);
    CHECK(decode_exact(k_empty_golden, sizeof(k_empty_golden), 0x42U, NULL, 0U));
    return 1;
}

static int test_maximum_payload(void)
{
    uint8_t payload[AA55_FRAME_MAX_PAYLOAD];
    uint8_t out[AA55_FRAME_MAX_SIZE];
    size_t size = 0U;
    uint8_t i;

    for (i = 0U; i < AA55_FRAME_MAX_PAYLOAD; ++i)
    {
        payload[i] = i;
    }
    CHECK(aa55_encode(out, sizeof(out), 0x10U, payload,
                      AA55_FRAME_MAX_PAYLOAD, &size) == AA55_STATUS_OK);
    CHECK(size == sizeof(k_max_golden));
    CHECK(memcmp(out, k_max_golden, size) == 0);
    CHECK(decode_exact(k_max_golden, sizeof(k_max_golden), 0x10U,
                       payload, AA55_FRAME_MAX_PAYLOAD));
    return 1;
}

static int test_encode_errors(void)
{
    uint8_t out[AA55_FRAME_MAX_SIZE];
    uint8_t payload[33] = {0U};
    size_t size = 99U;

    CHECK(aa55_encode(out, sizeof(out), 0U, NULL, 0U, NULL) ==
          AA55_STATUS_INVALID_ARGUMENT);
    CHECK(aa55_encode(NULL, sizeof(out), 0U, NULL, 0U, &size) ==
          AA55_STATUS_INVALID_ARGUMENT);
    CHECK(size == 0U);
    size = 99U;
    CHECK(aa55_encode(out, sizeof(out), 0U, NULL, 1U, &size) ==
          AA55_STATUS_INVALID_ARGUMENT);
    CHECK(size == 0U);
    size = 99U;
    CHECK(aa55_encode(out, sizeof(out), 0U, payload, 33U, &size) ==
          AA55_STATUS_PAYLOAD_TOO_LARGE);
    CHECK(size == 0U);
    size = 99U;
    CHECK(aa55_encode(out, 7U, 0U, payload, 3U, &size) ==
          AA55_STATUS_BUFFER_TOO_SMALL);
    CHECK(size == 0U);
    CHECK(aa55_rx_feed(NULL, 0U) == AA55_RX_INVALID_ARGUMENT);
    aa55_rx_init(NULL);
    aa55_rx_reset(NULL);
    return 1;
}

static int test_noise_before_frame(void)
{
    static const uint8_t noise[] = {'s', 'e', 'r', 'v', 'o', ':', 'O', 'K', '\r', '\n'};
    aa55_rx_t rx;
    size_t i;

    aa55_rx_init(&rx);
    for (i = 0U; i < sizeof(noise); ++i)
    {
        CHECK(aa55_rx_feed(&rx, noise[i]) == AA55_RX_INCOMPLETE);
    }
    for (i = 0U; i < sizeof(k_golden); ++i)
    {
        aa55_rx_result_t result = aa55_rx_feed(&rx, k_golden[i]);
        CHECK(result == ((i + 1U == sizeof(k_golden))
                             ? AA55_RX_FRAME_READY
                             : AA55_RX_INCOMPLETE));
    }
    CHECK(rx.command == 0x70U);
    return 1;
}

static int test_concatenated_frames(void)
{
    aa55_rx_t rx;
    size_t i;
    unsigned int ready_count = 0U;

    aa55_rx_init(&rx);
    for (i = 0U; i < sizeof(k_golden); ++i)
    {
        if (aa55_rx_feed(&rx, k_golden[i]) == AA55_RX_FRAME_READY)
        {
            ++ready_count;
            CHECK(rx.command == 0x70U);
        }
    }
    for (i = 0U; i < sizeof(k_empty_golden); ++i)
    {
        if (aa55_rx_feed(&rx, k_empty_golden[i]) == AA55_RX_FRAME_READY)
        {
            ++ready_count;
            CHECK(rx.command == 0x42U);
            CHECK(rx.payload_length == 0U);
        }
    }
    CHECK(ready_count == 2U);
    return 1;
}

static int test_repeated_header_byte(void)
{
    aa55_rx_t rx;
    size_t i;

    aa55_rx_init(&rx);
    CHECK(aa55_rx_feed(&rx, 0xAAU) == AA55_RX_INCOMPLETE);
    for (i = 0U; i < sizeof(k_empty_golden); ++i)
    {
        aa55_rx_result_t result = aa55_rx_feed(&rx, k_empty_golden[i]);
        CHECK(result == ((i + 1U == sizeof(k_empty_golden))
                             ? AA55_RX_FRAME_READY
                             : AA55_RX_INCOMPLETE));
    }
    CHECK(rx.command == 0x42U);
    return 1;
}

static int test_crc_error_and_resync(void)
{
    static const uint8_t bad_frame[] = {0xAAU, 0x55U, 0x00U, 0x42U, 0xAAU};
    aa55_rx_t rx;
    size_t i;

    aa55_rx_init(&rx);
    for (i = 0U; i < sizeof(bad_frame) - 1U; ++i)
    {
        CHECK(aa55_rx_feed(&rx, bad_frame[i]) == AA55_RX_INCOMPLETE);
    }
    CHECK(aa55_rx_feed(&rx, bad_frame[4]) == AA55_RX_CRC_ERROR);

    /* The bad CRC byte was 0xAA and is reused as the next frame header. */
    for (i = 1U; i < sizeof(k_empty_golden); ++i)
    {
        aa55_rx_result_t result = aa55_rx_feed(&rx, k_empty_golden[i]);
        CHECK(result == ((i + 1U == sizeof(k_empty_golden))
                             ? AA55_RX_FRAME_READY
                             : AA55_RX_INCOMPLETE));
    }
    CHECK(rx.command == 0x42U);
    return 1;
}

static int test_length_error_and_resync(void)
{
    aa55_rx_t rx;
    size_t i;

    aa55_rx_init(&rx);
    CHECK(aa55_rx_feed(&rx, 0xAAU) == AA55_RX_INCOMPLETE);
    CHECK(aa55_rx_feed(&rx, 0x55U) == AA55_RX_INCOMPLETE);
    CHECK(aa55_rx_feed(&rx, 0xAAU) == AA55_RX_LENGTH_ERROR);

    /* The invalid length byte was 0xAA and is reused as the frame header. */
    for (i = 1U; i < sizeof(k_empty_golden); ++i)
    {
        aa55_rx_result_t result = aa55_rx_feed(&rx, k_empty_golden[i]);
        CHECK(result == ((i + 1U == sizeof(k_empty_golden))
                             ? AA55_RX_FRAME_READY
                             : AA55_RX_INCOMPLETE));
    }
    CHECK(rx.command == 0x42U);
    return 1;
}

static int test_recovery_on_next_complete_frame(void)
{
    static const uint8_t bad_crc[] = {0xAAU, 0x55U, 0x00U, 0x42U, 0x00U};
    static const uint8_t bad_length[] = {0xAAU, 0x55U, 0x21U};
    aa55_rx_t rx;
    size_t i;
    unsigned int ready_count = 0U;

    aa55_rx_init(&rx);
    for (i = 0U; i < sizeof(bad_crc); ++i)
    {
        aa55_rx_feed(&rx, bad_crc[i]);
    }
    for (i = 0U; i < sizeof(bad_length); ++i)
    {
        aa55_rx_feed(&rx, bad_length[i]);
    }
    for (i = 0U; i < sizeof(k_golden); ++i)
    {
        if (aa55_rx_feed(&rx, k_golden[i]) == AA55_RX_FRAME_READY)
        {
            ++ready_count;
        }
    }
    CHECK(ready_count == 1U);
    CHECK(rx.command == 0x70U);
    return 1;
}

static int test_round_trip_matrix(void)
{
    uint8_t payload[AA55_FRAME_MAX_PAYLOAD];
    uint8_t frame[AA55_FRAME_MAX_SIZE];
    unsigned int command;

    for (command = 0U; command <= 0xFFU; ++command)
    {
        uint8_t length = (uint8_t)(command % (AA55_FRAME_MAX_PAYLOAD + 1U));
        size_t frame_size = 0U;
        uint8_t i;

        for (i = 0U; i < length; ++i)
        {
            payload[i] = (uint8_t)(command + i * 17U);
        }
        CHECK(aa55_encode(frame, sizeof(frame), (uint8_t)command,
                          payload, length, &frame_size) == AA55_STATUS_OK);
        CHECK(decode_exact(frame, frame_size, (uint8_t)command, payload, length));
    }
    return 1;
}

static uint32_t xorshift32(uint32_t *state)
{
    uint32_t value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static int test_deterministic_random_input(void)
{
    aa55_rx_t rx;
    uint32_t random_state = 0xAA55C0DEUL;
    unsigned int i;

    aa55_rx_init(&rx);
    for (i = 0U; i < 100000U; ++i)
    {
        aa55_rx_result_t result = aa55_rx_feed(&rx, (uint8_t)xorshift32(&random_state));
        CHECK(result == AA55_RX_INCOMPLETE ||
              result == AA55_RX_FRAME_READY ||
              result == AA55_RX_CRC_ERROR ||
              result == AA55_RX_LENGTH_ERROR);
        CHECK(rx.state <= 5U);
        if (result == AA55_RX_FRAME_READY)
        {
            CHECK(rx.payload_length <= AA55_FRAME_MAX_PAYLOAD);
            CHECK(rx.index == rx.payload_length);
        }
    }
    return 1;
}

typedef int (*test_function_t)(void);

typedef struct
{
    const char *name;
    test_function_t function;
} test_case_t;

int main(void)
{
    static const test_case_t tests[] = {
        {"CRC-8 check vector", test_crc8_check_vector},
        {"golden encode/decode", test_golden_encode_decode},
        {"empty payload", test_empty_payload},
        {"maximum payload", test_maximum_payload},
        {"encode errors", test_encode_errors},
        {"noise before frame", test_noise_before_frame},
        {"concatenated frames", test_concatenated_frames},
        {"repeated header byte", test_repeated_header_byte},
        {"CRC error and resync", test_crc_error_and_resync},
        {"length error and resync", test_length_error_and_resync},
        {"recovery on next frame", test_recovery_on_next_complete_frame},
        {"round-trip matrix", test_round_trip_matrix},
        {"deterministic random input", test_deterministic_random_input}
    };
    const size_t test_count = sizeof(tests) / sizeof(tests[0]);
    size_t i;
    unsigned int passed = 0U;

    for (i = 0U; i < test_count; ++i)
    {
        if (tests[i].function())
        {
            ++passed;
            printf("[PASS] %s\n", tests[i].name);
        }
        else
        {
            printf("[FAIL] %s\n", tests[i].name);
        }
    }

    printf("%u/%u tests passed\n", passed, (unsigned int)test_count);
    return (passed == test_count) ? 0 : 1;
}
