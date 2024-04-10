#ifndef _BUDDY_H_
#define _BUDDY_H_

#include <pmap.h>

#define BUDDY_PAGE_BASE 0x03C00000
#define BUDDY_PAGE_END 0x04000000

void buddy_init(void);
int buddy_alloc(u_int size, struct Page **new);
void buddy_free(struct Page *pp, int npp);

#endif
