#include <stdio.h>
#include <stdint.h>

int main(void)
{
    int8_t signed_value = -1;
    uint8_t unsigned_value = 255;

    printf("signed_value:   %d\n", signed_value);
    printf("unsigned_value: %u\n", unsigned_value);

    printf("signed_value   hex: 0x%02X\n", (uint8_t)signed_value);
    printf("unsigned_value hex: 0x%02X\n", unsigned_value);
    return 0;
}
