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
  // mode: 0 = nenhum, 1 = poda (halving), 2 = reset completo
  // log_filename: se não vazio, grava CSV (posicao, bits_por_simbolo, evento)
  //   - amostrado a cada 1000 símbolos
  //   - eventos de poda/reset são sempre registrados com marcador na coluna "evento"
  //   - padrão quando -log sem nome: "compression_log.log"
  // retorna bits/símbolo (comprimento médio final)
  double encoder_multi(const vector<pair<string, string>> &files, const int KMAX,
                       const string &output_filename = "output",
                       const int j_window = 0, const int threshold = 0,
                       const int mode = 0,
                       const string &log_filename = "");
  void decode_multi(const string &compressed_filename,
                    const string &output_dir);
};
