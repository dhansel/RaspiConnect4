/*

  Naive Memory Allocator   v.1.0  (C)  By Filippo Bergamasco  2014


  This is a super simple memory allocator. A linked list is used to keep trace
  of free blocks. On memory request, the list is iterated until a block with
  enough space is found. Then, the available space of this block is splitted into
  a data section (returned to the caller) and a free block with the remaining space.

  When an allocated data section should be freed, a new free block is created
  and inserted in the correct place into the linked list according to the block
  location in memory. Then, merge_adjacent_free_blocks() function is called to
  attempt to merge adjacent free blocks.

  Usage:

    1) Set the memory area to be used by the allocator:
        nmalloc_set_memory_area(  pBuff, size );

    2) Call
        nmalloc_malloc( size )
       to obtain a memory chunk of size "size"

    3) When an allocated chunk "a" is no longer needed, call
        nmalloc_free( &a );

        the pointer a is automatically set to 0.



   Finally, memory chunks are managed keeping the free blocks header struct
   aligned to avoid un-necessary padding and reduce memory footprint.

 ----------------------------------------------------------------------------

    Copyright 2014 Filippo Bergamasco

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

 ----------------------------------------------------------------------------
*/

#include <cstddef>
#include "Memory.hpp"

typedef struct _block
{
    struct _block* prev;
    struct _block* next;
    size_t size;

} block;

#define BLOCK_HEADER_SIZE sizeof(block)

typedef struct
{
    void* data;
    block* first_free;

} _nmalloc_data_t;

static _nmalloc_data_t _nmalloc_data;


static void merge_adjacent_free_blocks( void )
{
    block* cb;

    cb = _nmalloc_data.first_free;

    while( cb->next )
    {
        if( (((unsigned char*)cb) + cb->size) == ((unsigned char*)(cb->next)) )
        {
            /* merge this block with the next */
            cb->size = cb->size + cb->next->size;
            cb->next = cb->next->next;
            if( cb->next )
                cb->next->prev = cb;
        }
        else
        {
            /* go on with the list */
            cb = cb->next;
        }
    }
}


extern "C" void memory_set_area( void* pBuff, size_t max_size )
{
    _nmalloc_data.data = pBuff;

    _nmalloc_data.first_free = (block*)pBuff;
    _nmalloc_data.first_free->size = max_size;
    _nmalloc_data.first_free->prev = 0;
    _nmalloc_data.first_free->next = 0;

}


extern "C" void* memory_alloc( size_t size )
{
    block* cfree;
    block newfblock;
    block* pnewfblock;
    size_t data_size;
    size_t size_needed;

    if( size == 0 )
        return 0;

    data_size =  sizeof(size_t) + size;

    if( data_size < BLOCK_HEADER_SIZE )
        data_size = BLOCK_HEADER_SIZE;

    /* Let data_size be a multiple of BLOCK_HEADER_SIZE to respect struct alignment */
    data_size += BLOCK_HEADER_SIZE - (data_size % (BLOCK_HEADER_SIZE));

    size_needed = data_size + BLOCK_HEADER_SIZE;


    /* Follow the free blocks list to find a good one */
    cfree = _nmalloc_data.first_free;

    while( cfree && cfree->next && cfree->size < size_needed )
        cfree = cfree->next;


    if( cfree==0 || cfree->size < size_needed )
    {
        /* not enough space, for now */
        return 0;
    }


    /* subdivide this free block into used data size, data and a new free block header */
    newfblock.next = cfree->next;
    newfblock.prev = cfree->prev;
    newfblock.size = cfree->size - data_size;
;

    pnewfblock =  (block*)(cfree + (data_size/BLOCK_HEADER_SIZE ));
    *pnewfblock = newfblock;

    if( pnewfblock->prev == 0 )
        _nmalloc_data.first_free = pnewfblock;
    else
        pnewfblock->prev->next = pnewfblock;

    if( pnewfblock->next )
        pnewfblock->next->prev = pnewfblock;


    *(size_t*)cfree = data_size;
    return ((unsigned char *)cfree)+sizeof(size_t);
}


extern "C" void memory_free(void **pptr)
{
    block* prevB;
    block* nextB;
    block *newfblock;
    size_t block_size;
    void* ptr = *pptr;

    ptr = (unsigned char*)ptr - sizeof( size_t );
    block_size = *((size_t*)ptr);

    newfblock = (block*)ptr;
    newfblock->size = block_size;


    /* Find the correct place into the free blocks list
     * to insert the new free block */

    if( newfblock < _nmalloc_data.first_free )
    {
        /* this will be the new first block */
        newfblock->prev = 0;
        newfblock->next = _nmalloc_data.first_free;
        _nmalloc_data.first_free = newfblock;

    } else
    {
        prevB = _nmalloc_data.first_free;
        nextB = prevB->next;
        while( nextB )
        {
            if( prevB < newfblock &&  newfblock < nextB )
            {
                /* This is the correct place */
                break;
            }
            prevB=nextB;
            nextB = nextB->next;
        }
        newfblock->prev = prevB;
        newfblock->next = nextB;
    }


    /* Update neighbours */
    if( newfblock->prev )
        newfblock->prev->next = newfblock;

    if( newfblock->next )
        newfblock->next->prev = newfblock;


    *pptr = 0;
    merge_adjacent_free_blocks();
}


// ----------------------------- C++ operators


void* operator new(size_t count)
{
  return memory_alloc(count); 
}

void* operator new[](size_t count)
{
  return memory_alloc(count); 
}

void operator delete(void* p)
{
  if( p!=NULL ) memory_free(&p);
}

void operator delete(void* p, size_t t)
{
  if( p!=NULL ) memory_free(&p);
}

void operator delete[](void* p)
{
  if( p!=NULL ) memory_free(&p);
}

void operator delete[](void* p, size_t t)
{
  if( p!=NULL ) memory_free(&p);
}
