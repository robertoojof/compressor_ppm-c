#pragma once
#include "ICommand.hpp"
#include "../fileIO/ReadData.hpp"
#include <chrono>
#include <iostream>

using namespace std;

class EncodeCommand : public ICommand {
public:
  void execute(const vector<string> &args) override {
    string filename = args[0];
    cout << "Comprimindo arquivo: " << filename << "..." << endl;

    string message;
    ReadData data;
    data.readFile(message, filename.c_str());

    // ... (Insira aqui toda a sua lógica original de compressão, timers, e
    // prints)
  }

  size_t getExpectedArgCount() const override {
    return 1; // Espera 1 argumento: <arquivo a ser comprimido>
  }

  string getHelp() const override {
    return "-encode <arquivo> : Comprime o arquivo especificado.";
  }
};