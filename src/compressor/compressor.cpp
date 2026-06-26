#include "compressor.hpp"

using namespace std;

void Compressor::encoder_pure(string &message,
                              const string &compressed_filename,
                              const int KMAX) {
  PatriciaTree arvore;
  vector<unsigned char> historico;
  set<unsigned char> simbolosVistos;

  // Arquivo de saída e log
  ofstream output_file("output.bin", ios::binary);
  ofstream log_file("compressor_log.txt");

  // grava cabeçalho: K (1 byte) + tamanho original (4 bytes)
  uint8_t k_byte = static_cast<uint8_t>(KMAX);
  output_file.write(reinterpret_cast<const char *>(&k_byte), sizeof(uint8_t));
  uint32_t original_size = static_cast<uint32_t>(message.size());
  output_file.write(reinterpret_cast<const char *>(&original_size),
                    sizeof(uint32_t));

  // Inicializa o fluxo de saída de bits
  BitOutputStream bit_output(output_file);

  // Inicializa o codificador aritmético
  ArithmeticEncoder encoder(32, bit_output);

  // percorre cada símbolo da mensagem
  for (size_t i = 0; i < message.length(); i++) {
    unsigned char simbolo = message[i];

    // registra símbolos a serem excluídos, que já sabemos que não irão aparecer
    set<unsigned char> simbolosExcluidos;

    // variáveis para limitar o tamanho do contexto
    int hsize = static_cast<int>(historico.size()); // tamanho do historico
    int maxLen = min(KMAX, hsize); // tamanho maximo do contexto atual

    // tentar contextos de KMAX até 0 (percorre os contextos em ordem
    // decrescente)
    bool codificado = false;

    // tentar contextos de KMAX até 0 (percorre os contextos em ordem
    // decrescente)
    for (int len = maxLen; len >= 0; len--) {
      string contexto;
      contexto.reserve(len);
      for (int j = hsize - len; j < hsize; j++) {
        contexto.push_back(static_cast<char>(historico[j]));
      }

      // procura contexto na árvore
      PatriciaNode *no = arvore.buscarContexto(contexto);

      // se contexto foi encontrado
      if (no != nullptr) {
        vector<uint32_t> freqs;
        if (i == 0) {
          freqs.resize(ALFABETO, 0);
          freqs.push_back(1);
        } else {
          freqs = no->freq;
          if (no->distintos < ALFABETO)
            freqs.push_back(no->distintos);
        }

        // remove símbolos que foram excluídos em contextos maiores
        for (unsigned char s : simbolosExcluidos) {
          if (freqs[s] > 0) {
            freqs[s] = 0;
          }
        }
        SimpleFrequencyTable freq_table(freqs); // cria atual frequency table

        // se símbolo está no contexto
        if (no->freq[simbolo] > 0) {
          // cout << "Codificando simbolo no contexto K=" << len <<
          // endl; codifica o símbolo
          // *** Informação se símbolo está no contexto ***
          double p = static_cast<double>(freq_table.get(simbolo)) /
                     freq_table.getTotal();

          /* soma_informacao += -log2(p); */

          encoder.write(freq_table, static_cast<uint32_t>(simbolo));
          codificado = true;
          break;
        } else {
          // *** Informação do ESC ***
          double pEsc = static_cast<double>(freq_table.get(ESC_SYMBOL)) /
                        freq_table.getTotal();

          /* soma_informacao += -log2(pEsc); */

          encoder.write(freq_table, ESC_SYMBOL);

          // adiciona todos os símbolos deste contexto ao conjunto de excluídos
          for (int s = 0; s < ALFABETO; s++) {
            if (no->freq[s] > 0) {
              simbolosExcluidos.insert(static_cast<unsigned char>(s));
            }
          }
        }
      }
    }

    // se não foi possível codificar, usa equiprobabilidades
    if (!codificado) {
      // Cria equiprobabilidade apenas para símbolos não vistos
      vector<uint32_t> freqs_equal(ALFABETO, 0);
      int simbolosNaoVistos = 0;

      for (int s = 0; s < ALFABETO; s++) {
        if (simbolosVistos.find(static_cast<unsigned char>(s)) ==
            simbolosVistos.end()) {
          freqs_equal[s] = 1;
          simbolosNaoVistos++;
        }
      }

      SimpleFrequencyTable freq_table(freqs_equal);

      double p =
          static_cast<double>(freq_table.get(simbolo)) / freq_table.getTotal();

      /* soma_informacao += -log2(p); */

      encoder.write(freq_table, static_cast<uint32_t>(simbolo));
    }

    simbolosVistos.insert(simbolo);

    // atualizar a árvore
    for (int len = 0; len <= maxLen; len++) {
      string contexto;
      contexto.reserve(len);
      for (int j = hsize - len; j < hsize; j++) {
        contexto.push_back(static_cast<char>(historico[j]));
      }
      arvore.inserirContexto(contexto, simbolo);
    }

    // administra tamanho do histórico para não estourar a memória
    historico.push_back(simbolo);
    if (static_cast<int>(historico.size()) > KMAX)
      historico.erase(historico.begin());
  }

  /* double informacao_media = soma_informacao / message.size(); */

  // Limpeza
  encoder.finish();
  output_file.close();
  log_file.close();
}

