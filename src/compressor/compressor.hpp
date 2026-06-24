#pragma once
#include <string>

using namespace std;

class Compressor {
public:
  void encoder_master_with_reset(string &message,
                                 const string &compressed_filename,
                                 const string &log_filename, int kmax,
                                 size_t window_size, double threshold);
  void decode_master_reset(const string &compressed_filename,
                           const string &output_filename);
};