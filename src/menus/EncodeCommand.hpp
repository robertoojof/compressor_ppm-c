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
    string output_filename = "output";

    int j_window = 0;
    int p_threshold = 0;

    vector<string> fileArgs;
    for (size_t i = 0; i < args.size(); ++i) {
      if (args[i] == "-k" || args[i] == "-K") {
        if (i + 1 >= args.size())
          throw runtime_error("Faltou o valor de K apos -k");
        KMAX = stoi(args[++i]);
      } else if (args[i] == "-o" || args[i] == "-O") {
        if (i + 1 >= args.size())
          throw runtime_error("Faltou o valor de <nome_saida> apos -o");
        output_filename = args[++i];
      } else if (args[i] == "-j" || args[i] == "-J") {
        if (i + 1 >= args.size())
          throw runtime_error("Faltou o valor de <janela> apos -j");
        j_window = stoi(args[++i]);
      } else if (args[i] == "-p" || args[i] == "-P") {
        if (i + 1 >= args.size())
          throw runtime_error("Faltou o valor de <percentual> apos -p");
        p_threshold = stoi(args[++i]);
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
    compressor.encoder_multi(files, KMAX, output_filename, j_window, p_threshold);
    auto end = chrono::high_resolution_clock::now();

    double elapsed = chrono::duration<double>(end - start).count();
    cout << "Concluido em " << elapsed << "s -> " << output_filename << ".bin" << endl;
  }

  size_t getExpectedArgCount() const override { return 1; }

  string getHelp() const override {
    return "-encode [-k <kmax>] [-o <saida>] [-j <janela>] [-p <percentual>] <arq1> [arq2 ...] : "
           "Comprime. -j e -p ativam poda por janela (ex: -j 10000 -p 10).";
  }
};