void Compressor::decode_pure(const string &compressed_filename,
                             const string &output_filename) {
  PatriciaTree arvore;
  vector<unsigned char> historico;
  set<unsigned char> simbolosVistos;

  ifstream input_file(compressed_filename, ios::binary);

  // Lê cabeçalho básico: K (1 byte) + tamanho original (4 bytes)
  uint8_t k_byte;
  input_file.read(reinterpret_cast<char *>(&k_byte), sizeof(uint8_t));
  int kmax = static_cast<int>(k_byte);

  uint32_t original_size;
  input_file.read(reinterpret_cast<char *>(&original_size), sizeof(uint32_t));

  // Salva posição atual (após ler K + size)
  streampos pos_apos_header_basico = input_file.tellg();

  // Tenta ler window_size
  // uint32_t tentativa_ws;
  // input_file.read(reinterpret_cast<char *>(&tentativa_ws), sizeof(uint32_t));

  // Heurística: window_size válido está entre 100 e 100000
  // Valores típicos são 1000, 5000, 10000
  // if (tentativa_ws >= 100 && tentativa_ws <= 100000) {
  //   formato_novo = true;
  //   window_size = tentativa_ws;
  //   // Já está na posição correta
  // } else {
  //   formato_novo = false;
  //   window_size = 0; // Não usado no formato antigo
  //   // Volta para a posição após header básico
  //   input_file.seekg(pos_apos_header_basico);
  // }

  input_file.seekg(pos_apos_header_basico);

  BitInputStream bit_input(input_file);
  ArithmeticDecoder decoder(32, bit_input);
  string decoded_message;
  int num_resets = 0;

  // Tabela para flag de reset (só usada se formato_novo)
  vector<uint32_t> freqs_flag(2, 1);
  SimpleFrequencyTable flag_table(freqs_flag);

  for (uint32_t i = 0; i < original_size; i++) {

    // No início de cada janela (após a 2ª), lê flag de reset (SÓ se formato
    // novo)
    // if (formato_novo && i > 0 && i % window_size == 0 && i >= 2 *
    // window_size) {
    //   uint32_t flag = decoder.read(flag_table);

    //   if (flag == 1) // RESET
    //   {
    //     delete arvore;
    //     arvore = new PatriciaTree();
    //     historico.clear();
    //     simbolosVistos.clear();
    //     num_resets++;
    //   }
    // }

    set<unsigned char> simbolosExcluidos;
    int hsize = static_cast<int>(historico.size());
    int maxLen = min(kmax, hsize);

    // Verifica limite de memória (mesma lógica do codificador)
    // if (arvore.getNumNos() > MAX_NODES) {
    //   delete arvore;
    //   arvore = new PatriciaTree();
    //   historico.clear();
    //   simbolosVistos.clear();
    //   hsize = 0;
    //   maxLen = 0;
    // }

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
    decoded_message += static_cast<char>(simbolo);

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

  ofstream output_file(output_filename, ios::binary);
  output_file.write(decoded_message.c_str(), decoded_message.size());
  output_file.close();
  // delete arvore;
}

void Compressor::encoder_multi(const vector<pair<string, string>> &files,
                               const int KMAX) {
  PatriciaTree arvore;
  vector<unsigned char> historico;
  set<unsigned char> simbolosVistos;

  // Concatena conteúdo de todos os arquivos
  string message;
  for (const auto &f : files)
    message += f.second;

  ofstream output_file("output.bin", ios::binary);

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

  for (size_t i = 0; i < message.length(); i++) {
    unsigned char simbolo = message[i];
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
        if (i == 0) {
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

  BitInputStream bit_input(input_file);
  ArithmeticDecoder decoder(32, bit_input);
  string decoded_message;

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
    decoded_message += static_cast<char>(simbolo);

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

  size_t offset = 0;
  for (uint32_t i = 0; i < num_files; i++) {
    string out_path = output_dir.empty() || output_dir == "."
                          ? names[i]
                          : output_dir + "/" + names[i];
    ofstream out(out_path, ios::binary);
    out.write(decoded_message.c_str() + offset, sizes[i]);
    out.close();
    cout << "Extraido: " << out_path << " (" << sizes[i] << " bytes)" << endl;
    offset += sizes[i];
  }
}
