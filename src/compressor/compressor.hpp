#pragma once
#include <string>

class Compressor {
public:
  void encoder_master_with_reset(std::string &message,
                                 const std::string &compressed_filename,
                                 const std::string &log_filename, int kmax,
                                 size_t window_size, double threshold);
  void decode_master_reset(const std::string &compressed_filename,
                           const std::string &output_filename);
};