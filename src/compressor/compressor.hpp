#pragma once

#include "../ppm-c/arithmetic/ArithmeticCoder.hpp"
#include "../ppm-c/ppm/PatriciaTree.hpp"

#include <fstream>
#include <set>
#include <string>
#include <vector>

using namespace std;

class Compressor {
private:
  void encoder_pure(std::string &message,
                    const std::string &compressed_filename, int kmax);
  // void encoder_reset();
  // void encoder_pode();

public:
  void encode();
  void decode_pure(const string &compressed_filename,
                    const string &output_filename);
};