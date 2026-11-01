# aa55-frame

[简体中文](README.zh-CN.md)

A small, header-only C99 library for `AA 55` binary frames. It includes a checked encoder, a streaming decoder, and CRC-8. No heap allocation, operating-system calls, or platform-specific drivers are required.

## Features

- C99, with C++ inclusion guards
- One header file and no external dependencies
- No dynamic memory allocation
- Payloads from 0 to 32 bytes
- Explicit encoder error codes and output-capacity checks
- Byte-at-a-time decoding for UART interrupts, DMA buffers, and host programs
- Recovery from noise, split frames, concatenated frames, invalid lengths, and CRC errors
- Host-side tests and a minimal example

## Frame format

| Byte | Field | Description |
|---:|---|---|
| 0 | `0xAA` | First header byte |
| 1 | `0x55` | Second header byte |
| 2 | `LEN` | Payload size, excluding `CMD` and `CRC8` |
| 3 | `CMD` | Application-defined command |
| 4.. | `PAYLOAD` | Zero or more payload bytes |
| Last | `CRC8` | CRC over `LEN`, `CMD`, and `PAYLOAD` |

CRC parameters: polynomial `0x07`, initial value `0x00`, no input or output reflection, and no final XOR. This is the usual CRC-8 check configuration; the ASCII string `123456789` produces `0xF4`.

Example: command `0x70` with payload `01 02 03` is encoded as:

```text
AA 55 03 70 01 02 03 DC
```

## Interoperability

The encoder, decoder, and CRC have been cross-checked byte-for-byte against an independent Python implementation of the same format. Frames produced by both implementations are identical, and they have been exchanged over a real 115200-baud 8N1 UART link between an STM32 microcontroller and an embedded vision module in field operation. Any other implementation of this frame format inter-operates directly, which makes the library a convenient reference codec for heterogeneous multi-board systems.

## Add to a project

### Copy the header

Copy [`include/aa55_frame.h`](include/aa55_frame.h) into the project and include it:

```c
#include "aa55_frame.h"
```

### CMake

```cmake
add_subdirectory(path/to/aa55-frame)
target_link_libraries(your_target PRIVATE aa55::frame)
```

The library target is `INTERFACE` because all functions are defined in the header.

## Encode a frame

```c
uint8_t payload[] = {0x01, 0x02, 0x03};
uint8_t frame[AA55_FRAME_MAX_SIZE];
size_t frame_size;

aa55_status_t status = aa55_encode(
    frame, sizeof(frame),
    0x70,
    payload, sizeof(payload),
    &frame_size
);

if (status == AA55_STATUS_OK) {
    /* Send frame[0..frame_size). */
}
```

The encoder does not modify `frame_size` beyond setting it to zero when validation fails. The output and payload buffers must not overlap.

## Decode a byte stream

```c
aa55_rx_t rx;
aa55_rx_init(&rx);

void on_uart_byte(uint8_t byte)
{
    aa55_rx_result_t result = aa55_rx_feed(&rx, byte);

    if (result == AA55_RX_FRAME_READY) {
        /* rx.command, rx.payload_length, and rx.payload are valid here. */
    }
}
```

Create one receiver per stream. The decoded fields remain valid until the next `aa55_rx_feed()` call, so copy them if they are needed later. Call `aa55_rx_reset()` to discard a partial frame.

## API summary

| Function | Purpose |
|---|---|
| `aa55_encode()` | Encode one frame with size and argument validation |
| `aa55_rx_init()` | Initialize a streaming receiver |
| `aa55_rx_reset()` | Discard the current partial frame |
| `aa55_rx_feed()` | Process one input byte |
| `aa55_crc8()` | Calculate CRC-8 over a byte array |
| `aa55_crc8_update()` | Update CRC-8 with one byte |

Encoder status values:

| Value | Meaning |
|---|---|
| `AA55_STATUS_OK` | Frame encoded successfully |
| `AA55_STATUS_INVALID_ARGUMENT` | A required pointer is `NULL` |
| `AA55_STATUS_PAYLOAD_TOO_LARGE` | Payload exceeds 32 bytes |
| `AA55_STATUS_BUFFER_TOO_SMALL` | Output capacity is insufficient |

Decoder results:

| Value | Meaning |
|---|---|
| `AA55_RX_INCOMPLETE` | More bytes are required |
| `AA55_RX_FRAME_READY` | A valid frame is available |
| `AA55_RX_CRC_ERROR` | CRC mismatch; resynchronization has started |
| `AA55_RX_LENGTH_ERROR` | Invalid payload length; resynchronization has started |
| `AA55_RX_INVALID_ARGUMENT` | Receiver pointer is `NULL` |

## Build and test

CMake 3.15 or newer and a C99 compiler are sufficient:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

The test suite covers standard CRC vectors, golden frames, empty and maximum payloads, argument validation, noise, repeated headers, concatenated frames, CRC and length errors, round trips for all command values, and deterministic random input.

Run the example after building:

```sh
./build/examples/aa55_basic
```

On Windows with a multi-configuration generator, the executable may be under `build/examples/Debug/`.

## Scope and limits

- Command values and payload meanings belong to the application.
- CRC-8 detects transmission errors; it does not provide authentication or protection against deliberate modification.
- The protocol has no sequence numbers, acknowledgements, or retransmission. Add them at the application or transport layer if required.
- The payload limit is fixed at 32 bytes to keep receiver storage predictable.

## License

MIT. See [LICENSE](LICENSE).
