#include "compressor.hpp"

using namespace std;

void Compressor::encoder_pure(string &message,
                              const string &compressed_filename, int kmax) {
  PatriciaTree arvore;
  vector<unsigned char> historico;
  set<unsigned char> simbolosVistos;

  // Arquivo de saída e log
  ofstream output_file("output.bin", ios::binary);
  ofstream log_file("compressor_log.txt");

  // grava cabeçalho: K (1 byte) + tamanho original (4 bytes)
  uint8_t k_byte = static_cast<uint8_t>(kmax);
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
    int maxLen = std::min(kmax, hsize); // tamanho maximo do contexto atual

    // tentar contextos de KMAX até 0 (percorre os contextos em ordem
    // decrescente)
    bool codificado = false;

    // tentar contextos de KMAX até 0 (percorre os contextos em ordem
    // decrescente)
    for (int len = maxLen; len >= 0; len--) {
      std::string contexto;
      contexto.reserve(len);
      for (int j = hsize - len; j < hsize; j++) {
        contexto.push_back(static_cast<char>(historico[j]));
      }

      // procura contexto na árvore
      PatriciaNode *no = arvore.buscarContexto(contexto);

      // se contexto foi encontrado
      if (no != nullptr) {
        std::vector<std::uint32_t> freqs;
        if (i == 0) {
          freqs.resize(ALFABETO, 0);
          freqs.push_back(1);
        } else {
          freqs = no->freq;
        }

        // remove símbolos que foram excluídos em contextos maiores
        for (unsigned char s : simbolosExcluidos) {
          if (freqs[s] > 0) {
            freqs[s] = 0;
          }
        }

        // se há símbolos não vistos, inclui o ESC
        if (no->distintos < ALFABETO)
          freqs.push_back(no->distintos);       // frequencia de ESC equivale ao
                                                // número de símbolos existentes
        SimpleFrequencyTable freq_table(freqs); // cria atual frequency table

        // se símbolo está no contexto
        if (no->freq[simbolo] > 0) {
          // std::cout << "Codificando simbolo no contexto K=" << len <<
          // std::endl; codifica o símbolo
          // *** Informação se símbolo está no contexto ***
          double p = static_cast<double>(freq_table.get(simbolo)) /
                     freq_table.getTotal();

          /* soma_informacao += -std::log2(p); */

          encoder.write(freq_table, static_cast<std::uint32_t>(simbolo));
          codificado = true;
          break;
        } else {
          // *** Informação do ESC ***
          double pEsc = static_cast<double>(freq_table.get(ESC_SYMBOL)) /
                        freq_table.getTotal();

          /* soma_informacao += -std::log2(pEsc); */

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
      std::vector<std::uint32_t> freqs_equal(ALFABETO, 0);
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

      /* soma_informacao += -std::log2(p); */

      encoder.write(freq_table, static_cast<std::uint32_t>(simbolo));
    }

    simbolosVistos.insert(simbolo);

    // atualizar a árvore
    for (int len = 0; len <= maxLen; len++) {
      std::string contexto;
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

void Compressor::encode() {}

void Compressor::decode_reset(const string &compressed_filename,
                              const string &output_filename) {}
