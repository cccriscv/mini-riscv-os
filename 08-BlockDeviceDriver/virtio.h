#include "types.h"
/*
 * References:
 * 1. https://github.com/ianchen0119/xv6-riscv/blob/riscv/kernel/virtio.h
 * 2. https://github.com/sgmarz/osblog/blob/master/risc_v/src/virtio.rs
 */
#define VIRTIO_MMIO_BASE 0x10001000
/* OFFSET */
#define VIRTIO_MMIO_MAGIC_VALUE 0x000 // Magic value must be 0x74726976
#define VIRTIO_MMIO_VERSION 0x004     // Version: 1 (Legacy)

/*
 * Device ID:
 * 1 (Network Device)
 * 2 (Block Device)
 * 4 (Random number generator Device)
 * 16 (GPU Device)
 * 18 (Input Device)
 */

#define VIRTIO_MMIO_DEVICE_ID 0x008
#define VIRTIO_MMIO_VENDOR_ID 0x00c // 0x554d4551
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028  // page size for PFN, write-only
#define VIRTIO_MMIO_QUEUE_SEL 0x030        // select queue, write-only
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034    // max size of current queue, read-only
#define VIRTIO_MMIO_QUEUE_NUM 0x038        // size of current queue, write-only
#define VIRTIO_MMIO_QUEUE_ALIGN 0x03c      // used ring alignment, write-only
#define VIRTIO_MMIO_QUEUE_PFN 0x040        // physical page number for queue, read/write
#define VIRTIO_MMIO_QUEUE_READY 0x044      // ready bit
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050     // write-only
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060 // read-only
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064    // write-only
#define VIRTIO_MMIO_STATUS 0x070           // read/write
#define VIRTIO_MMIO_QueueDescLow 0x080
#define VIRTIO_MMIO_QueueDescHigh 0x084
#define VIRTIO_MMIO_QueueAvailLow 0x090
#define VIRTIO_MMIO_QueueAvailHigh 0x094
#define VIRTIO_MMIO_QueueUsedLow 0x0a0
#define VIRTIO_MMIO_QueueUsedHigh 0x0a4

// status register bits, from qemu virtio_config.h
#define VIRTIO_CONFIG_S_ACKNOWLEDGE 1
#define VIRTIO_CONFIG_S_DRIVER 2
#define VIRTIO_CONFIG_S_DRIVER_OK 4
#define VIRTIO_CONFIG_S_FEATURES_OK 8

// device feature bits
#define VIRTIO_BLK_F_RO 5          /* Disk is read-only */
#define VIRTIO_BLK_F_SCSI 7        /* Supports scsi command passthru */
#define VIRTIO_BLK_F_CONFIG_WCE 11 /* Writeback mode available in config */
#define VIRTIO_BLK_F_MQ 12         /* support more than one vq */
#define VIRTIO_F_ANY_LAYOUT 27
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX 29

// this many virtio descriptors.
#define NUM 8

// 描述符包含這些訊息: 地址，地址長度，某些標誌和其他信息。使用此描述符，我們可以將設備指向 RAM 中任何緩衝區的內存地址。
typedef struct virtq_desc
{
  /*
   * addr: 我們可以在 64-bit 內存地址內的任何位置告訴設備存儲位置。
   * len: 讓 Device 知道有多少內存可用。
   * flags: 用於控制 descriptor 。
   * next: 告訴 Device 下一個描述符的 Index 。如果指定了 VIRTQ_DESC_F_NEXT， Device 僅讀取該字段。否則無效。
   */
  uint64 addr;
  uint32 len;
  uint16 flags;
  uint16 next;
} virtq_desc_t;

#define VRING_DESC_F_NEXT 1     // chained with another descriptor
#define VRING_DESC_F_WRITE 2    // device writes (vs read)
#define VRING_DESC_F_INDIRECT 4 // buffer contains a list of buffer descriptors

/*
 * 用來存放 descriptor 的索引，當 Device 收到通知時，它會檢查 AvailableRing 確認需要讀取哪些 Descriptor 。
 * 注意: Descriptor 和 AvailableRing 都存儲在 RAM 中。
 */

typedef struct virtq_avail
{
  uint16 flags;     // always zero
  uint16 idx;       // driver will write ring[idx] next
  uint16 ring[NUM]; // descriptor numbers of chain heads
} virtq_avail_t;

/*
 * 當內部的 Index 與 UsedRing 的 Index 相等，代表所有資料都已經被讀取，這個 Device 是唯一需要被寫進 Index 的。
 */

typedef struct virtq_used_elem
{
  uint32 id; // index of start of completed descriptor chain
  uint32 len;
} virtq_used_elem_t;

/*
 * Device 可以使用 UsedRing 向 OS 發送訊息。
 * Device 通常使用它來告知 OS 它已完成先前通知的請求。
 * AvailableRing 與 UsedRing 非常相似，差異在於: OS 需要查看 UsedRing 得知哪個 Descriptor 已經被服務。
 */

typedef struct virtq_used
{
  uint16 flags; // always zero
  uint16 idx;   // device increments when it adds a ring[] entry
  struct virtq_used_elem ring[NUM];
} virtq_used_t;

// these are specific to virtio block devices, e.g. disks,
// described in Section 5.2 of the spec.

#define VIRTIO_BLK_T_IN 0  // read the disk
#define VIRTIO_BLK_T_OUT 1 // write the disk

// the format of the first descriptor in a disk request.
// to be followed by two more descriptors containing
// the block, and a one-byte status.
typedef struct virtio_blk_req
{
  uint32 type;     // VIRTIO_BLK_T_IN or ..._OUT
  uint32 reserved; // 將 Header 擴充到 16-byte ，並將 64-bit sector 移到正確的位置。
  uint64 sector;
} virtio_blk_req_t;