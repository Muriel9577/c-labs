#include <stdio.h>

int sum_to(int n)
{
    int sum = 0;
    for (int i = 1; i <= n; i++) {
        sum += i;
    }
    return sum;
}

int main(void)
{
    printf("%d\n", sum_to(5));
    return 0;
}
