#pragma once
#include "../compressor/compressor.hpp"
#include "../fileIO/ReadData.hpp"
#include "ICommand.hpp"
#include <chrono>
#include <iomanip>
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
    int p_threshold = 0;   // ativa poda (halving)
    int r_threshold = 0;   // ativa reset completo
    string log_filename;   // vazio = sem log; preenchido por -log

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
        // -p <percentual>: ativa PODA por janela
        if (i + 1 >= args.size())
          throw runtime_error("Faltou o valor de <percentual> apos -p");
        p_threshold = stoi(args[++i]);
      } else if (args[i] == "-r" || args[i] == "-R") {
        // -r <percentual>: ativa RESET por janela
        if (i + 1 >= args.size())
          throw runtime_error("Faltou o valor de <percentual> apos -r");
        r_threshold = stoi(args[++i]);
      } else if (args[i] == "-log") {
        // -log [arquivo]: log progressivo de bits/símbolo
        // se não vier nome, usa "compression_log.log"
        if (i + 1 < args.size() && args[i + 1][0] != '-')
          log_filename = args[++i];
        else
          log_filename = "compression_log.log";
      } else {
        fileArgs.push_back(args[i]);
      }
    }

    // Poda e reset são mutuamente exclusivos
    if (p_threshold > 0 && r_threshold > 0)
      throw runtime_error("Nao e possivel usar -p (poda) e -r (reset) ao mesmo tempo");

    if (fileArgs.empty())
      throw runtime_error("Nenhum arquivo especificado");

    // mode: 0=nenhum  1=poda  2=reset
    int threshold = (p_threshold > 0) ? p_threshold : r_threshold;
    int mode      = (p_threshold > 0) ? 1 : (r_threshold > 0) ? 2 : 0;

    for (const auto &filename : fileArgs) {
      string content;
      data.readFile(content, filename);
      size_t pos = filename.find_last_of("/\\");
      string name = pos == string::npos ? filename : filename.substr(pos + 1);
      files.push_back({name, content});
    }

    size_t total_bytes = 0;
    for (const auto &f : files) total_bytes += f.second.size();

    auto start = chrono::high_resolution_clock::now();
    Compressor compressor;
    double bps = compressor.encoder_multi(files, KMAX, output_filename, j_window, threshold, mode, log_filename);
    auto end = chrono::high_resolution_clock::now();

    double elapsed = chrono::duration<double>(end - start).count();
    cout << "Concluido em " << elapsed << "s -> " << output_filename << ".bin\n"
         << "Bits/simbolo: " << fixed << setprecision(4) << bps
         << "  |  Razao: 1:" << setprecision(2) << (8.0 / bps)
         << "  |  " << total_bytes << " bytes -> "
         << static_cast<size_t>(total_bytes * bps / 8) << " bytes (aprox)" << endl;
  }

  size_t getExpectedArgCount() const override { return 1; }

  string getHelp() const override {
    return "-encode [-k <kmax>] [-o <saida>] [-j <janela>] [-p <perc> | -r <perc>] [-log [arquivo]] <arq1> ...\n"
           "  -p: poda (halving) quando a janela degrada mais de <perc>%\n"
           "  -r: reset completo nas mesmas condições  (nao use -p e -r juntos)\n"
           "  -log [arquivo]: grava CSV posicao,bits_por_simbolo,evento (padrao: compression_log.log)";
  }
};
