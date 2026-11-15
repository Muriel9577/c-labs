#include <stdio.h>
#include <unistd.h>

int main(void)
{
    long page_size = sysconf(_SC_PAGESIZE);

    printf("Page size: %ld bytes\n", page_size);
    return 0;
}
