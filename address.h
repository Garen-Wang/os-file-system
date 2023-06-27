#ifndef OS_COURSE_DESIGN_ADDRESS_H
#define OS_COURSE_DESIGN_ADDRESS_H

// 24 bit
// 10 bit offset
// 14 bit block_id
class Address {
public:
  unsigned char addrs[3]{};

  int get_block_id();
  int get_offset();
  void set_block_id(int id);
  void set_offset(int offset);
};

#endif // OS_COURSE_DESIGN_ADDRESS_H
