#include <stdio.h>
#include <stdint.h>

int main(void)
{
    uint32_t value = 0x12345678;
    unsigned char *p = (unsigned char *)&value;

    printf("value = 0x%08X\n\n", value);
    printf("Address            Value\n");
    for (int i = 0; i < 4; i++) {
        printf("%p  0x%02X\n", (void *)&p[i], p[i]);
    }
    return 0;
}
