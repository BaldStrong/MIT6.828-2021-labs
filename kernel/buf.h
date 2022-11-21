struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  // struct buf *prev; // LRU cache list
  uint ticks;
  struct buf* next;
  uchar data[BSIZE];
};

