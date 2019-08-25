#ifndef MEMORY_HPP
#define MEMORY_HPP

#ifndef __cplusplus 

#include <stddef.h>
void  memory_set_area(void* pBuff, size_t max_size);
void* memory_alloc(size_t size);
void  memory_free(void **pptr);

#endif

#endif
