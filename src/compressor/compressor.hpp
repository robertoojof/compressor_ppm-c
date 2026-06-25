#pragma once

#include "../ppm-c/arithmetic/ArithmeticCoder.hpp"
#include "../ppm-c/ppm/PatriciaTree.hpp"

#include <fstream>
#include <set>
#include <string>
#include <vector>

using namespace std;

class Compressor {
public:
  void encoder_pure(std::string &message,
                    const std::string &compressed_filename, int kmax);
  void encode();
  void decode_pure(const string &compressed_filename,
                    const string &output_filename);
};