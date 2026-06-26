#include "ReadData.hpp"
#include "../config/config.hpp"
#include <filesystem>

using namespace std;
namespace fs = filesystem;

void ReadData::readFile(string &message, const string &filename) {
  ifstream file(filename, ios::binary);
  if (file.is_open()) {
    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

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

ofstream ReadData::writeFile(const string &path) {
  fs::path p(path);
  if (p.has_parent_path())
    fs::create_directories(p.parent_path());

  ofstream ofs(path, ios::binary);
  if (!ofs.is_open())
    throw runtime_error("Erro: Não foi possível criar o arquivo '" + path + "'");

  return ofs;
}
