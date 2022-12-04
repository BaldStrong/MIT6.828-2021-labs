#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200
#define O_TRUNC   0x400
// 00 01 10 即二进制表示倒数第一位和第二位都必须是0才行，因为采用的按位OR操作
#define O_NOFOLLOW 0x010
