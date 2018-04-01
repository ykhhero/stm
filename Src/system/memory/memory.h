#ifndef __MEMORY_H__
#define __MEMORY_H__

#define PAGE_SIZE 10
#define PAGE_NUM 8
extern char memorys[];
extern char* memory_pg0;
extern char* memory_pg1;
extern char* memory_pg2;
extern char* memory_pg3;
extern char* memory_pg4;
extern char* memory_pg5;
extern char* memory_pg6;
extern char* memory_pg7;

void init_memory(void);

#endif
