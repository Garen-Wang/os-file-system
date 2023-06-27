#ifndef OS_COURSE_DESIGN_FILE_H
#define OS_COURSE_DESIGN_FILE_H

#include "common.h"
class File {
public:
  int inode_id = -1;
  char filename[MAX_FILENAME_SIZE]{};

  explicit File() = default;
};

#endif // OS_COURSE_DESIGN_FILE_H
