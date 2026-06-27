#pragma once

#include "../fileIO/ReadData.hpp"
#include "../ppm-c/arithmetic/ArithmeticCoder.hpp"
#include "../ppm-c/ppm/PatriciaTree.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace std;

class Compressor {
public:
  // Codigo morto
  // void encoder_pure(string &message, const string &compressed_filename,
  //                   const int KMAX);
  // void decode_pure(const string &compressed_filename,
  //                  const string &output_filename);

  // Funções utilizadas ~ Funcionam com multiplos arquivos
  void encoder_multi(const vector<pair<string, string>> &files, const int KMAX,
                     const string &output_filename = "output");
  void decode_multi(const string &compressed_filename,
                    const string &output_dir);
};
