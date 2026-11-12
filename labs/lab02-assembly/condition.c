#include <stdio.h>

int max_value(int a, int b)
{
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

int main(void)
{
    printf("%d\n", max_value(10, 20));
    return 0;
}
