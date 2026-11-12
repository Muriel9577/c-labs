#include <stdio.h>

int main(void)
{
    unsigned int values[] = {10, 65, 127, 255, 256, 1024};
    int count = sizeof(values) / sizeof(values[0]);

    for (int i = 0; i < count; i++) {
        printf("Decimal: %u\n", values[i]);
        printf("Hexadecimal: 0x%X\n\n", values[i]);
    }
    return 0;
}
