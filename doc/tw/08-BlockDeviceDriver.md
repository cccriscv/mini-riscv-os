# 08-BlockDeviceDriver -- RISC-V 的嵌入式作業系統

在實現外部中斷的機制以後，我們已經在先前的 Lab 中加入了 UART 的 ISR，為了讓作業系統能夠讀取磁碟資料，我們必須加入 VirtIO 的 ISR :

```cpp
void external_handler()
{
  int irq = plic_claim();
  if (irq == UART0_IRQ)
  {
    lib_isr();
  }
  else if (irq == VIRTIO_IRQ)
  {
    virtio_disk_isr();
  }
  else if (irq)
  {
    lib_printf("unexpected interrupt irq = %d\n", irq);
  }

  if (irq)
  {
    plic_complete(irq);
  }
}
```

當然，在開始之前我們仍需要認識一下 VirtIO 協定。

## 先備知識: Virtio

### Descriptor

Descriptor 包含這些訊息: 地址，地址長度，某些 flag 和其他信息。
使用 Descriptor，我們可以將設備指向 RAM 中任何緩衝區的內存位址。

```cpp
struct virtq_desc
{
  uint64 addr;
  uint32 len;
  uint16 flags;
  uint16 next;
};
```

- addr: 我們可以在 64-bit 內存地址內的任何位置告訴設備存儲位置。
- len: 讓 Device 知道有多少內存可用。
- flags: 用於控制 descriptor。
- next: 告訴 Device 下一個描述符的 Index。如果指定了 VIRTQ_DESC_F_NEXT，Device 僅讀取該字段。否則無效。

### AvailableRing

用來存放 Descriptor 的索引，當 Device 收到通知時，它會檢查 AvailableRing 確認需要讀取哪些 Descriptor。

> 需要注意的是: Descriptor 和 AvailableRing 都存儲在 RAM 中。

```cpp
struct virtq_avail
{
  uint16 flags;     // always zero
  uint16 idx;       // driver will write ring[idx] next
  uint16 ring[NUM]; // descriptor numbers of chain heads
  uint16 unused;
};
```

### UsedRing

UsedRing 讓 Device 能夠向 OS 發送訊息，因此，Device 通常使用它來告知 OS 它已完成先前通知的請求。
AvailableRing 與 UsedRing 非常相似，差別在於: OS 需要查看 UsedRing 得知哪個 Descriptor 已經被服務。

```cpp
struct virtq_used_elem
{
  uint32 id; // index of start of completed descriptor chain
  uint32 len;
};

struct virtq_used
{
  uint16 flags; // always zero
  uint16 idx;   // device increments when it adds a ring[] entry
  struct virtq_used_elem ring[NUM];
};
```

## 發送讀寫請求

為了節省閱讀 VirtIO Spec 的時間，本次的 Block Device Driver 參考了 xv6-riscv 的實作，不過在 xv6 的檔案系統實作中有非常多層:

```
+------------------+
|  File descriptor |
+------------------+
|  Pathname        |
+------------------+
|  Directory       |
+------------------+
|  Inode           |
+------------------+
|  Logging         |
+------------------+
|  Buffer cache    |
+------------------+
|  Disk            |
+------------------+
```

完成 Device Driver 會讓我們在日後實現檔案系統更為方便，此外，參考 xv6-riscv 的設計，我們還會需要實現一層 Buffer cache 用來同步硬碟上的資料。

### 指定寫入的 Sector

```cpp
uint64_t sector = b->blockno * (BSIZE / 512);
```

### 分配 descriptor

