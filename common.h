#ifndef OS_COURSE_DESIGN_COMMON_H
#define OS_COURSE_DESIGN_COMMON_H

constexpr int SYSTEM_SIZE = 16 * 1024 * 1024; // 16 MiB
constexpr int BLOCK_SIZE = 1 * 1024;          // 1 KiB
constexpr int NUM_BLOCK = SYSTEM_SIZE / BLOCK_SIZE;

constexpr int ADDR_LENGTH = 24;

constexpr int SUPER_BLOCK_SIZE = 1 * 1024;
constexpr int INODE_BITMAP_SIZE = 2 * 1024;
constexpr int BLOCK_BITMAP_SIZE = 2 * 1024;
constexpr int INODE_TABLE_SIZE = 2 * 1024 * 1024;
constexpr int INODE_SIZE = 128;

// SUPER_BLOCK, INODE_BITMAP, BLOCK_BITMAP, INODE_TABLE
constexpr int SUPER_BLOCK_ADDR = 0;
constexpr int INODE_BITMAP_ADDR = SUPER_BLOCK_ADDR + SUPER_BLOCK_SIZE;
constexpr int BLOCK_BITMAP_ADDR = INODE_BITMAP_ADDR + INODE_BITMAP_SIZE;
constexpr int INODE_TABLE_ADDR = BLOCK_BITMAP_ADDR + BLOCK_BITMAP_SIZE;

constexpr int NUM_INODE_BITMAP_BLOCK = INODE_BITMAP_SIZE / BLOCK_SIZE;
constexpr int NUM_BLOCK_BITMAP_BLOCK = BLOCK_BITMAP_SIZE / BLOCK_SIZE;
constexpr int NUM_TABLE_BLOCK = INODE_TABLE_SIZE / BLOCK_SIZE;

constexpr int MAX_FILENAME_SIZE = 20;

constexpr int FILEMODE_DENTRY = 1;
constexpr int FILEMODE_FILE = 0;

#endif // OS_COURSE_DESIGN_COMMON_H