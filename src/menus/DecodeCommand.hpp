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
    string output = args[1];
    cout << "Descomprimindo: " << compressed << "..." << endl;

    auto start = chrono::high_resolution_clock::now();
    Compressor compressor;
    compressor.decode_pure(compressed, output);
    auto end = chrono::high_resolution_clock::now();

    double elapsed = chrono::duration<double>(end - start).count();
    cout << "Concluido em " << elapsed << "s -> " << output << endl;
  }

  size_t getExpectedArgCount() const override {
    return 2;
  }

  string getHelp() const override {
    return "-decode <arquivo_comprimido> <saida> : Descomprime o arquivo.";
  }
};
