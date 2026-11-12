#include <stdio.h>
#include <stdint.h>

int main(void)
{
    uint32_t value = 0x12345678;
    unsigned char *p = (unsigned char *)&value;

    printf("value = 0x%08X\n", value);
    printf("byte 0 = 0x%02X\n", p[0]);
    printf("byte 1 = 0x%02X\n", p[1]);
    printf("byte 2 = 0x%02X\n", p[2]);
    printf("byte 3 = 0x%02X\n", p[3]);
    return 0;
}
