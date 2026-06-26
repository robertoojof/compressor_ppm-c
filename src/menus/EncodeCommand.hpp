#pragma once
#include "../compressor/compressor.hpp"
#include "../fileIO/ReadData.hpp"
#include "ICommand.hpp"
#include <chrono>
#include <iostream>

using namespace std;

class EncodeCommand : public ICommand {
public:
  void execute(const vector<string> &args) override {
    ReadData data;
    vector<pair<string, string>> files;
    int KMAX{0};

    vector<string> fileArgs;
    for (size_t i = 0; i < args.size(); ++i) {
      if (args[i] == "-k" || args[i] == "-K") {
        if (i + 1 >= args.size())
          throw runtime_error("Faltou o valor de K apos -k");
        KMAX = stoi(args[++i]);
      } else {
        fileArgs.push_back(args[i]);
      }
    }

    if (fileArgs.empty())
      throw runtime_error("Nenhum arquivo especificado");

    for (const auto &filename : fileArgs) {
      string content;
      data.readFile(content, filename);
      size_t pos = filename.find_last_of("/\\");
      string name = pos == string::npos ? filename : filename.substr(pos + 1);
      files.push_back({name, content});
    }

    auto start = chrono::high_resolution_clock::now();
    Compressor compressor;
    compressor.encoder_multi(files, KMAX);
    auto end = chrono::high_resolution_clock::now();

    double elapsed = chrono::duration<double>(end - start).count();
    cout << "Concluido em " << elapsed << "s -> output.bin" << endl;
  }

  size_t getExpectedArgCount() const override { return 1; }

  string getHelp() const override {
    return "-encode [-k <valor>] <arquivo1> [arquivo2 ...] : Comprime os "
           "arquivos especificados em output.bin.";
  }
};