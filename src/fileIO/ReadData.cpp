#include "ReadData.hpp"
#include "../config/config.hpp"

using namespace std;

void ReadData::readFile(string &message, const string &filename) {
  // ler arquivo como binário
  ifstream file(filename, ios::binary);
  if (file.is_open()) {
    // obter tamanho do arquivo, movemos o ponteiro para o final do arquivo
    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

    // ler bytes do arquivo
    message.resize(fileSize);
    file.read(&message[0], fileSize);
    file.close();
    cout << "Arquivo '" << filename << "' lido com sucesso (" << fileSize
         << " bytes)" << endl;
  } else {
    cerr << "Erro: Não foi possível abrir o arquivo '" << filename << "'"
         << endl;
    throw runtime_error("Erro: Falha na leitura dos bytes do arquivo '" +
                        filename + "'");
  }
}
