#include "compressor.hpp"

using namespace std;
namespace fs = filesystem;

void Compressor::encoder_multi(const vector<pair<string, string>> &files,
                               const int KMAX, const string &output_filename) {
  PatriciaTree arvore;
  vector<unsigned char> historico;
  set<unsigned char> simbolosVistos;

  string nomeArquivoSaida = output_filename + ".bin";

  ofstream output_file(nomeArquivoSaida, ios::binary);

  // Cabeçalho: K (1 byte) + num_files (4 bytes)
  uint8_t k_byte = static_cast<uint8_t>(KMAX);
  output_file.write(reinterpret_cast<const char *>(&k_byte), sizeof(uint8_t));
  uint32_t num_files = static_cast<uint32_t>(files.size());
  output_file.write(reinterpret_cast<const char *>(&num_files),
                    sizeof(uint32_t));

  // Metadados por arquivo: name_len (2 bytes) + name + size (4 bytes)
  for (const auto &f : files) {
    uint16_t name_len = static_cast<uint16_t>(f.first.size());
    output_file.write(reinterpret_cast<const char *>(&name_len),
                      sizeof(uint16_t));
    output_file.write(f.first.c_str(), name_len);
    uint32_t file_size = static_cast<uint32_t>(f.second.size());
    output_file.write(reinterpret_cast<const char *>(&file_size),
                      sizeof(uint32_t));
  }

  BitOutputStream bit_output(output_file);
  ArithmeticEncoder encoder(32, bit_output);

  // Itera sobre os arquivos diretamente, sem concatenar em uma string única
  for (const auto &f : files) {
    for (size_t fi = 0; fi < f.second.size(); fi++) {
      unsigned char simbolo = static_cast<unsigned char>(f.second[fi]);
      set<unsigned char> simbolosExcluidos;

      int hsize = static_cast<int>(historico.size());
      int maxLen = min(KMAX, hsize);
      bool codificado = false;

      for (int len = maxLen; len >= 0; len--) {
        string contexto;
        contexto.reserve(len);
        for (int j = hsize - len; j < hsize; j++)
          contexto.push_back(static_cast<char>(historico[j]));

        PatriciaNode *no = arvore.buscarContexto(contexto);

        if (no != nullptr) {
          vector<uint32_t> freqs;
          if (simbolosVistos.empty()) {
            freqs.resize(ALFABETO, 0);
            freqs.push_back(1);
          } else {
            freqs = no->freq;
            if (no->distintos < ALFABETO)
              freqs.push_back(no->distintos);
          }

          for (unsigned char s : simbolosExcluidos) {
            if (freqs[s] > 0)
              freqs[s] = 0;
          }

          SimpleFrequencyTable freq_table(freqs);

          if (no->freq[simbolo] > 0) {
            encoder.write(freq_table, static_cast<uint32_t>(simbolo));
            codificado = true;
            break;
          } else {
            encoder.write(freq_table, ESC_SYMBOL);
            for (int s = 0; s < ALFABETO; s++) {
              if (no->freq[s] > 0)
                simbolosExcluidos.insert(static_cast<unsigned char>(s));
            }
          }
        }
      }

      if (!codificado) {
        vector<uint32_t> freqs_equal(ALFABETO, 0);
        for (int s = 0; s < ALFABETO; s++) {
          if (simbolosVistos.find(static_cast<unsigned char>(s)) ==
              simbolosVistos.end())
            freqs_equal[s] = 1;
        }
        SimpleFrequencyTable freq_table(freqs_equal);
        encoder.write(freq_table, static_cast<uint32_t>(simbolo));
      }

      simbolosVistos.insert(simbolo);

      for (int len = 0; len <= maxLen; len++) {
        string contexto;
        contexto.reserve(len);
        for (int j = hsize - len; j < hsize; j++)
          contexto.push_back(static_cast<char>(historico[j]));
        arvore.inserirContexto(contexto, simbolo);
      }

      historico.push_back(simbolo);
      if (static_cast<int>(historico.size()) > KMAX)
        historico.erase(historico.begin());
    }
  }

  encoder.finish();
  output_file.close();
}

