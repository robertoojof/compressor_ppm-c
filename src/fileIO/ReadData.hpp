#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class ReadData {
public:
  void readFile(string &message, const string &filename);
};