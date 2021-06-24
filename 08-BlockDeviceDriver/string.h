#ifndef __STRING_H__
#define __STRING_H__

void *memset(void *, int, unsigned int);
void *
memcpy(void *dst, const void *src, unsigned int n);
void *
memmove(void *dst, const void *src, unsigned int n);

#endif