#include <stdio.h>

int calculate(int a, int b)
{
    int result = a + b;
    return result;
}

int main(void)
{
    int value = calculate(10, 20);
    printf("%d\n", value);
    return 0;
}
