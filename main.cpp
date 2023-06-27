#include "fs.h"

#include <iostream>

void print_welcome_msg() {
  std::cout << "Operating Systems Module Practice 2023" << std::endl;
  std::cout << "Group members: " << std::endl
            << "王樾 202030430189" << std::endl
            << "黄嘉威 20203043????" << std::endl
            << "姜融青 2020????????" << std::endl;
}

std::vector<std::string> split_command(std::string input) {
  size_t pos;
  std::string token;
  std::vector<std::string> vec;
  while ((pos = input.find(' ')) != std::string::npos) {
    token = input.substr(0, pos);
    vec.push_back(token);
    input.erase(0, pos + 1);
  }
  if (!input.empty()) {
    vec.push_back(input);
  }
  return std::move(vec);
}

int main() {
  srand(time(nullptr));
  // print_welcome_msg();
  FileSystem fs;
  fs.init();
  std::string input;
  while (true) {
    fs.init_print();
    std::getline(std::cin, input);
    auto cmd = split_command(input);
    if (cmd.empty())
      continue;
    if (cmd[0] == "createFile") {
      if (cmd.size() == 3) {
        auto filesize = std::stoi(cmd[2]);
        fs.print_result(fs.create_file(cmd[1], filesize));
      } else {
        std::cerr << "createFile cmd in wrong format." << std::endl;
      }
    } else if (cmd[0] == "deleteFile") {
      if (cmd.size() == 2) {
        fs.print_result(fs.delete_file(cmd[1]));
      } else {
        std::cerr << "deleteFile cmd in wrong format." << std::endl;
      }
    } else if (cmd[0] == "createDir") {
      if (cmd.size() == 2) {
        fs.print_result(fs.create_dir(cmd[1]));
      } else {
        std::cerr << "createDir cmd in wrong format." << std::endl;
      }
    } else if (cmd[0] == "deleteDir") {
      if (cmd.size() == 2) {
        fs.print_result(fs.delete_dir(cmd[1]));
      } else {
        std::cerr << "deleteDir cmd in wrong format." << std::endl;
      }
    } else if (cmd[0] == "changeDir") {
      if (cmd.size() == 2) {
        fs.print_result(fs.change_dir(cmd[1]));
      } else {
        std::cerr << "changeDir cmd in wrong format." << std::endl;
      }
    } else if (cmd[0] == "dir") {
      if (cmd.size() == 1) {
        fs.print_result(fs.list_dir_contents());
      } else {
        std::cerr << "dir cmd in wrong format." << std::endl;
      }
    } else if (cmd[0] == "cp") {
      if (cmd.size() == 3) {
        fs.print_result(fs.copy(cmd[1], cmd[2]));
      } else {
        std::cerr << "cp cmd in wrong format." << std::endl;
      }
    } else if (cmd[0] == "sum") {
      if (cmd.size() == 1) {
        fs.print_result(fs.sum());
      } else {
        std::cerr << "sum cmd in wrong format." << std::endl;
      }
    } else if (cmd[0] == "cat") {
      if (cmd.size() == 2) {
        fs.print_result(fs.cat(cmd[1]));
      } else {
        std::cerr << "cat cmd in wrong format." << std::endl;
      }
    } else if (cmd[0] == "exit") {
      if (cmd.size() == 1) {
        break;
      } else {
        std::cerr << "exit cmd in wrong format." << std::endl;
      }
    } else {
      std::cerr << cmd[0] << ": cmd not found" << std::endl;
    }
  }
  return 0;
}
