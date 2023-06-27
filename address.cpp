#include "address.h"

int Address::get_block_id() {
  int addr = (addrs[0] << 16) | (addrs[1] << 8) | addrs[2];
  return addr >> 10;
}
int Address::get_offset() {
  int addr = (addrs[0] << 16) | (addrs[1] << 8) | addrs[2];
  return addr & ((1 << 10) - 1);
}
void Address::set_block_id(int id) {
  int x[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int temp = id;
  unsigned char tempid[3];
  for (int i = 0; temp != 0; i++) {
    if (temp % 2 == 1) {
      x[i + 2] = 1;
    } else {
      x[i + 2] = 0;
    }
    temp = temp / 2;
  }
  for (int i = 0; i < 8; i++) {
    tempid[0] = (tempid[0] << 1) | x[15 - i];
  }
  for (int i = 0; i < 8; i++) {
    tempid[1] = (tempid[1] << 1) | x[7 - i];
  }
  unsigned char AND_ITEM2 = (1 << 1) | 1;
  addrs[0] = tempid[0];
  addrs[1] = tempid[1] | (addrs[1] & AND_ITEM2);
}
void Address::set_offset(int offset) {
  int x[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int temp = offset;
  unsigned char tempid[3];
  for (int i = 0; temp != 0; i++) {
    if (temp % 2 == 1) {
      x[i] = 1;
    } else {
      x[i] = 0;
    }
    temp = temp / 2;
  }
  for (int i = 0; i < 8; i++) {
    tempid[1] = (tempid[1] << 1) | x[15 - i];
  }
  for (int i = 0; i < 8; i++) {
    tempid[2] = (tempid[2] << 1) | x[7 - i];
  }
  unsigned char AND_ITEM6 =
      ((((((1 << 1) | 1) << 1 | 1) << 1 | 1) << 1 | 1) << 1 | 1) << 2;
  addrs[2] = tempid[2];
  addrs[1] = tempid[1] | (addrs[1] & AND_ITEM6);
}
