#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class ReadData {
public:
  void readFile(std::string &message, const std::string &filename);
};