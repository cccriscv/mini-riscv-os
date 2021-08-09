# 09-MemoryAllocator -- RISC-V 的嵌入式作業系統

## 先備知識: Linker Script 撰寫

撰寫 Linker Script 可以讓編譯器在連結的階段按照我們的想法將每個 Section 放到指令的記憶體位址上。

![](https://camo.githubusercontent.com/1d58b18d5a293fe858931e54cce54ac53f4e86b08da25de332a16434688e7434/68747470733a2f2f692e696d6775722e636f6d2f756f72425063642e706e67)

以上圖為例，系統程式在執行時會將每個區段放到記憶體當中，至於到底要分配哪些區段，每個區段的屬性 (可讀可寫可執行) 就需要使用 Linker Script 來告訴編譯器了!

### 語法教學: 入口點與架構

看到本專案的 `os.ld`，即為 mini-riscv-os 的 Linker Script:

```
OUTPUT_ARCH( "riscv" )

ENTRY( _start )

MEMORY
{
  ram   (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 128M
}
```

觀察上面的腳本，可以歸納出幾個小結論:

- 輸出的執行檔會執行在 `riscv` 平台
- 程式的進入點為 `_start`
- 該記憶體名稱為 `ram`，其屬性為:
  - [x] W (可寫)
  - [x] X (可執行)
  - [x] A (可分配)
  - [ ] R (唯讀)
  - [ ] I (初始化)
- 記憶體的起點為 0x80000000，長度為 128 MB，換言之，記憶體範圍為: 0x08000000 - 0x88000000。

接著，可以看到 Linker Script 在 SECTION 段之中切割了好幾的區段，分別是:

- .text
- .rodata
- .data
- .bss

以其中一段的範例對腳本進行解說:

```
.text : {
    PROVIDE(_text_start = .);
    *(.text.init) *(.text .text.*)
    PROVIDE(_text_end = .);
  } >ram AT>ram :text
```

- `PROVIDE` 可以幫助我們定義符號，這些符號也都代表著一個記憶體位址
- `*(.text.init) *(.text .text.*)` 幫助我們配對任何的 object file 中的 .text 區段。
- `>ram AT>ram :text`

  - ram 為 VMA (Virtual Memory Address)，當程式運作時，section 會得到這個記憶體位址。
  - ram :text 為 LMA (Load Memory Address)，當該區段被載入時，會被放到這個記憶體位址。

  最後，Linker Script 還定義了起始與終點的 Symbol 以及 Heap 的位置:

  ```
  PROVIDE(_memory_start = ORIGIN(ram));
  PROVIDE(_memory_end = ORIGIN(ram) + LENGTH(ram));

  PROVIDE(_heap_start = _bss_end);
  PROVIDE(_heap_size = _memory_end - _heap_start);
  ```

如果用圖片表示，記憶體的分布狀況如下:

![](https://i.imgur.com/NCJ3BgL.png)

## 進入正題

### Heap 是什麼?

本文中所提到的 Heap 與資料結構的 Heap 不同，這邊的 Heap 是指可供作業系統與 Process 分配的記憶體空間，我們都知道， Stack 會存放已經初始化的固定長度資料，比起 Stack ， Heap 有了更多彈性，我們想要使用多少空間就分配多少空間，並且在使用後可以進行記憶體回收避免浪費。

```cpp
#include <stdlib.h>
int *p = (int*) malloc(sizeof(int));
// ...
free(p);
```

上面的 C 語言範例便是使用 `malloc()` 進行動態記憶體的配置，並且在使用完畢後呼叫 `free()` 對記憶體進行回收。

### mini-riscv-os 的實作

了解 Heap 與 Linker Script 所描述出的記憶體結構後，就要進入本文的重點了!
在這次的單元中，我們特別切割出一段空間供 Heap 使用，這樣一來，在系統程式中也可以實現類似於 Memory Allocator 的功能:

```assembly
.section .rodata
.global HEAP_START
HEAP_START: .word _heap_start

.global HEAP_SIZE
HEAP_SIZE: .word _heap_size

.global TEXT_START
TEXT_START: .word _text_start

.global TEXT_END
TEXT_END: .word _text_end

.global DATA_START
DATA_START: .word _data_start

.global DATA_END
DATA_END: .word _data_end

.global RODATA_START
RODATA_START: .word _rodata_start

.global RODATA_END
RODATA_END: .word _rodata_end

.global BSS_START
BSS_START: .word _bss_start

.global BSS_END
BSS_END: .word _bss_end
```

在 `mem.s` 中，我們宣告多個變數，每個變數都代表了先前在 Linker Script 定義的 Symbol，這樣一來，我們就可以在 C 程式中取用這些記憶體位址:

```cpp
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
```

### 如何管理記憶體區塊

其實在主流的作業系統中，Heap 的結構非常複雜，會有多個列表管理未被分配的記憶體區塊以及不同大小的已分配記憶體區塊。

```cpp
static uint32_t _alloc_start = 0;
static uint32_t _alloc_end = 0;
static uint32_t _num_pages = 0;

#define PAGE_SIZE 256
#define PAGE_ORDER 8
```

在 mini-riscv-os 中，我們統一區塊的大小為 25b Bits，也就是說，當我們呼叫 `malloc(sizeof(int))` 時，他也會一口氣分配 256 Bits 的空間給這個請求。

```cpp
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
```

在 `page_init()` 當中可以看到，假設有 N 個 256 Bits 的記憶體區塊可供分配，我們勢必要實作出一個資料結構來管理記憶體區塊的狀態:

```cpp
struct Page
{
  uint8_t flags;
};
```

因此，Heap 的記憶體會被用來存放: N 個 Page Struct 以及 N 個 256 Bits 的記憶體區塊，呈現一對一的關係。
至於，要如何分辨第 A 個記憶體區塊是否被分配，就要看與之對應的 Page Struct 中的 flag 記錄了什麼:

- 00: This means this page hasn't been allocated
- 01: This means this page was allocated
- 11: This means this page was allocated and is the last page of the memory block allocated

`00` 與 `01` 的狀態十分易懂，至於 `11` 會在哪種情況用到呢?我們繼續往下看:

```cpp
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
```

透過閱讀 `malloc()` 的原始碼可以得知，當使用者裡用它嘗試獲取大於 256 Bits 的記憶體空間時，他會先計算請求的記憶體大小需要多少區塊才能滿足。計算完成後，他會在連續的記憶體區塊中尋找相連且未分配的記憶體區塊分配給該請求:

```
malloc(513);
Cause 513 Bits > The size of the 2 blocks,
thus, malloc will allocates 3 blocks for the request.

Before Allocation:

+----+    +----+    +----+    +----+    +----+
| 00 | -> | 00 | -> | 00 | -> | 00 | -> | 00 |
+----+    +----+    +----+    +----+    +----+

After Allocation:

+----+    +----+    +----+    +----+    +----+
| 01 | -> | 01 | -> | 11 | -> | 00 | -> | 00 |
+----+    +----+    +----+    +----+    +----+

```

分配完成後，我們可以發現最後一個被分配的記憶體區塊的 Flag 為 `11`，這樣一來，等到使用者呼叫 `free()` 釋放這塊記憶體時，系統就可以透過 Flag 確認帶有 `11` 標記的區塊是需要被釋放的最後一塊區塊。

## Reference

- [10 分鐘讀懂 linker scripts](https://blog.louie.lu/2016/11/06/10%E5%88%86%E9%90%98%E8%AE%80%E6%87%82-linker-scripts/)
- [Step by step, learn to develop an operating system on RISC-V](https://github.com/plctlab/riscv-operating-system-mooc)
- [Heap Exploitation](https://github.com/ianchen0119/About-Security/wiki/Heap-Exploitation)
