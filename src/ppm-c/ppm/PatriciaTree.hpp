#pragma once

#include <fstream>
#include <string>

#include "PatriciaNode.hpp"

using namespace std;

class PatriciaTree {
private:
  PatriciaNode *raiz; // raiz da árvore
  void imprimirNo(PatriciaNode *no, const string &contextoAcum = "") const;
  size_t numNos;

public:
  PatriciaTree();
  ~PatriciaTree();
  PatriciaNode *buscarContexto(const string &contexto) const;
  void liberarMemoria(PatriciaNode *no);
  void inserirContexto(const string &contexto, unsigned char simbolo);
  PatriciaNode *buscarContexto(const string &contexto) const;
};
