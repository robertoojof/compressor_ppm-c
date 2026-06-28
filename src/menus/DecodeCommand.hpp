#pragma once
#include "ICommand.hpp"
#include "../compressor/compressor.hpp"
#include <chrono>
#include <iostream>

using namespace std;

class DecodeCommand : public ICommand {
public:
  void execute(const vector<string> &args) override {
    string compressed = args[0];
    string output_dir = args.size() > 1 ? args[1] : ".";
    cout << "Descomprimindo: " << compressed << "..." << endl;

    auto start = chrono::high_resolution_clock::now();
    Compressor compressor;
    compressor.decode_multi(compressed, output_dir);
    auto end = chrono::high_resolution_clock::now();

    double elapsed = chrono::duration<double>(end - start).count();
    cout << "Concluido em " << elapsed << "s" << endl;
  }

  size_t getExpectedArgCount() const override {
    return 1;
  }

  string getHelp() const override {
    return "-decode <arquivo_comprimido> [diretorio_saida]\n"
           "  Descomprime um .bin gerado pelo -encode e extrai os arquivos originais.\n"
           "  <arquivo_comprimido>  caminho para o arquivo .bin\n"
           "  [diretorio_saida]     onde extrair (padrao: diretorio atual)";
  }
};
