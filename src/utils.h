#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus 
extern "C"         
{                  
#endif

extern void busywait( unsigned int cycles );
extern void usleep( unsigned int usec );
extern unsigned int time_microsec();

extern unsigned int strlen( char* str );

inline int isspace(char c) { return (c>=9 && c<=13) || c==32; }

inline void *memcpy(void* vdst, const void* vsrc, unsigned int len)
{
  unsigned char *src    = (unsigned char *) vsrc;
  unsigned char *dst    = (unsigned char *) vdst;
  unsigned char* srcEnd = src + len;
  while( src<srcEnd ) *dst++ =  *src++;

  return vdst;
}

inline void memset(void* vdst, int value, unsigned int len)
{
  unsigned char *dst    = (unsigned char *) vdst;
  unsigned char* dstEnd = dst + len;
  while( dst<dstEnd ) *dst++ =  value;
}

/*
 *   Data memory barrier
 *   No memory access after the DMB can run until all memory accesses before it
 *    have completed
 *    
 */
#define dmb() asm volatile \
                ("mcr p15, #0, %[zero], c7, c10, #5" : : [zero] "r" (0) )


/*
 *  Data synchronisation barrier
 *  No instruction after the DSB can run until all instructions before it have
 *  completed
 */
#define dsb() asm volatile \
                ("mcr p15, #0, %[zero], c7, c10, #4" : : [zero] "r" (0) )


/*
 * Clean and invalidate entire cache
 * Flush pending writes to main memory
 * Remove all data in data cache
 */
#define flushcache() asm volatile \
                ("mcr p15, #0, %[zero], c7, c14, #0" : : [zero] "r" (0) )


#ifdef __cplusplus 
}
#endif

#endif
