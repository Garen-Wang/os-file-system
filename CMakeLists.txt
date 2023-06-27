cmake_minimum_required(VERSION 3.25)
project(os_course_design)

set(CMAKE_CXX_STANDARD 17)

add_executable(os_course_design main.cpp inode.cpp fs.cpp inode.h fs.h super_block.h common.h file.h address.h address.cpp)
