#include <stdio.h>

void print_binary(unsigned char value)
{
    for (int i = 7; i >= 0; i--) {
        printf("%u", (value >> i) & 1u);
    }
}

int main(void)
{
    unsigned char values[] = {0, 1, 10, 65, 127, 128, 255};
    int count = sizeof(values) / sizeof(values[0]);

    for (int i = 0; i < count; i++) {
        printf("Decimal: %u\n", values[i]);
        printf("Hex: 0x%02X\n", values[i]);
        printf("Binary: ");
        print_binary(values[i]);
        printf("\n\n");
    }
    return 0;
}
