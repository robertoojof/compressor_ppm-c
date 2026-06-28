#include "compressor.hpp"

using namespace std;
namespace fs = filesystem;

void Compressor::encoder_multi(const vector<pair<string, string>> &files,
                               const int KMAX, const string &output_filename,
                               const int j_window, const int threshold,
                               const int mode) {
  PatriciaTree arvore;
  vector<unsigned char> historico;
  set<unsigned char> simbolosVistos;
  vector<uint32_t> event_positions; // posições onde poda ou reset foram aplicados

  string nomeArquivoSaida = output_filename + ".bin";
  ofstream output_file(nomeArquivoSaida, ios::binary);

  // Cabeçalho: K (1b) + j_window (4b) + threshold (1b) + mode (1b) + num_files (4b)
  // mode: 0=nenhum  1=poda  2=reset
  uint8_t k_byte = static_cast<uint8_t>(KMAX);
  output_file.write(reinterpret_cast<const char *>(&k_byte), sizeof(uint8_t));
  uint32_t j_val = static_cast<uint32_t>(j_window);
  output_file.write(reinterpret_cast<const char *>(&j_val), sizeof(uint32_t));
  uint8_t t_val = static_cast<uint8_t>(threshold);
  output_file.write(reinterpret_cast<const char *>(&t_val), sizeof(uint8_t));
  uint8_t m_val = static_cast<uint8_t>(mode);
  output_file.write(reinterpret_cast<const char *>(&m_val), sizeof(uint8_t));
  uint32_t num_files = static_cast<uint32_t>(files.size());
  output_file.write(reinterpret_cast<const char *>(&num_files), sizeof(uint32_t));

  // Metadados por arquivo: name_len (2 bytes) + name + size (4 bytes)
  for (const auto &f : files) {
    uint16_t name_len = static_cast<uint16_t>(f.first.size());
    output_file.write(reinterpret_cast<const char *>(&name_len), sizeof(uint16_t));
    output_file.write(f.first.c_str(), name_len);
    uint32_t file_size = static_cast<uint32_t>(f.second.size());
    output_file.write(reinterpret_cast<const char *>(&file_size), sizeof(uint32_t));
  }

  BitOutputStream bit_output(output_file);
  ArithmeticEncoder encoder(32, bit_output);

  // Rastreamento de janelas para detecção de degradação
  long long prev_window_bits = 0;
  long long window_start_bits = 0;
  int window_count = 0;
  uint32_t symbol_counter = 0;

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

        if (no != nullptr && no->distintos > 0) {
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

      symbol_counter++;

      // Verificação de janela: compara comprimento médio atual com o anterior
      if (j_window > 0 && symbol_counter % static_cast<uint32_t>(j_window) == 0) {
        long long bits_now = bit_output.getTotalBitsWritten();
        long long curr_window_bits = bits_now - window_start_bits;

        if (window_count >= 1 && prev_window_bits > 0 && threshold > 0 && mode != 0) {
          // Degrada se curr/prev > (100 + threshold)/100
          if (curr_window_bits * 100 > prev_window_bits * (100 + threshold)) {
            event_positions.push_back(symbol_counter);

            if (mode == 1) {
              // ---- PODA (halving): divide todas as frequências por 2 ----
              // Contextos raros perdem evidência; os frequentes se mantêm.
              // simbolosVistos é atualizado para refletir o que ainda sobrou.
              arvore.podar(simbolosVistos);
            } else if (mode == 2) {
              // ---- RESET: apaga toda a árvore e reinicia o modelo do zero ----
              // A codificação continua no mesmo stream aritmético,
              // mas o modelo estatístico recomeça como se o arquivo começasse aqui.
              arvore.resetar();
              simbolosVistos.clear();
              historico.clear();
            }
          }
        }

        prev_window_bits = curr_window_bits;
        window_start_bits = bits_now;
        window_count++;
      }
    }
  }

  encoder.finish();

  // Footer: posições dos eventos (poda ou reset) + contagem (últimos 4 bytes)
  for (uint32_t pos : event_positions)
    output_file.write(reinterpret_cast<const char *>(&pos), sizeof(uint32_t));
  uint32_t num_events = static_cast<uint32_t>(event_positions.size());
  output_file.write(reinterpret_cast<const char *>(&num_events), sizeof(uint32_t));

  output_file.close();
}

void Compressor::decode_multi(const string &compressed_filename,
                              const string &output_dir) {
  PatriciaTree arvore;
  vector<unsigned char> historico;
  set<unsigned char> simbolosVistos;

  ifstream input_file(compressed_filename, ios::binary);

  // Lê o footer primeiro: últimos 4 bytes = num_prune_events
  set<uint32_t> prune_set;
  input_file.seekg(-static_cast<streamoff>(sizeof(uint32_t)), ios::end);
  uint32_t num_prune_events;
  input_file.read(reinterpret_cast<char *>(&num_prune_events), sizeof(uint32_t));
  if (num_prune_events > 0) {
    input_file.seekg(
        -static_cast<streamoff>((num_prune_events + 1) * sizeof(uint32_t)),
        ios::end);
    for (uint32_t e = 0; e < num_prune_events; e++) {
      uint32_t pos;
      input_file.read(reinterpret_cast<char *>(&pos), sizeof(uint32_t));
      prune_set.insert(pos);
    }
  }

  // Volta ao início para ler o cabeçalho normalmente
  input_file.seekg(0, ios::beg);

  uint8_t k_byte;
  input_file.read(reinterpret_cast<char *>(&k_byte), sizeof(uint8_t));
  int kmax = static_cast<int>(k_byte);

  // j_window e threshold não são usados pelo decoder (só pelo encoder para detectar degradação)
  uint32_t j_dummy;
  input_file.read(reinterpret_cast<char *>(&j_dummy), sizeof(uint32_t));
  uint8_t t_dummy;
  input_file.read(reinterpret_cast<char *>(&t_dummy), sizeof(uint8_t));

  // mode: 0=nenhum  1=poda  2=reset — o decoder precisa saber qual operação aplicar
  uint8_t mode_byte;
  input_file.read(reinterpret_cast<char *>(&mode_byte), sizeof(uint8_t));
  int mode = static_cast<int>(mode_byte);

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
    // Aplica poda ou reset se o encoder o fez nesta posição (lido do footer)
    if (!prune_set.empty() && prune_set.count(i)) {
      if (mode == 1) {
        // ---- PODA (halving): espelha exatamente o que o encoder fez ----
        arvore.podar(simbolosVistos);
      } else if (mode == 2) {
        // ---- RESET: zera o modelo e retoma do zero, igual ao encoder ----
        arvore.resetar();
        simbolosVistos.clear();
        historico.clear();
      }
      prune_set.erase(i);
    }

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

      if (no != nullptr && no->distintos > 0) {
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