void Compressor::decode_multi(const string &compressed_filename,
                              const string &output_dir) {
  PatriciaTree arvore;
  vector<unsigned char> historico;
  set<unsigned char> simbolosVistos;

  ifstream input_file(compressed_filename, ios::binary);

  uint8_t k_byte;
  input_file.read(reinterpret_cast<char *>(&k_byte), sizeof(uint8_t));
  int kmax = static_cast<int>(k_byte);

  uint32_t num_files;
  input_file.read(reinterpret_cast<char *>(&num_files), sizeof(uint32_t));

  vector<string> names;
  vector<uint32_t> sizes;
  uint32_t total_size = 0;

  for (uint32_t i = 0; i < num_files; i++) {
    uint16_t name_len;
    input_file.read(reinterpret_cast<char *>(&name_len), sizeof(uint16_t));
    string name(name_len, '\0');
    input_file.read(&name[0], name_len);
    uint32_t file_size;
    input_file.read(reinterpret_cast<char *>(&file_size), sizeof(uint32_t));
    names.push_back(name);
    sizes.push_back(file_size);
    total_size += file_size;
  }

  // monta um caminho de arquivo combinando output_dir com name
  auto make_path = [&](const string &name) -> string {
    return output_dir.empty() || output_dir == "." ? name
                                                   : output_dir + "/" + name;
  };

  if (!output_dir.empty() && output_dir != ".")
    fs::create_directories(output_dir);

  BitInputStream bit_input(input_file);
  ArithmeticDecoder decoder(32, bit_input);

  uint32_t file_idx = 0;
  uint32_t bytes_written = 0;
  ofstream output_file(make_path(names[0]), ios::binary);

  for (uint32_t i = 0; i < total_size; i++) {
    set<unsigned char> simbolosExcluidos;
    int hsize = static_cast<int>(historico.size());
    int maxLen = min(kmax, hsize);

    unsigned char simbolo = 0;
    bool decodificado = false;

    for (int len = maxLen; len >= 0; len--) {
      string contexto;
      contexto.reserve(len);
      for (int j = hsize - len; j < hsize; j++)
        contexto.push_back(static_cast<char>(historico[j]));

      PatriciaNode *no = arvore.buscarContexto(contexto);

      if (no != nullptr) {
        vector<uint32_t> freqs;
        if (simbolosVistos.empty()) {
          freqs.resize(ALFABETO, 0);
          freqs.push_back(1);
        } else {
          freqs = no->freq;
          if (no->distintos < ALFABETO)
            freqs.push_back(no->distintos);
        }

        for (unsigned char s : simbolosExcluidos) {
          if (freqs[s] > 0)
            freqs[s] = 0;
        }

        SimpleFrequencyTable freq_table(freqs);
        uint32_t decoded_symbol = decoder.read(freq_table);

        if (decoded_symbol == ESC_SYMBOL) {
          for (int s = 0; s < ALFABETO; s++) {
            if (no->freq[s] > 0)
              simbolosExcluidos.insert(static_cast<unsigned char>(s));
          }
          continue;
        } else {
          simbolo = static_cast<unsigned char>(decoded_symbol);
          decodificado = true;
          break;
        }
      }
    }

    if (!decodificado) {
      vector<uint32_t> freqs_equal(ALFABETO, 0);
      for (int s = 0; s < ALFABETO; s++) {
        if (simbolosVistos.find(static_cast<unsigned char>(s)) ==
            simbolosVistos.end())
          freqs_equal[s] = 1;
      }
      SimpleFrequencyTable freq_table(freqs_equal);
      uint32_t decoded_symbol = decoder.read(freq_table);
      simbolo = static_cast<unsigned char>(decoded_symbol);
    }

    simbolosVistos.insert(simbolo);

    output_file.put(static_cast<char>(simbolo));
    bytes_written++;

    if (file_idx < num_files && bytes_written >= sizes[file_idx]) {
      output_file.close();
      cout << "Extraido: " << make_path(names[file_idx]) << " ("
           << sizes[file_idx] << " bytes)" << endl;
      file_idx++;
      bytes_written = 0;
      if (file_idx < num_files)
        output_file.open(make_path(names[file_idx]), ios::binary);
    }

    for (int len = 0; len <= maxLen; len++) {
      string contexto;
      contexto.reserve(len);
      for (int j = hsize - len; j < hsize; j++)
        contexto.push_back(static_cast<char>(historico[j]));
      arvore.inserirContexto(contexto, simbolo);
    }

    historico.push_back(simbolo);
    if (static_cast<int>(historico.size()) > kmax)
      historico.erase(historico.begin());
  }

  input_file.close();
}
