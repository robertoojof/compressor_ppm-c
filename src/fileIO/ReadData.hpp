#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

class ReadData {
public:
  void readFile(string &message, const string &filename);
  static ofstream writeFile(const string &path);
};
