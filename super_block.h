#ifndef OS_COURSE_DESIGN_SUPER_BLOCK_H
#define OS_COURSE_DESIGN_SUPER_BLOCK_H

#include "common.h"
class SuperBlock {
public:
  int system_size = SYSTEM_SIZE;
  int block_size = BLOCK_SIZE;
  int num_block = SYSTEM_SIZE / BLOCK_SIZE;
  int addr_length = ADDR_LENGTH;
  int max_filename_size = MAX_FILENAME_SIZE;
  int super_block_size = SUPER_BLOCK_SIZE;
  int inode_size = INODE_SIZE;
  int inode_bitmap_size = INODE_BITMAP_SIZE;
  int block_bitmap_size = BLOCK_BITMAP_SIZE;
  int inode_table_size = INODE_TABLE_SIZE;

  explicit SuperBlock() = default;
};

#endif // OS_COURSE_DESIGN_SUPER_BLOCK_H
