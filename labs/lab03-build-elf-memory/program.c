#include <stdio.h>

int global_value = 10;
int global_empty;

int add(int a, int b)
{
    return a + b;
}

int main(void)
{
    int result = add(global_value, 20);
    printf("Result: %d\n", result);
    return 0;
}
