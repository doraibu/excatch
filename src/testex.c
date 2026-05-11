#include "excatch.h"
#include <stdio.h>

void internal_task()
{
    guard(mem, raii_malloc(128, free));
    printf("Memory: %p\n", RAII_UNTAG(mem));
    printf("CRITICAL\n");
    throw("CRITICAL_FAILURE", 500);
}

int main()
{
    try {
	printf("Initializing...\n");
	internal_task();
    } catch(code) {
	printf("Exception captured! Code: %d\n", code);
    } end_try

    return 0;
}