因為 [qemu-virt](https://github.com/qemu/qemu/blob/master/hw/block/virtio-blk.c) 會一次讀取 3 個 descriptor，所以在發送請求之前我們要先分配好這些空間。

```cpp
static int
alloc3_desc(int *idx)
{
  for (int i = 0; i < 3; i++)
  {
    idx[i] = alloc_desc();
    if (idx[i] < 0)
    {
      for (int j = 0; j < i; j++)
        free_desc(idx[j]);
      return -1;
    }
  }
  return 0;
}
```

### 發送 Block request

宣告 req 的結構:

```cpp
struct virtio_blk_req *buf0 = &disk.ops[idx[0]];
```

因為磁碟有讀寫操作之分，為了讓 qemu 知道要讀還是要寫，我們要在請求中的 `type` 成員中寫入 **flag** :

```cpp
if(write)
  buf0->type = VIRTIO_BLK_T_OUT; // write the disk
else
  buf0->type = VIRTIO_BLK_T_IN; // read the disk
buf0->reserved = 0; // The reserved portion is used to pad the header to 16 bytes and move the 32-bit sector field to the correct place.
buf0->sector = sector; // specify the sector that we wanna modified.
```

### 填充 Descriptor

到了這一步，我們已經分配好 Descriptor 與 req 的基本資料了，接著我們可以對這三個 Descriptor 做資料填充:

```cpp
disk.desc[idx[0]].addr = buf0;
  disk.desc[idx[0]].len = sizeof(struct virtio_blk_req);
  disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
  disk.desc[idx[0]].next = idx[1];

  disk.desc[idx[1]].addr = ((uint32)b->data) & 0xffffffff;
  disk.desc[idx[1]].len = BSIZE;
  if (write)
    disk.desc[idx[1]].flags = 0; // device reads b->data
  else
    disk.desc[idx[1]].flags = VRING_DESC_F_WRITE; // device writes b->data
  disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
  disk.desc[idx[1]].next = idx[2];

  disk.info[idx[0]].status = 0xff; // device writes 0 on success
  disk.desc[idx[2]].addr = (uint32)&disk.info[idx[0]].status;
  disk.desc[idx[2]].len = 1;
  disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // device writes the status
  disk.desc[idx[2]].next = 0;

  // record struct buf for virtio_disk_intr().
  b->disk = 1;
  disk.info[idx[0]].b = b;

  // tell the device the first index in our chain of descriptors.
  disk.avail->ring[disk.avail->idx % NUM] = idx[0];

  __sync_synchronize();

  // tell the device another avail ring entry is available.
  disk.avail->idx += 1; // not % NUM ...

  __sync_synchronize();

  *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value is queue number

  // Wait for virtio_disk_intr() to say request has finished.
  while (b->disk == 1)
  {
  }

  disk.info[idx[0]].b = 0;
  free_chain(idx[0]);

```

當 Descriptor 被填充完畢，`*R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0;` 會提醒 VIRTIO 接收我們的 Block request。

此外，`while (b->disk == 1)` 可以確保作業系統收到 Virtio 發出的外部中斷後再繼續執行下面的程式碼。

## 實作 VirtIO 的 ISR

當系統程式接收到外部中斷，會根據 IRQ Number 判斷中斷是由哪一個外部設備發起的 (VirtIO, UART...)。

```cpp
void external_handler()
{
  int irq = plic_claim();
  if (irq == UART0_IRQ)
  {
    lib_isr();
  }
  else if (irq == VIRTIO_IRQ)
  {
    lib_puts("Virtio IRQ\n");
    virtio_disk_isr();
  }
  else if (irq)
  {
    lib_printf("unexpected interrupt irq = %d\n", irq);
  }

  if (irq)
  {
    plic_complete(irq);
  }
}
```

如果是 VirtIO 發起的中斷，便會轉派給 `virtio_disk_isr()` 進行處理。

```cpp
void virtio_disk_isr()
{

  // the device won't raise another interrupt until we tell it
  // we've seen this interrupt, which the following line does.
  // this may race with the device writing new entries to
  // the "used" ring, in which case we may process the new
  // completion entries in this interrupt, and have nothing to do
  // in the next interrupt, which is harmless.
  *R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;

  __sync_synchronize();

  // the device increments disk.used->idx when it
  // adds an entry to the used ring.

  while (disk.used_idx != disk.used->idx)
  {
    __sync_synchronize();
    int id = disk.used->ring[disk.used_idx % NUM].id;

    if (disk.info[id].status != 0)
      panic("virtio_disk_intr status");

    struct blk *b = disk.info[id].b;
    b->disk = 0; // disk is done with buf
    disk.used_idx += 1;
  }

}
```

`virtio_disk_isr()` 主要工作會將 disk 的狀態改下，告訴系統先前發出的讀寫操作已經被順利執行了。
其中 `b->disk = 0;`，可以讓先前提到的 `while (b->disk == 1)` 順利跳出，釋放 disk 中的自旋鎖。

```cpp
int os_main(void)
{
	os_start();
	disk_read();
	int current_task = 0;
	while (1)
	{
		lib_puts("OS: Activate next task\n");
		task_go(current_task);
		lib_puts("OS: Back to OS\n");
		current_task = (current_task + 1) % taskTop; // Round Robin Scheduling
		lib_puts("\n");
	}
	return 0;
}
```

由於 mini-riscv-os 並沒有實作能夠休眠的鎖，所以筆者將 `disk_read()` 這個測試函式在開機時執行一次，若要向上實現更高層的檔案系統，就會需要使用 sleep lock，以避免當有多個任務嘗試取用硬碟資源時造成 deadlock 的情況發生。

## Reference

- [xv6-riscv](https://github.com/mit-pdos/xv6-riscv)
- [Lecture: Virtual I/O Protocol Operating Systems Stephen Marz](https://web.eecs.utk.edu/~smarz1/courses/cosc361/notes/virtio/)
- [Implementing a virtio-blk driver in my own operating system](https://brennan.io/2020/03/22/sos-block-device/)
- [xv6-rv32](https://github.com/riscv2os/xv6-rv32?fbclid=IwAR3eeG5jjIrpHM8Rh_0VdaZEikoEEtoIdDHZnx8CxxhqAcE89R0oZQoGaEY)
