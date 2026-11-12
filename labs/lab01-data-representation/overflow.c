#include <stdio.h>
#include <stdint.h>

int main(void)
{
    uint8_t value = 255;
    printf("Before: %u\n", value);
    value++;
    printf("After value++: %u\n", value);

    printf("\n--- checks ---\n");
    uint8_t checks[] = {254, 255};
    for (int i = 0; i < 2; i++) {
        uint8_t v = checks[i];
        printf("value = %u, after v++ = %u\n", checks[i], (uint8_t)(v + 1));
    }

    uint8_t base = 255;
    uint8_t r1 = base + 1;
    uint8_t r2 = base + 2;
    printf("255 + 1 in uint8_t = %u\n", r1);
    printf("255 + 2 in uint8_t = %u\n", r2);
    return 0;
}
