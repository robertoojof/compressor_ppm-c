#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "../../config/config.hpp"

using namespace std;

struct PatriciaNode {
  string prefixo;        // segmento compactado
  vector<uint32_t> freq; // contagens por símbolo
  uint32_t distintos;    // nº de símbolos distintos
  map<unsigned char, PatriciaNode *>
      filhos; // map de filhos do nó, indentificado pelo primeiro caractere do
              // contexto

  // construtor - inicializa o nó atual
  PatriciaNode(string p = "")
      : prefixo(move(p)), freq(ALFABETO, 0), distintos(0) {}
};
