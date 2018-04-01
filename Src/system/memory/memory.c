#include "memory.h"
#include "string.h"

char memorys[PAGE_NUM * PAGE_SIZE];
char* memory_pg0 = memorys;
char* memory_pg1 = &memorys[PAGE_SIZE * 1];
char* memory_pg2 = &memorys[PAGE_SIZE * 2];
char* memory_pg3 = &memorys[PAGE_SIZE * 3];
char* memory_pg4 = &memorys[PAGE_SIZE * 4];
char* memory_pg5 = &memorys[PAGE_SIZE * 5];
char* memory_pg6 = &memorys[PAGE_SIZE * 6];
char* memory_pg7 = &memorys[PAGE_SIZE * 7];

void init_memory()
{
	memset(memorys, 0, PAGE_NUM * PAGE_SIZE);
}
