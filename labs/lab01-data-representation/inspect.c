#include <stdio.h>
#include <ctype.h>

static void print_binary(unsigned char value)
{
    for (int i = 7; i >= 0; i--) {
        printf("%u", (value >> i) & 1u);
    }
}

int main(void)
{
    unsigned int input;

    printf("Enter value (0-255): ");
    if (scanf("%u", &input) != 1 || input > 255) {
        printf("Invalid input: enter a number from 0 to 255\n");
        return 1;
    }

    unsigned char value = (unsigned char)input;

    printf("Decimal: %u\n", value);
    printf("Hexadecimal: 0x%02X\n", value);
    printf("Binary: ");
    print_binary(value);
    printf("\n");

    if (isprint(value)) {
        printf("Character: %c\n", value);
    }
    return 0;
}
