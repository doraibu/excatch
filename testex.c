#define EXCATCH_IMPLEMENTATION
#include "excatch.h"
#include <stdio.h>

void profunda()
{
    printf("Entrou na função...\n");
    exc_throw("main", 404);
}

int main()
{
    try {
	printf("tentando algo...\n");
	profunda();
	printf("Isso nunca vai acontecer");
    } catch {
	printf("capturando erro!\n");
    } try_end

    return 0;
}
