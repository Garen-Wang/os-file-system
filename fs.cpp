#include "fs.h"
#include "address.h"
#include "file.h"

#include <unistd.h>
#include <cstring>

#include <fstream>
#include <iostream>
#include <vector>

std::string get_filemode_name(int filemode) {
  if (filemode == FILEMODE_DENTRY)
    return "dir";
  if (filemode == FILEMODE_FILE)
    return "file";
  return "unknown";
}

std::vector<std::string> split_path(std::string path) {
  size_t pos;
  std::string token;
  std::vector<std::string> vec;
  if (path[0] == '/') {
    path = path.substr(1);
  }
  while ((pos = path.find('/')) != std::string::npos) {
    token = path.substr(0, pos);
    vec.push_back(token);
    path.erase(0, pos + 1);
  }
  if (!path.empty()) {
    vec.push_back(path);
  }
  return std::move(vec);
}

// SUPER_BLOCK, INODE_BITMAP, BLOCK_BITMAP, INODE_TABLE
FileSystem::~FileSystem() { fclose(f); }

void FileSystem::init() {
  std::string os = "unix.os";
  std::fstream fstream;
  fstream.open(os, std::ios::in);
  if (!fstream) {
    // file not exists
    // Opens a file to update both reading and writing. The file must exist.
    f = fopen(os.c_str(), "wb+");
    if (f == nullptr) {
      std::cerr << "Error when creating" << std::endl;
      exit(1);
    }
    static char buffer[SYSTEM_SIZE];
    fwrite(buffer, SYSTEM_SIZE, 1, f); // create 16 MiB

    fseek(f, 0, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, f); // write super block

    fseek(f, INODE_BITMAP_ADDR, SEEK_SET);
    static char empty_buf = 0;
    fwrite(&empty_buf, sizeof(char), INODE_BITMAP_SIZE, f);

    fseek(f, BLOCK_BITMAP_ADDR, SEEK_SET); // optional
    fwrite(&empty_buf, sizeof(char), BLOCK_BITMAP_SIZE, f);

    set_block_bitmap(0); // super block
    for (int i = 0; i < NUM_INODE_BITMAP_BLOCK; ++i) {
      set_block_bitmap(i + 1);
    }
    for (int i = 0; i < NUM_BLOCK_BITMAP_BLOCK; ++i) {
      set_block_bitmap(i + 1 + NUM_INODE_BITMAP_BLOCK);
    }
    for (int i = 0; i < NUM_TABLE_BLOCK; ++i) {
      set_block_bitmap(i + 1 + NUM_INODE_BITMAP_BLOCK + NUM_BLOCK_BITMAP_BLOCK);
    }

    root_inode = new INode();
    root_inode->id = 0;
    root_inode->ctime = time(nullptr);
    root_inode->filemode = FILEMODE_DENTRY;

    set_inode_bitmap(0); // root inode
    cur_inode = root_inode;
  } else {
    // file exists
    f = fopen(os.c_str(), "rb+");
    if (f == nullptr) {
      std::cerr << "error when loading" << std::endl;
      exit(1);
    }

    fseek(f, 0, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, f);
    root_inode = cur_inode = read_inode(0);
  }
  strcpy(cur_path, "~");
}

