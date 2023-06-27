#include "inode.h"

#include <cstring>

void INode::clear() {
  id = -1;
  filesize = filemode = 0;
  ctime = 0;
  mcount = 0;
  memset(dir_addrs, 0, sizeof(dir_addrs));
  memset(indir_addrs, 0, sizeof(indir_addrs));
}

INode::INode(const INode &inode) {
  id = inode.id;
  filesize = inode.filesize;
  filemode = inode.filemode;
  ctime = inode.ctime;
  memcpy(dir_addrs, inode.dir_addrs, sizeof(dir_addrs));
  memcpy(indir_addrs, inode.indir_addrs, sizeof(indir_addrs));
}
