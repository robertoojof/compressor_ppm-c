#pragma once

#include <fstream>
#include <string>

#include "PatriciaNode.hpp"

class PatriciaTree {
private:
  PatriciaNode *raiz; // raiz da árvore
  void imprimirNo(PatriciaNode *no, const std::string &contextoAcum = "") const;
  size_t numNos;

public:
  PatriciaTree();
  ~PatriciaTree();
  PatriciaNode *buscarContexto(const std::string &contexto) const;
  void liberarMemoria(PatriciaNode *no);
  void inserirContexto(const std::string &contexto, unsigned char simbolo);
  PatriciaNode *buscarContexto(const std::string &contexto) const;
};