ResultCode FileSystem::list_dir_contents() {
  static const int NUM_FILES_OF_BLOCK = sb.block_size / sizeof(File);
  cur_inode = read_inode(cur_inode->id);
  for (int i = 0, cnt = cur_inode->count; i < INode::NUM_DIR_ADDR && cnt > 0;
       ++i) {
    for (int j = 0; j < NUM_FILES_OF_BLOCK && cnt > 0; ++j, --cnt) {
      int pos = cur_inode->dir_addrs[i] * BLOCK_SIZE + sizeof(File) * j;
      fseek(f, pos, SEEK_SET);
      File file;
      fread(&file, sizeof(File), 1, f);
      if (file.inode_id != -1) {
        auto inode = read_inode(file.inode_id);
        char time_buffer[32];
        std::strftime(time_buffer, 32, "%a, %d.%m.%Y %H:%M:%S",
                      std::localtime(&inode->ctime));
        std::cout << "name: " << file.filename << ", ctime: " << time_buffer
                  << ", type: " << get_filemode_name(inode->filemode)
                  << ", size (KiB): " << inode->filesize << std::endl;
      }
    }
  }
  return SUCCESS;
}
ResultCode FileSystem::create_file(std::string path, int filesize) {
  auto inode = path[0] == '/' ? root_inode : cur_inode;
  std::vector<std::string> vec = split_path(path);
  if (vec.empty() || vec.back().empty())
    return NO_FILE_NAME;
  if (vec.back().length() >= MAX_FILENAME_SIZE)
    return FILENAME_LENGTH_EXCEEDED;

  for (int i = 0; i < vec.size() - 1; ++i) {
    auto next_inode = get_next_inode(inode, vec[i]);
    if (next_inode == nullptr)
      return DIR_NOT_EXIST;
    if (next_inode->filemode != FILEMODE_DENTRY)
      return DIR_NOT_EXIST;
    inode = next_inode;
  }
  auto next_inode = get_next_inode(inode, vec.back());
  if (next_inode != nullptr)
    return FILE_EXIST;

  const int MAX_DIR_NUM = sb.block_size / sizeof(File) * INode::NUM_DIR_ADDR;
  if (inode->count >= MAX_DIR_NUM)
    return DIR_EXCEEDED;

  int unused = get_unused_block_num();
  auto max_file_size = 10 + sb.block_size / sizeof(Address); // 351
  if (filesize > unused || filesize > max_file_size)
    return NOT_ENOUGH_SPACE;

  File file;
  int new_inode_id = get_unused_inode_id();
  if (new_inode_id == -1)
    return NOT_ENOUGH_SPACE;
  auto new_inode = new INode();
  new_inode->filesize = filesize;
  new_inode->filemode = FILEMODE_FILE;
  file.inode_id = new_inode->id = new_inode_id;
  new_inode->ctime = time(nullptr);
  set_inode_bitmap(file.inode_id);
  strcpy(file.filename, vec.back().c_str());

  write_file_to_dentry(file, inode);

  int remaining_size = filesize;
  for (int i = 0; i < INode::NUM_DIR_ADDR && remaining_size > 0;
       ++i, --remaining_size) {
    new_inode->dir_addrs[i] = get_unused_block_id();
    set_block_bitmap(new_inode->dir_addrs[i]);
    write_random_to_block(new_inode->dir_addrs[i]);
  }
  if (remaining_size > 0) {
    new_inode->indir_addrs[0] = get_unused_block_id();
    set_block_bitmap(new_inode->indir_addrs[0]);
    int cnt = 0;
    while (remaining_size--) {
      int block_id = get_unused_block_id();
      set_block_bitmap(block_id);
      Address addr;
      addr.set_block_id(block_id);
      addr.set_offset(0);
      write_random_to_block(block_id);
      write_addr_to_block(addr, new_inode->indir_addrs[0], cnt++);
    }
  }
  write_inode(new_inode->id, new_inode);
  return SUCCESS;
}

ResultCode FileSystem::create_dir(std::string path) {
  auto inode = path[0] == '/' ? root_inode : cur_inode;
  std::vector<std::string> vec = split_path(path);
  if (vec.empty() || vec.back().empty())
    return NO_DIR_NAME;

  for (int i = 0; i < vec.size() - 1; ++i) {
    auto next_inode = get_next_inode(inode, vec[i]);
    if (next_inode == nullptr || next_inode->filemode != FILEMODE_DENTRY)
      return DIR_NOT_EXIST;
    inode = next_inode;
  }
  auto next_inode = get_next_inode(inode, vec.back());
  if (next_inode != nullptr)
    return DIR_EXIST;

  static const int MAX_DIR_NUM =
      sb.block_size / sizeof(File) * INode::NUM_DIR_ADDR;
  if (inode->count >= MAX_DIR_NUM)
    return DIR_EXCEEDED;

  File file;
  int new_inode_id = get_unused_inode_id();
  if (new_inode_id == -1)
    return NOT_ENOUGH_SPACE;
  auto new_inode = new INode();
  new_inode->filemode = FILEMODE_DENTRY;
  file.inode_id = new_inode->id = new_inode_id;
  new_inode->ctime = time(nullptr);
  set_inode_bitmap(file.inode_id);
  strcpy(file.filename, vec.back().c_str());

  write_file_to_dentry(file, inode);
  write_inode(new_inode->id, new_inode);
  return SUCCESS;
}

