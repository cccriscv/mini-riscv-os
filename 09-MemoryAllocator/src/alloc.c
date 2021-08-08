/*  This Code derived from RVOS
 *  -- https://github.com/plctlab/riscv-operating-system-mooc
 */

#include "os.h"

extern uint32_t TEXT_START;
extern uint32_t TEXT_END;
extern uint32_t DATA_START;
extern uint32_t DATA_END;
extern uint32_t RODATA_START;
extern uint32_t RODATA_END;
extern uint32_t BSS_START;
extern uint32_t BSS_END;
extern uint32_t HEAP_START;
extern uint32_t HEAP_SIZE;

/*
 * _alloc_start points to the actual start address of heap pool
 * _alloc_end points to the actual end address of heap pool
 * _num_pages holds the actual max number of pages we can allocate.
 */

static uint32_t _alloc_start = 0;
static uint32_t _alloc_end = 0;
static uint32_t _num_pages = 0;

#define PAGE_SIZE 256
#define PAGE_ORDER 8

#define PAGE_TAKEN (uint8_t)(1 << 0)
#define PAGE_LAST (uint8_t)(1 << 1)
#define pageNum(x) ((x) / ((PAGE_SIZE) + 1)) + 1

/*
 * Page Descriptor 
 * flags:
 * - 00: This means this page hasn't been allocated
 * - 01: This means this page was allocated
 * - 11: This means this page was allocated and is the last page of the memory block allocated
 */

struct Page
{
  uint8_t flags;
};

static inline void _clear(struct Page *page)
{
  page->flags = 0;
}

static inline int _is_free(struct Page *page)
{
  if (page->flags & PAGE_TAKEN)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

static inline void _set_flag(struct Page *page, uint8_t flags)
{
  page->flags |= flags;
}

static inline int _is_last(struct Page *page)
{
  if (page->flags & PAGE_LAST)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

/*
 * align the address to the border of page (256)
 */

static inline uint32_t _align_page(uint32_t address)
{
  uint32_t order = (1 << PAGE_ORDER) - 1;
  return (address + order) & (~order);
}

void page_init()
{
  _num_pages = (HEAP_SIZE / PAGE_SIZE) - 2048;
  lib_printf("HEAP_START = %x, HEAP_SIZE = %x, num of pages = %d\n", HEAP_START, HEAP_SIZE, _num_pages);

  struct Page *page = (struct Page *)HEAP_START;
  for (int i = 0; i < _num_pages; i++)
  {
    _clear(page);
    page++;
  }

  _alloc_start = _align_page(HEAP_START + 2048 * PAGE_SIZE);
  _alloc_end = _alloc_start + (PAGE_SIZE * _num_pages);

  lib_printf("TEXT:   0x%x -> 0x%x\n", TEXT_START, TEXT_END);
  lib_printf("RODATA: 0x%x -> 0x%x\n", RODATA_START, RODATA_END);
  lib_printf("DATA:   0x%x -> 0x%x\n", DATA_START, DATA_END);
  lib_printf("BSS:    0x%x -> 0x%x\n", BSS_START, BSS_END);
  lib_printf("HEAP:   0x%x -> 0x%x\n", _alloc_start, _alloc_end);
}

/*
 * Allocate a memory block which is composed of contiguous physical pages
 * - npages: the number of PAGE_SIZE pages to allocate
 */

void *malloc(size_t size)
{
  int npages = pageNum(size);
  int found = 0;
  struct Page *page_i = (struct Page *)HEAP_START;
  for (int i = 0; i < (_num_pages - npages); i++)
  {
    if (_is_free(page_i))
    {
      found = 1;

      /* 
			 * meet a free page, continue to check if following
			 * (npages - 1) pages are also unallocated.
			 */

      struct Page *page_j = page_i;
      for (int j = i; j < (i + npages); j++)
      {
        if (!_is_free(page_j))
        {
          found = 0;
          break;
        }
        page_j++;
      }
      /*
			 * get a memory block which is good enough for us,
			 * take housekeeping, then return the actual start
			 * address of the first page of this memory block
			 */
      if (found)
      {
        struct Page *page_k = page_i;
        for (int k = i; k < (i + npages); k++)
        {
          _set_flag(page_k, PAGE_TAKEN);
          page_k++;
        }
        page_k--;
        _set_flag(page_k, PAGE_LAST);
        return (void *)(_alloc_start + i * PAGE_SIZE);
      }
    }
    page_i++;
  }
  return NULL;
}

/*
 * Free the memory block
 * - p: start address of the memory block
 */
void free(void *p)
{
  /*
	 * Assert (TBD) if p is invalid
	 */
  if (!p || (uint32_t)p >= _alloc_end)
  {
    return;
  }
  /* get the first page descriptor of this memory block */
  struct Page *page = (struct Page *)HEAP_START;
  page += ((uint32_t)p - _alloc_start) / PAGE_SIZE;
  /* loop and clear all the page descriptors of the memory block */
  while (!_is_free(page))
  {
    if (_is_last(page))
    {
      _clear(page);
      break;
    }
    else
    {
      _clear(page);
      page++;
      ;
    }
  }
}

void page_test()
{
  void *p = malloc(1024);
  lib_printf("p = 0x%x\n", p);

  void *p2 = malloc(512);
  lib_printf("p2 = 0x%x\n", p2);

  void *p3 = malloc(sizeof(int));
  lib_printf("p3 = 0x%x\n", p3);

  free(p);
  free(p2);
  free(p3);
}