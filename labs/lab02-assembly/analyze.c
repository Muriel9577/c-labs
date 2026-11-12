#include <stdio.h>

int analyze(int a, int b)
{
    int result = 0;

    if (a > b) {
        result = a - b;
    } else {
        result = b - a;
    }

    for (int i = 0; i < 3; i++) {
        result += i * 2;
    }

    return result;
}

int main(void)
{
    printf("Result: %d\n", analyze(10, 4));
    return 0;
}