ResultCode FileSystem::change_dir(std::string path) {
  auto inode = path[0] == '/' ? root_inode : cur_inode;
  std::vector<std::string> vec = split_path(path);
  if (!vec.empty()) {
    if (vec.back().empty())
      return NO_DIR_NAME;
    for (int i = 0; i < vec.size() - 1; ++i) {
      auto next_inode = get_next_inode(inode, vec[i]);
      if (next_inode == nullptr || next_inode->filemode != FILEMODE_DENTRY)
        return DIR_NOT_EXIST;
      inode = next_inode;
    }
    auto next_inode = get_next_inode(inode, vec.back());
    if (next_inode == nullptr)
      return DIR_NOT_EXIST;
    cur_inode = next_inode;
  } else {
    cur_inode = read_inode(root_inode->id);
  }

  if (path[0] == '/') {
    strcpy(cur_path, "~");
  }
  for (const auto &v : vec) {
    strcat(cur_path, "/");
    strcat(cur_path, v.c_str());
  }
  return SUCCESS;
}
ResultCode FileSystem::copy(std::string src_path, std::string dest_path) {
  auto inode = src_path[0] == '/' ? root_inode : cur_inode;
  std::vector<std::string> vec = split_path(src_path);
  if (vec.empty() || vec.back().empty())
    return NO_FILE_NAME;
  if (vec.back().length() > MAX_FILENAME_SIZE)
    return FILENAME_LENGTH_EXCEEDED;

  for (int i = 0; i < vec.size() - 1; ++i) {
    auto next_inode = get_next_inode(inode, vec[i]);
    if (next_inode == nullptr)
      return DIR_NOT_EXIST;
    if (next_inode->filemode != FILEMODE_DENTRY)
      return DIR_NOT_EXIST;
    inode = next_inode;
  }
  auto next_inode = get_next_inode(inode, vec.back());
  if (next_inode == nullptr || next_inode->filemode == FILEMODE_DENTRY)
    return NO_SUCH_FILE;
  inode = next_inode;

  // read contents from src
  std::vector<char> contents;
  int filesize = inode->filesize;
  for (int i = 0; i < INode::NUM_DIR_ADDR && filesize > 0; ++i, --filesize) {
    int block_id = inode->dir_addrs[i];
    fseek(f, block_id * sb.block_size, SEEK_SET);
    for (int j = 0; j < BLOCK_SIZE; ++j) {
      char buffer;
      fread(&buffer, sizeof(char), 1, f);
      contents.push_back(buffer);
    }
  }
  if (filesize > 0) {
    int offset = 0;
    while (filesize--) {
      Address address;
      fseek(f, inode->indir_addrs[0] * sb.block_size + offset * sizeof(Address),
            SEEK_SET);
      fread(&address, sizeof(Address), 1, f);
      int block_id = address.get_block_id();
      fseek(f, block_id * sb.block_size, SEEK_SET);
      for (int j = 0; j < BLOCK_SIZE; ++j) {
        char buffer;
        fread(&buffer, sizeof(char), 1, f);
        contents.push_back(buffer);
      }
      offset++;
    }
  }

  // create dest file with identical size
  ResultCode ret;
  if ((ret = create_file(dest_path, inode->filesize)) != SUCCESS) {
    return ret;
  }

  // copy contents to dest file
  inode = dest_path[0] == '/' ? root_inode : cur_inode;
  vec = split_path(dest_path);
  if (vec.empty() || vec.back().empty())
    return NO_FILE_NAME;

  for (int i = 0; i < vec.size() - 1; ++i) {
    auto next_inode = get_next_inode(inode, vec[i]);
    if (next_inode == nullptr)
      return DIR_NOT_EXIST;
    if (next_inode->filemode != FILEMODE_DENTRY)
      return DIR_NOT_EXIST;
    inode = next_inode;
  }
  next_inode = get_next_inode(inode, vec.back());
  if (next_inode->filemode == FILEMODE_DENTRY)
    return NO_SUCH_FILE;
  inode = next_inode;

  filesize = inode->filesize;
  int index = 0;
  for (int i = 0; i < INode::NUM_DIR_ADDR && filesize > 0; ++i, --filesize) {
    int block_id = inode->dir_addrs[i];
    fseek(f, block_id * sb.block_size, SEEK_SET);
    for (int j = 0; j < BLOCK_SIZE; ++j) {
      char buffer = contents[index++];
      fwrite(&buffer, sizeof(char), 1, f);
    }
  }
  if (filesize > 0) {
    int offset = 0;
    while (filesize--) {
      Address address;
      fseek(f, inode->indir_addrs[0] * sb.block_size + offset * sizeof(Address),
            SEEK_SET);
      fread(&address, sizeof(Address), 1, f);
      int block_id = address.get_block_id();
      fseek(f, block_id * sb.block_size, SEEK_SET);
      for (int j = 0; j < BLOCK_SIZE; ++j) {
        char buffer = contents[index++];
        fwrite(&buffer, sizeof(char), 1, f);
      }
      offset++;
    }
  }
  return SUCCESS;
}
ResultCode FileSystem::delete_file(std::string path) {
  auto inode = path[0] == '/' ? root_inode : cur_inode;
  std::vector<std::string> vec = split_path(path);
  if (vec.empty() || vec.back().empty())
    return NO_FILE_NAME;

  for (int i = 0; i < vec.size() - 1; ++i) {
    auto next_inode = get_next_inode(inode, vec[i]);
    if (next_inode == nullptr || next_inode->filemode != FILEMODE_DENTRY)
      return NO_SUCH_DIR;
    inode = next_inode;
  }
  auto next_inode = get_next_inode(inode, vec.back());
  if (next_inode == nullptr || next_inode->filemode == FILEMODE_DENTRY)
    return NO_SUCH_FILE;
  delete_file_from_dentry(inode, vec.back());
  inode = next_inode;

  unset_inode_bitmap(inode->id);
  int filesize = inode->filesize;
  for (int i = 0; i < INode::NUM_DIR_ADDR && filesize > 0; ++i, --filesize) {
    unset_block_bitmap(inode->dir_addrs[i]);
  }
  if (filesize > 0) {
    unset_block_bitmap(inode->indir_addrs[0]);
    int offset = 0;
    while (filesize--) {
      Address addr;
      fseek(f, inode->indir_addrs[0] * sb.block_size + offset * sizeof(Address),
            SEEK_SET);
      fread(&addr, sizeof(Address), 1, f);
      unset_block_bitmap(addr.get_block_id());
      offset++;
    }
  }
  return SUCCESS;
}
ResultCode FileSystem::delete_dir(std::string path) {
  auto inode = path[0] == '/' ? root_inode : cur_inode;
  std::vector<std::string> vec = split_path(path);
  if (vec.empty() || vec.back().empty())
    return NO_DIR_NAME;

  for (int i = 0; i < vec.size() - 1; ++i) {
    auto next_inode = get_next_inode(inode, vec[i]);
    if (next_inode == nullptr || next_inode->filemode != FILEMODE_DENTRY)
      return NO_SUCH_DIR;
    inode = next_inode;
  }
  auto next_inode = get_next_inode(inode, vec.back());
  if (next_inode == nullptr || next_inode->filemode == FILEMODE_FILE)
    return NO_SUCH_DIR;

  if (path[0] == '/') {
    std::vector<std::string> cur = split_path(cur_path);
    if (cur.back().empty())
      cur.pop_back();

    if (vec.size() <= cur.size()) {
      bool ok = true;
      for (int i = 0; i < vec.size(); ++i) {
        if (vec[i] != cur[i]) {
          ok = false;
          break;
        }
      }
      if (ok)
        return CANNOT_DELETE_TEMP_DIR;
    }
  }
  if (next_inode->count > 0)
    return DIR_NOT_EMPTY;

  delete_file_from_dentry(inode, vec.back());

  inode = next_inode;
  unset_inode_bitmap(inode->id);
  int count = inode->mcount;
  int FILE_PER_BLOCK = sb.block_size / sizeof(File);
  for (int dir_addr : inode->dir_addrs) {
    if (count <= 0) {
      break;
    }
    count -= FILE_PER_BLOCK;
    unset_block_bitmap(dir_addr);
  }

  return SUCCESS;
}
ResultCode FileSystem::cat(std::string path) {
  auto inode = path[0] == '/' ? root_inode : cur_inode;
  auto vec = split_path(path);
  if (vec.empty() || vec.back().empty()) {
    return NO_FILE_NAME;
  }

  for (int i = 0; i < vec.size() - 1; ++i) {
    auto next_inode = get_next_inode(inode, vec[i]);
    if (next_inode == nullptr || next_inode->filemode != FILEMODE_DENTRY)
      return DIR_NOT_EXIST;
    inode = next_inode;
  }
  auto next_inode = get_next_inode(inode, vec.back());
  if (next_inode == nullptr || next_inode->filemode != FILEMODE_FILE)
    return NO_SUCH_FILE;
  inode = next_inode;

  int filesize = inode->filesize;
  for (int block_id : inode->dir_addrs) {
    if (filesize == 0) {
      break;
    }
    filesize--;
    fseek(f, block_id * sb.block_size, SEEK_SET);
    for (int j = 0; j < BLOCK_SIZE; ++j) {
      char buffer;
      fread(&buffer, sizeof(char), 1, f);
      std::cout << buffer;
    }
  }
  if (filesize > 0) {
    int offset = 0;
    while (filesize--) {
      Address address;
      fseek(f, inode->indir_addrs[0] * sb.block_size + offset * sizeof(Address),
            SEEK_SET);
      fread(&address, sizeof(Address), 1, f);
      int block_id = address.get_block_id();
      fseek(f, block_id * sb.block_size, SEEK_SET);
      for (int j = 0; j < BLOCK_SIZE; ++j) {
        char buffer;
        fread(&buffer, sizeof(char), 1, f);
        std::cout << buffer;
      }
      offset++;
    }
  }
  std::cout << std::endl;
  return SUCCESS;
}
ResultCode FileSystem::sum() {
  int unused = get_unused_block_num();
  int used = NUM_BLOCK - unused;

  std::cout << "system size: " << sb.system_size << " Bytes" << std::endl;
  std::cout << "block size: " << sb.block_size << " Bytes" << std::endl;
  std::cout << "inode bitmap Size: " << sb.inode_bitmap_size << " B"
            << std::endl;
  std::cout << "block bitmap Size: " << sb.inode_bitmap_size << " B"
            << std::endl;
  std::cout << "inode table Size: " << sb.inode_table_size << " B" << std::endl;
  std::cout << "# of Blocks: " << sb.num_block << std::endl;
  std::cout << "# of used Blocks: " << used << std::endl;
  std::cout << "# of unused Blocks: " << unused << std::endl;
  return SUCCESS;
}

