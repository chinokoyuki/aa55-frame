#include <stdio.h>

#include "aa55_frame.h"

int main(void)
{
    const uint8_t payload[] = {0x01U, 0x02U, 0x03U};
    uint8_t frame[AA55_FRAME_MAX_SIZE];
    size_t frame_size = 0U;
    size_t i;
    aa55_rx_t rx;
    aa55_status_t status;

    status = aa55_encode(frame, sizeof(frame), 0x70U,
                         payload, (uint8_t)sizeof(payload), &frame_size);
    if (status != AA55_STATUS_OK)
    {
        fprintf(stderr, "Encoding failed: %d\n", (int)status);
        return 1;
    }

    printf("Encoded frame:");
    for (i = 0U; i < frame_size; ++i)
    {
        printf(" %02X", frame[i]);
    }
    printf("\n");

    aa55_rx_init(&rx);
    for (i = 0U; i < frame_size; ++i)
    {
        if (aa55_rx_feed(&rx, frame[i]) == AA55_RX_FRAME_READY)
        {
            printf("Decoded: command=0x%02X, payload=", rx.command);
            for (size_t j = 0U; j < rx.payload_length; ++j)
            {
                printf("%s0x%02X", (j == 0U) ? "" : ", ", rx.payload[j]);
            }
            printf("\n");
            return 0;
        }
    }

    fprintf(stderr, "No complete frame decoded\n");
    return 1;
}
