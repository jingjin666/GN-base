#ifndef __MMAP_H
#define __MMAP_H

#define MAP_SHARED     0x01
#define MAP_PRIVATE    0x02
#define MAP_SHARED_VALIDATE 0x03
#define MAP_TYPE       0x0f
#define MAP_FIXED      0x10
#define MAP_ANON       0x20
#define MAP_ANONYMOUS  MAP_ANON
#define MAP_NORESERVE  0x4000
#define MAP_GROWSDOWN  0x0100
#define MAP_DENYWRITE  0x0800
#define MAP_EXECUTABLE 0x1000
#define MAP_LOCKED     0x2000
#define MAP_POPULATE   0x8000
#define MAP_NONBLOCK   0x10000
#define MAP_STACK      0x20000
#define MAP_HUGETLB    0x40000
#define MAP_SYNC       0x80000
#define MAP_FIXED_NOREPLACE 0x100000
#define MAP_FILE       0

#define MAP_HUGE_SHIFT 26
#define MAP_HUGE_MASK  0x3f
#define MAP_HUGE_16KB  (14 << 26)
#define MAP_HUGE_64KB  (16 << 26)
#define MAP_HUGE_512KB (19 << 26)
#define MAP_HUGE_1MB   (20 << 26)
#define MAP_HUGE_2MB   (21 << 26)
#define MAP_HUGE_8MB   (23 << 26)
#define MAP_HUGE_16MB  (24 << 26)
#define MAP_HUGE_32MB  (25 << 26)
#define MAP_HUGE_256MB (28 << 26)
#define MAP_HUGE_512MB (29 << 26)
#define MAP_HUGE_1GB   (30 << 26)
#define MAP_HUGE_2GB   (31 << 26)
#define MAP_HUGE_16GB  (34U << 26)

#define PROT_NONE      0
#define PROT_READ      1
#define PROT_WRITE     2
#define PROT_EXEC      4
#define PROT_GROWSDOWN 0x01000000
#define PROT_GROWSUP   0x02000000

#define MS_ASYNC       1
#define MS_INVALIDATE  2
#define MS_SYNC        4

#define MCL_CURRENT    1
#define MCL_FUTURE     2
#define MCL_ONFAULT    4

#define POSIX_MADV_NORMAL     0
#define POSIX_MADV_RANDOM     1
#define POSIX_MADV_SEQUENTIAL 2
#define POSIX_MADV_WILLNEED   3
#define POSIX_MADV_DONTNEED   4

unsigned long sys_brk(unsigned long brk);
unsigned long sys_mmap(unsigned long start, unsigned long len, unsigned long prot, unsigned long flags, unsigned long fd, unsigned long off);
unsigned long sys_munmap(unsigned long addr, unsigned long len);

#endif
