#pragma once

#include <fstream>
#include <set>
#include <string>

#include "PatriciaNode.hpp"
#include <set>
#include <vector>

using namespace std;

class PatriciaTree {
private:
  PatriciaNode *raiz; // raiz da árvore
  void imprimirNo(PatriciaNode *no, const string &contextoAcum = "") const;
  size_t numNos;
  bool podarNo(PatriciaNode *no);

public:
  PatriciaTree();
  ~PatriciaTree();
  PatriciaNode *buscarContexto(const string &contexto) const;
  void liberarMemoria(PatriciaNode *no);
  void inserirContexto(const string &contexto, unsigned char simbolo);
  void podar(set<unsigned char> &simbolosVistos);
  void resetar();
};
