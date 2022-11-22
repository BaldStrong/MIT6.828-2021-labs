// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BUCKETCOUNT 13
uint extern ticks;

struct {
  // struct spinlock lock;
  struct spinlock lock[BUCKETCOUNT];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[BUCKETCOUNT];
} bcache;

// 根据请求的blockno，查看如果他被缓存的话，会在哪个哈希桶中
uint keyToIndex(const uint key) {
  return key % BUCKETCOUNT;
}


void
binit(void) {
  struct buf* b;

  // initlock(&bcache.lock, "bcache");

  for (int i = 0; i < BUCKETCOUNT; ++i) {
    initlock(&bcache.lock[i], "bcache.bucket");
  }

  bcache.head[0].next = &bcache.buf[0];
  // Create linked list of buffers
  // 初始化所有buf
  // 这里需要-1
  for (b = bcache.buf; b < bcache.buf + NBUF - 1; b++) {
    b->next = b + 1;
    initsleeplock(&b->lock, "buffer");
  }

}
int can_lock(int id, int j) {
  int num = BUCKETCOUNT / 2;
  if (id <= num) {
    if (j > id && j <= (id + num))
      return 0;
  } else {
    if ((id < j && j < BUCKETCOUNT) || (j <= (id + num) % BUCKETCOUNT)) {
      return 0;
    }
  }
  return 1;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno) {
  struct buf* b;

  // Is the block already cached?
  int id = keyToIndex(blockno);
  acquire(&bcache.lock[id]);

  for (b = bcache.head[id].next; b; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      if (holding(&bcache.lock[id])) release(&bcache.lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // 利用ticks，使用LRU算法，找出最久最少使用的block cache
  uint min_ticks = __UINT32_MAX__;
  int last_bucket_index = -1;

  for (int i = 0; i < BUCKETCOUNT; ++i) {
    if (i != id && can_lock(id, i)) {
      acquire(&bcache.lock[i]);
    } else if (!can_lock(id, i)) {
      continue;
    }

    // 找到下一个可以加锁遍历的桶,开始遍历，从中找到ticks最小且引用计数为0的block
    for (b = bcache.head[i].next; b; b = b->next) {
      if (b->refcnt == 0) {
        if (b->ticks < min_ticks) {
          min_ticks = b->ticks;
          if (last_bucket_index != -1 && i != last_bucket_index && holding(&bcache.lock[last_bucket_index])) {
            release(&bcache.lock[last_bucket_index]);
          }
          last_bucket_index = i;
        }
      }
    }
    // 释放掉刚刚遍历的桶锁，如果持有锁的话
    if (i != id && i != last_bucket_index && holding(&bcache.lock[i])) release(&bcache.lock[i]);
  }
  // 所有哈希桶遍历结束
  // 如果没找到可用来驱赶的block cache，panic
  if (last_bucket_index == -1) panic("bget: no buffers");

  // 遍历目标桶，找到最小ticks的block，从链表中去掉
  // 此处注意，由于没有prev,所以只能通过判断下一个块是否是要被替换的，
  for (b = &bcache.head[last_bucket_index]; b->next; b = b->next) {
    // printf("%p\t%p\n", b, b->next);
    if ((b->next)->refcnt == 0 && (b->next)->ticks == min_ticks) {
      // 把目标block保存下来，然后从该桶里面去除
      // res指向了b->next，b->next指向了b->next->next
      struct buf* res = b->next;
      b->next = b->next->next;

      if (holding(&bcache.lock[last_bucket_index])) release(&bcache.lock[last_bucket_index]);

      // 找到hash id所属桶的末尾，把目标block链接到最后，代表最新的block
      struct buf* id_b = &bcache.head[id];
      while (id_b->next) {
        id_b = id_b->next;
      }

      id_b->next = res;
      res->next = 0;// 这样可以保证下次找到链表尾部
      id_b->dev = dev;
      id_b->blockno = blockno;
      id_b->valid = 0;
      id_b->refcnt = 1;
      if (holding(&bcache.lock[id])) release(&bcache.lock[id]);
      acquiresleep(&id_b->lock);
      return id_b;
    }
  }
  return 0;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
  bread(uint dev, uint blockno) {
  struct buf* b;

  b = bget(dev, blockno);
  if (!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf* b) {
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf* b) {
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int id = keyToIndex(b->blockno);

  acquire(&bcache.lock[id]);
  b->refcnt--;
  // 当块被释放时，记录ticks，代表块最后被使用的时间
  if (b->refcnt == 0) {
    b->ticks = ticks;
  }
  release(&bcache.lock[id]);
}

void
bpin(struct buf* b) {
  int id = keyToIndex(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt++;
  release(&bcache.lock[id]);
}

void
bunpin(struct buf* b) {
  int id = keyToIndex(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  release(&bcache.lock[id]);
}