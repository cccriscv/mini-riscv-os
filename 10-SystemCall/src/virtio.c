/*  This Code derived from xv6-riscv (64bit)
 *  -- https://github.com/mit-pdos/xv6-riscv/
 */

#include "virtio.h"
#include "os.h"

#define R(addr) ((volatile uint32 *)(VIRTIO_MMIO_BASE + (addr)))
#define BSIZE 512 // block size
#define PGSHIFT 12

struct blk
{
  uint32 dev;
  uint32 blockno;
  lock_t lock;
  int disk;
  unsigned char data[BSIZE];
};

static struct disk
{
  char pages[2 * PGSIZE];
  /* descriptor */
  virtq_desc_t *desc;
  /* AvailableRing */
  virtq_avail_t *avail;
  /* UsedRing */
  virtq_used_t *used;
  /* For decord each descriptor is free or not */
  char free[NUM];
  struct
  {
    struct blk *b;
    char status;
  } info[NUM];
  uint16 used_idx;
  /* Disk command headers */
  virtio_blk_req_t ops[NUM];

  struct lock vdisk_lock;
} __attribute__((aligned(PGSIZE))) disk;

struct blk b[3];

void virtio_tester(int write)
{
  if (!b[0].dev)
  {
    lib_puts("buffer init...\n");
    for (size_t i = 0; i < 1; i++)
    {
      b[i].dev = 1; // always is 1
      b[i].blockno = 1;
      for (size_t j = 0; j < BSIZE; j++)
      {
        b[i].data[j] = 0;
      }

      lock_init(&(b[i].lock));
    }
  }

  lib_puts("block read...\n");
  virtio_disk_rw(&b[0], write);
  size_t i = 0;
  for (; i < 10; i++)
  {
    lib_printf("%x \n", b[0].data[i]);
  }
  lib_puts("\n");
}

void virtio_disk_init()
{
  uint32 status = 0;

  lock_init(&disk.vdisk_lock);

  if (*R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
      *R(VIRTIO_MMIO_VERSION) != 1 ||
      *R(VIRTIO_MMIO_DEVICE_ID) != 2 ||
      *R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551)
  {
    panic("could not find virtio disk");
  }
  /* Reset the device */
  *R(VIRTIO_MMIO_STATUS) = status;
  /* Set the ACKNOWLEDGE status bit to the status register. */
  status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
  *R(VIRTIO_MMIO_STATUS) = status;
  /* Set the DRIVER status bit to the status register. */
  status |= VIRTIO_CONFIG_S_DRIVER;
  *R(VIRTIO_MMIO_STATUS) = status;
  /* negotiate features */
  uint32 features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
  features &= ~(1 << VIRTIO_BLK_F_RO);
  features &= ~(1 << VIRTIO_BLK_F_SCSI);
  features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
  features &= ~(1 << VIRTIO_BLK_F_MQ);
  features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
  features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
  features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
  *R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

  /* tell device that feature negotiation is complete. */
  status |= VIRTIO_CONFIG_S_FEATURES_OK;
  *R(VIRTIO_MMIO_STATUS) = status;

  /* tell device we're completely ready. */
  status |= VIRTIO_CONFIG_S_DRIVER_OK;
  *R(VIRTIO_MMIO_STATUS) = status;

  *R(VIRTIO_MMIO_GUEST_PAGE_SIZE) = PGSIZE;
  /* initialize queue 0. */
  *R(VIRTIO_MMIO_QUEUE_SEL) = 0;
  uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
  if (max == 0)
    panic("virtio disk has no queue 0\n");
  if (max < NUM)
    panic("virtio disk max queue too short\n");
  *R(VIRTIO_MMIO_QUEUE_NUM) = NUM;
  memset(disk.pages, 0, sizeof(disk.pages));
  *R(VIRTIO_MMIO_QUEUE_PFN) = ((uint32)disk.pages) / PGSIZE;
  *R(VIRTIO_MMIO_QUEUE_ALIGN) = PGSIZE;
  *R(VIRTIO_MMIO_QUEUE_READY) = 1;
  // desc = pages -- num * virtq_desc
  // avail = pages + 0x40 -- 2 * uint16, then num * uint16
  // used = pages + 4096 -- 2 * uint16, then num * vRingUsedElem
  disk.desc = (struct virtq_desc *)disk.pages;
  disk.avail = (struct virtq_avail *)(disk.pages + NUM * sizeof(virtq_desc_t));
  disk.used = (struct virtq_used *)(disk.pages + PGSIZE);

  *R(VIRTIO_MMIO_QueueDescLow) = disk.desc;
  *R(VIRTIO_MMIO_QueueAvailLow) = disk.avail;
  *R(VIRTIO_MMIO_QueueUsedLow) = disk.used;

  // all NUM descriptors start out unused.
  for (int i = 0; i < NUM; i++)
    disk.free[i] = 1;
  lib_puts("Disk init work is success!\n");
}

