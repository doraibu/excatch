#define EXCATCH_IMPLEMENTATION
#include "excatch.h"
#include <stdio.h>
#include <stdlib.h>

#define EXC_NULL_PTR 1
#define EXC_OUT_OF_MEM 2
#define EXC_DIV_ZERO 3
#define EXC_IO_ERROR 4

void divn(int a, int b)
{
    if (b == 0)
        exc_throw(__func__, EXC_DIV_ZERO);

    printf("Result: %d\n", a / b);
}

void readn(const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f)
        exc_throw(__func__, EXC_IO_ERROR);

    fclose(f);
}

void procn(int* data, int divisor)
{
    if (!data)
    exc_throw(__func__, EXC_NULL_PTR);

    divn(*data, divisor);
}

int main()
{
    try {
	int data[] = {42, 0};
	procn(data, 0);
	printf("This will never reach if exception is launched\n");
    } catch {
	printf("Exception caught! code: %d\n", _err);
    } try_end
    return 0;
}