// utility methods
int FileSystem::get_unused_inode_id() const {
  fseek(f, INODE_BITMAP_ADDR, SEEK_SET);
  int pos = -1;
  for (int i = 0; i < INODE_BITMAP_SIZE; ++i) {
    unsigned char buf;
    fread(&buf, sizeof(unsigned char), 1, f);
    for (int j = 0; j < 8; ++j) {
      if ((buf & (1 << j)) == 0) {
        pos = (i << 3) + j;
        break;
      }
    }
    if (pos != -1)
      break;
  }
  return pos;
}
void FileSystem::set_block_bitmap(int n) const {
  int num = n / 8, offset = n % 8;
  unsigned char buf;
  fseek(f, BLOCK_BITMAP_ADDR + num, SEEK_SET);
  fread(&buf, sizeof(unsigned char), 1, f);
  buf |= (1 << offset); // just mark
  fseek(f, BLOCK_BITMAP_ADDR + num, SEEK_SET);
  fwrite(&buf, sizeof(unsigned char), 1, f);
}
void FileSystem::set_inode_bitmap(int n) const {
  int num = n / 8, offset = n % 8;
  unsigned char buf;
  fseek(f, INODE_BITMAP_ADDR + num, SEEK_SET);
  fread(&buf, sizeof(unsigned char), 1, f);
  buf |= (1 << offset); // just mark
  fseek(f, INODE_BITMAP_ADDR + num, SEEK_SET);
  fwrite(&buf, sizeof(unsigned char), 1, f);
}
void FileSystem::unset_block_bitmap(int n) const {
  int num = n / 8, offset = n % 8;
  unsigned char buf;
  fseek(f, BLOCK_BITMAP_ADDR + num, SEEK_SET);
  fread(&buf, sizeof(unsigned char), 1, f);
  unsigned char mask = ~(1 << offset);
  buf &= mask; // just mark
  fseek(f, BLOCK_BITMAP_ADDR + num, SEEK_SET);
  fwrite(&buf, sizeof(unsigned char), 1, f);
}
void FileSystem::unset_inode_bitmap(int n) const {
  int num = n / 8, offset = n % 8;
  unsigned char buf;
  fseek(f, INODE_BITMAP_ADDR + num, SEEK_SET);
  fread(&buf, sizeof(unsigned char), 1, f);
  unsigned char mask = ~(1 << offset);
  buf &= mask; // just mark
  fseek(f, INODE_BITMAP_ADDR + num, SEEK_SET);
  fwrite(&buf, sizeof(unsigned char), 1, f);
}
INode *FileSystem::read_inode(int n) const {
  auto inode = new INode();
  fseek(f, INODE_TABLE_ADDR + INODE_SIZE * n, SEEK_SET);
  fread(inode, sizeof(INode), 1, f);
  return inode;
}
void FileSystem::write_inode(int pos, INode *inode) const {
  fseek(f, INODE_TABLE_ADDR + INODE_SIZE * pos, SEEK_SET);
  fwrite(inode, sizeof(INode), 1, f);
}
INode *FileSystem::get_next_inode(INode *inode, const std::string &filename) {
  int cnt = inode->mcount;
  int FILE_PER_BLOCK = sb.block_size / sizeof(File);
  for (int dir_addr : inode->dir_addrs) {
    if (cnt == 0)
      break;
    fseek(f, BLOCK_SIZE * dir_addr, SEEK_SET);
    for (int j = 0; j < FILE_PER_BLOCK && cnt > 0; ++j, --cnt) {
      File file;
      fread(&file, sizeof(File), 1, f);
      if (file.inode_id == -1)
        continue;
      if (strcmp(file.filename, filename.c_str()) == 0) {
        return read_inode(file.inode_id);
      }
    }
  }
  return nullptr;
}
int FileSystem::get_unused_block_id() const {
  fseek(f, BLOCK_BITMAP_ADDR, SEEK_SET);
  int pos = -1;
  for (int i = 0; i < BLOCK_BITMAP_SIZE; ++i) {
    unsigned char byte;
    fread(&byte, sizeof(unsigned char), 1, f);
    for (int j = 0; j < 8; ++j) {
      if ((byte & (1 << j)) == 0) {
        pos = (i << 3) + j;
        break;
      }
    }
    if (pos != -1)
      break;
  }
  return pos;
}
void FileSystem::write_random_to_block(int block_id) const {
  fseek(f, block_id * sb.block_size, SEEK_SET);
  for (int i = 0; i < sb.block_size; ++i) {
    char randomChar = (rand() % 26) + 'a';
    // std::cout << randomChar << std::endl;
    fwrite(&randomChar, sizeof(char), 1, f);
  }
}
void FileSystem::write_addr_to_block(Address addr, int block_id,
                                     int offset) const {
  fseek(f, block_id * sb.block_size + offset * sizeof(Address), SEEK_SET);
  fwrite(&addr, sizeof(Address), 1, f);
}
int FileSystem::get_unused_block_num() const {
  int cnt = 0;
  fseek(f, BLOCK_BITMAP_ADDR, SEEK_SET);
  for (int i = 0; i < BLOCK_BITMAP_SIZE; ++i) {
    unsigned char byte;
    fread(&byte, sizeof(unsigned char), 1, f);
    for (int j = 0; j < 8; ++j) {
      if ((byte & (1 << j)) == 0)
        cnt++;
    }
  }
  return cnt;
}
void FileSystem::write_file_to_dentry(File file, INode *inode) {
  int cnt = inode->count;
  const int FILE_PER_BLOCK = sb.block_size / sizeof(File);

  if (cnt == inode->mcount) {
    if (cnt % FILE_PER_BLOCK == 0) {
      inode->dir_addrs[cnt / FILE_PER_BLOCK] = get_unused_block_id();
      set_block_bitmap(inode->dir_addrs[cnt / FILE_PER_BLOCK]);
      fseek(f, BLOCK_SIZE * inode->dir_addrs[cnt / FILE_PER_BLOCK], SEEK_SET);
    } else {
      fseek(f,
            BLOCK_SIZE * inode->dir_addrs[cnt / FILE_PER_BLOCK] +
                sizeof(File) * (cnt % FILE_PER_BLOCK),
            SEEK_SET);
    }
    fwrite(&file, sizeof(File), 1, f);

    inode->count++;
    inode->mcount++;
    write_inode(inode->id, inode);
  } else {
    bool ok = false;
    int temp = inode->mcount;
    for (int dir_addr : inode->dir_addrs) {
      if (temp == 0)
        break;
      for (int j = 0; j < FILE_PER_BLOCK; ++j) {
        if (temp == 0)
          break;
        temp--;
        File tempfile;
        fseek(f, BLOCK_SIZE * dir_addr + sizeof(File) * j, SEEK_SET);
        fread(&tempfile, sizeof(File), 1, f);
        if (tempfile.inode_id == -1) {
          ok = true;
          fseek(f, BLOCK_SIZE * dir_addr + sizeof(File) * j, SEEK_SET);
          fwrite(&file, sizeof(File), 1, f);
        }
        if (ok)
          break;
      }
      if (ok)
        break;
    }

    inode->count++;
    write_inode(inode->id, inode);
  }

  if (inode->id == cur_inode->id)
    cur_inode = read_inode(cur_inode->id);
  if (inode->id == root_inode->id)
    root_inode = read_inode(root_inode->id);
}
void FileSystem::delete_file_from_dentry(INode *inode,
                                         const std::string &filename) {
  int cnt = inode->mcount;
  const int FILE_PER_BLOCK = sb.block_size / sizeof(File);
  bool ok = false;
  for (int dir_addr : inode->dir_addrs) {
    if (cnt == 0)
      break;
    fseek(f, BLOCK_SIZE * dir_addr, SEEK_SET);
    for (int j = 0; j < FILE_PER_BLOCK; ++j) {
      if (cnt == 0)
        break;
      cnt--;
      File file;
      fread(&file, sizeof(File), 1, f);
      if (file.inode_id == -1)
        continue;
      if (strcmp(file.filename, filename.c_str()) == 0) {
        fseek(f, -sizeof(File), SEEK_CUR);
        file.inode_id = -1;
        fwrite(&file, sizeof(File), 1, f);
        ok = true;
      }
      if (ok)
        break;
    }
    if (ok)
      break;
  }
  inode->count--;
  write_inode(inode->id, inode);
}
void FileSystem::init_print() { std::cout << "os:" << cur_path << " root# "; }
void FileSystem::print_result(ResultCode ret) {
  switch (ret) {
  case SUCCESS:
    break;
  case DIR_NOT_EXIST:
    std::cerr << "error: directory not exist" << std::endl;
    break;
  case FILENAME_LENGTH_EXCEEDED:
    std::cerr << "error: filename length exceeded" << std::endl;
    break;
  case DIR_EXCEEDED:
    std::cerr << "error: number of directories exceeded" << std::endl;
    break;
  case NOT_ENOUGH_SPACE:
    std::cerr << "error: storage space is not enough" << std::endl;
    break;
  case NO_FILE_NAME:
    std::cerr << "error: no file name" << std::endl;
    break;
  case NO_DIR_NAME:
    std::cerr << "error: no directory name" << std::endl;
    break;
  case NO_SUCH_FILE:
    std::cerr << "error: no such file" << std::endl;
    break;
  case NO_SUCH_DIR:
    std::cerr << "error: no such dir" << std::endl;
    break;
  case DIR_NOT_EMPTY:
    std::cerr << "error: directory not empty" << std::endl;
    break;
  case FILE_EXIST:
    std::cerr << "error: file existing" << std::endl;
    break;
  case DIR_EXIST:
    std::cerr << "error: directory existing" << std::endl;
    break;
  case CANNOT_DELETE_TEMP_DIR:
    std::cerr << "error: cannot delete temp directory" << std::endl;
    break;
  }
}