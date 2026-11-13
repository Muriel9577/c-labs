#include <stdio.h>

void level3(void)
{
    int c = 30;
    printf("level3: &c = %p\n", (void *)&c);
}

void level2(void)
{
    int b = 20;
    printf("level2: &b = %p\n", (void *)&b);
    level3();
}

void level1(void)
{
    int a = 10;
    printf("level1: &a = %p\n", (void *)&a);
    level2();
}

int main(void)
{
    level1();
    return 0;
}
