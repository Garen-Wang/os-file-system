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
    auto command = split_command(input);
    if (command.empty())
      continue;
    if (command[0] == "createFile") {
      if (command.size() == 3) {
        auto filesize = std::stoi(command[2]);
        fs.print_result(fs.create_file(command[1], filesize));
      } else {
        std::cerr << "createFile command in wrong format." << std::endl;
      }
    } else if (command[0] == "deleteFile") {
      if (command.size() == 2) {
        fs.print_result(fs.delete_file(command[1]));
      } else {
        std::cerr << "deleteFile command in wrong format." << std::endl;
      }
    } else if (command[0] == "createDir") {
      if (command.size() == 2) {
        fs.print_result(fs.create_dir(command[1]));
      } else {
        std::cerr << "createDir command in wrong format." << std::endl;
      }
    } else if (command[0] == "deleteDir") {
      if (command.size() == 2) {
        fs.print_result(fs.delete_dir(command[1]));
      } else {
        std::cerr << "deleteDir command in wrong format." << std::endl;
      }
    } else if (command[0] == "changeDir") {
      if (command.size() == 2) {
        fs.print_result(fs.change_dir(command[1]));
      } else {
        std::cerr << "changeDir command in wrong format." << std::endl;
      }
    } else if (command[0] == "dir") {
      if (command.size() == 1) {
        fs.print_result(fs.list_dir_contents());
      } else {
        std::cerr << "dir command in wrong format." << std::endl;
      }
    } else if (command[0] == "cp") {
      if (command.size() == 3) {
        fs.print_result(fs.copy(command[1], command[2]));
      } else {
        std::cerr << "cp command in wrong format." << std::endl;
      }
    } else if (command[0] == "sum") {
      if (command.size() == 1) {
        fs.print_result(fs.sum());
      } else {
        std::cerr << "sum command in wrong format." << std::endl;
      }
    } else if (command[0] == "cat") {
      if (command.size() == 2) {
        fs.print_result(fs.cat(command[1]));
      } else {
        std::cerr << "cat command in wrong format." << std::endl;
      }
    } else if (command[0] == "exit") {
      if (command.size() == 1) {
        break;
      } else {
        std::cerr << "exit command in wrong format." << std::endl;
      }
    } else {
      std::cerr << command[0] << ": command not found" << std::endl;
    }
  }
  return 0;
}
