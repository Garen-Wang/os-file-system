#ifndef OS_COURSE_DESIGN_INODE_H
#define OS_COURSE_DESIGN_INODE_H

#include <ctime>
class INode {
public:
  static constexpr unsigned int NUM_DIR_ADDR = 10;
  static constexpr unsigned int NUM_INDIR_ADDR = 1;

  int id = -1;      // -1: illegal, start from 0
  int filesize = 0; // meaningful when filemode = 0
  int filemode = 0; // 1: d-entry, 0: file
  int count = 0;    // current count
  int mcount = 0;   // maximum count

  time_t ctime = 0;
  int dir_addrs[NUM_DIR_ADDR]{};
  int indir_addrs[NUM_INDIR_ADDR]{};

  explicit INode() = default;
  INode(const INode &inode);
  void clear();
};

#endif // OS_COURSE_DESIGN_INODE_H