// find a free descriptor, mark it non-free, return its index.
static int
alloc_desc()
{
  for (int i = 0; i < NUM; i++)
  {
    if (disk.free[i])
    {
      disk.free[i] = 0;
      return i;
    }
  }
  return -1;
}

// mark a descriptor as free.
static void
free_desc(int i)
{
  if (i >= NUM)
    panic("free_desc 1");
  if (disk.free[i])
    panic("free_desc 2");
  disk.desc[i].addr = 0;
  disk.desc[i].len = 0;
  disk.desc[i].flags = 0;
  disk.desc[i].next = 0;
  disk.free[i] = 1;
}

// free a chain of descriptors.
static void
free_chain(int i)
{
  while (1)
  {
    int flag = disk.desc[i].flags;
    int nxt = disk.desc[i].next;
    free_desc(i);
    if (flag & VRING_DESC_F_NEXT)
      i = nxt;
    else
      break;
  }
}

// allocate three descriptors (they need not be contiguous).
// disk transfers always use three descriptors.
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

void virtio_disk_rw(struct blk *b, int write)
{
  uint64 sector = b->blockno * (BSIZE / 512);

  lock_acquire(&disk.vdisk_lock);

  // allocate the three descriptors.
  int idx[3];
  while (1)
  {
    if (alloc3_desc(idx) == 0)
    {
      break;
    }
  }

  // format the three descriptors.
  // qemu's virtio-blk.c reads them.

  virtio_blk_req_t *buf0 = &disk.ops[idx[0]];

  if (write)
    buf0->type = VIRTIO_BLK_T_OUT; // write the disk
  else
    buf0->type = VIRTIO_BLK_T_IN; // read the disk
  buf0->reserved = 0;             // The reserved portion is used to pad the header to 16 bytes and move the 32-bit sector field to the correct place.
  buf0->sector = sector;          // specify the sector that we wanna modified.

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

  disk.info[idx[0]].status = 0;
  disk.desc[idx[2]].addr = (uint32)&disk.info[idx[0]].status;
  disk.desc[idx[2]].len = 1;
  disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // device writes the status
  disk.desc[idx[2]].next = 0;

  // record struct buf for virtio_disk_intr().
  b->disk = 1;
  disk.info[idx[0]].b = b;

  __sync_synchronize();

  // tell the device the first index in our chain of descriptors.
  disk.avail->ring[disk.avail->idx % NUM] = idx[0];

  __sync_synchronize();

  // tell the device another avail ring entry is available.
  disk.avail->idx += 1; // not % NUM ...

  *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // The device starts immediately when we write 0 to queue_notify.
  while (b->disk == 1)
  {
  }

  disk.info[idx[0]].b = 0;
  free_chain(idx[0]);

  lock_free(&disk.vdisk_lock);
}

void virtio_disk_isr()
{
  //lock_acquire(&disk.vdisk_lock);

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
    b->disk = 0;
    disk.used_idx += 1;
  }

  //lock_free(&disk.vdisk_lock);
}