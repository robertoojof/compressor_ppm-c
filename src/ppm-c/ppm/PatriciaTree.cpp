#include "PatriciaTree.hpp"

PatriciaTree::PatriciaTree() {
  // cria nó vazio como raiz
  raiz = new PatriciaNode();
  numNos = 1;
}

PatriciaTree::~PatriciaTree() { liberarMemoria(raiz); }

void PatriciaTree::liberarMemoria(PatriciaNode *no) {
  if (no == nullptr)
    return;

  for (auto &filho : no->filhos) {
    liberarMemoria(filho.second);
  }
  delete no;
}

void PatriciaTree::imprimirNo(PatriciaNode *no,
                              const string &contextoAcum) const {}

void PatriciaTree::inserirContexto(const string &contexto,
                                   unsigned char simbolo) {
  PatriciaNode *atual = raiz;
  string restante = contexto; // contexto ainda não "consumido"

  while (!restante.empty()) {
    unsigned char c = static_cast<unsigned char>(restante[0]);

    if (atual->filhos.count(c) == 0) { // caso não exista filho iniciado com c

      // um novo nó é criado
      PatriciaNode *novo = new PatriciaNode(restante);
      atual->filhos[c] = novo;
      atual = novo;
      restante.clear();
      numNos++; // incrementa contador

    } else { // caso exista

      PatriciaNode *prox = atual->filhos[c];
      const string &pref = prox->prefixo;

      size_t i = 0;
      while (i < pref.size() && i < restante.size() && pref[i] == restante[i])
        i++;

      if (i == pref.size()) {
        atual = prox;
        restante = restante.substr(i);
      } else {
        PatriciaNode *mid = new PatriciaNode(pref.substr(0, i));
        atual->filhos[c] = mid;
        numNos++; // incrementa contador (mid)

        prox->prefixo = pref.substr(i);
        mid->filhos[static_cast<unsigned char>(prox->prefixo[0])] = prox;

        if (i < restante.size()) {
          PatriciaNode *novoFilho = new PatriciaNode(restante.substr(i));
          mid->filhos[static_cast<unsigned char>(novoFilho->prefixo[0])] =
              novoFilho;
          atual = novoFilho;
          restante.clear();
          numNos++; // incrementa contador (novoFilho)
        } else {
          atual = mid;
          restante.clear();
        }
      }
    }
  }

  // atualiza frequência do símbolo
  if (atual->freq[simbolo] == 0)
    atual->distintos++;
  atual->freq[simbolo]++;
}

// Divide todas as frequências do nó por 2 (halving).
// Remove filhos que ficam vazios. Retorna true se o próprio nó ficou vazio.
bool PatriciaTree::podarNo(PatriciaNode *no) {
  vector<unsigned char> remover;
  for (auto &[key, filho] : no->filhos) {
    if (podarNo(filho))
      remover.push_back(key);
  }
  for (unsigned char key : remover) {
    delete no->filhos[key];
    no->filhos.erase(key);
    numNos--;
  }

  no->distintos = 0;
  for (int s = 0; s < ALFABETO; s++) {
    no->freq[s] /= 2;
    if (no->freq[s] > 0)
      no->distintos++;
  }

  return (no->distintos == 0 && no->filhos.empty());
}

// Aplica halving em toda a árvore e atualiza simbolosVistos:
// remove símbolos cujo count no contexto de ordem-0 (raiz) caiu para 0.
void PatriciaTree::podar(set<unsigned char> &simbolosVistos) {
  vector<unsigned char> remover;
  for (auto &[key, filho] : raiz->filhos) {
    if (podarNo(filho))
      remover.push_back(key);
  }
  for (unsigned char key : remover) {
    delete raiz->filhos[key];
    raiz->filhos.erase(key);
    numNos--;
  }

  raiz->distintos = 0;
  for (int s = 0; s < ALFABETO; s++) {
    raiz->freq[s] /= 2;
    if (raiz->freq[s] > 0)
      raiz->distintos++;
  }

  vector<unsigned char> limpar;
  for (unsigned char s : simbolosVistos) {
    if (raiz->freq[s] == 0)
      limpar.push_back(s);
  }
  for (unsigned char s : limpar)
    simbolosVistos.erase(s);
}

PatriciaNode *PatriciaTree::buscarContexto(const string &contexto) const {
  PatriciaNode *atual = raiz;
  string restante = contexto;

  while (!restante.empty()) {
    unsigned char c = static_cast<unsigned char>(restante[0]);

    if (atual->filhos.count(c) == 0) {
      return nullptr; // contexto não encontrado
    }

    PatriciaNode *prox = atual->filhos[c];
    const string &pref = prox->prefixo;

    size_t i = 0;
    while (i < pref.size() && i < restante.size() && pref[i] == restante[i])
      i++;

    if (i == pref.size()) {
      atual = prox;
      restante = restante.substr(i);
    } else {
      // prefixo não coincide completamente
      if (i < restante.size()) {
        return nullptr; // contexto não encontrado
      } else {
        atual = prox;
        restante.clear();
      }
    }
  }

  return atual;
}
