#ifndef OS_COURSE_DESIGN_FS_H
#define OS_COURSE_DESIGN_FS_H

#include "address.h"
#include "file.h"
#include "inode.h"
#include "super_block.h"

#include <cstdio>
#include <string>

enum ResultCode {
  SUCCESS = 0,
  DIR_NOT_EXIST,
  FILENAME_LENGTH_EXCEEDED,
  DIR_EXCEEDED, // when creating file or dir
  NOT_ENOUGH_SPACE,
  NO_FILE_NAME,
  NO_DIR_NAME,
  NO_SUCH_FILE,
  NO_SUCH_DIR,
  DIR_NOT_EMPTY,
  FILE_EXIST, // when creating file
  DIR_EXIST,  // when creating dir
  CANNOT_DELETE_TEMP_DIR,
};

static constexpr int MAX_PATH = 1000;

class FileSystem {
public:
  char cur_path[MAX_PATH];
  FILE *f = nullptr;
  INode *root_inode = nullptr;
  INode *cur_inode = nullptr;
  SuperBlock sb;

private:
  void set_block_bitmap(int n) const;
  void set_inode_bitmap(int n) const;
  void unset_block_bitmap(int n) const;
  void unset_inode_bitmap(int n) const;
  [[nodiscard]] INode *read_inode(int n) const;
  INode *get_next_inode(INode *inode, const std::string &filename);

  int allocate_unused_inode() const;
  int allocate_unused_block() const;
  void write_random_to_block(int block_id) const;
  void write_addr_to_block(Address addr, int block_id, int offset) const;

  int get_unused_block_num() const;

  void write_file_to_dentry(File file, INode *inode);
  void write_inode(int pos, INode *inode) const;

  void delete_file_from_dentry(INode *inode, const std::string &filename);

public:
  explicit FileSystem() = default;
  ~FileSystem();
  void init();

  ResultCode create_file(std::string path, int filesize);
  ResultCode create_dir(std::string path);
  ResultCode delete_file(std::string path);
  ResultCode delete_dir(std::string path);

  ResultCode change_dir(std::string path);
  ResultCode list_dir_contents();

  ResultCode copy(std::string src_path, std::string dest_path);
  ResultCode cat(std::string path);
  ResultCode sum();

  void init_print();
  void print_result(ResultCode ret);
};

#endif // OS_COURSE_DESIGN_FS_H
