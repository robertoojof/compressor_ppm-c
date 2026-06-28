#pragma once
#include "ICommand.hpp"
#include <iostream>
#include <map>
#include <memory>
#include <string>

using namespace std;

class CLIApplication {
private:
  map<string, unique_ptr<ICommand>> commands;
  string programName;

  void printHelp(ostream &out, bool isError = false) const {
    if (isError)
      out << "Uso incorreto ou comando invalido.\n\n";

    out << "PPM-C Compressor  —  codificacao aritmetica com modelo PPM\n"
        << "Uso: " << programName << " <comando> [opcoes] [arquivos]\n\n"
        << "Comandos disponíveis:\n\n";

    for (const auto &[name, cmd] : commands)
      out << cmd->getHelp() << "\n\n";
  }

public:
  CLIApplication(const string &progName) : programName(progName) {}

  // Permite "injetar" comandos na aplicação (Inversão de Dependência)
  void registerCommand(const string &commandName,
                       unique_ptr<ICommand> command) {
    commands[commandName] = move(command);
  }

  void run(int argc, char *argv[]) {
    if (argc < 2) {
      printHelp(cerr, /*isError=*/true);
      return;
    }

    string commandName = argv[1];

    if (commandName == "-h" || commandName == "--help") {
      printHelp(cout);
      return;
    }

    auto it = commands.find(commandName);

    // Se o comando existe no mapa
    if (it != commands.end()) {
      ICommand *cmd = it->second.get();

      // Extrai os argumentos subsequentes
      vector<string> args;
      for (int i = 2; i < argc; ++i) {
        args.push_back(argv[i]);
      }

      // Valida a quantidade de argumentos baseada no contrato da interface
      if (args.size() < cmd->getExpectedArgCount()) {
        cerr << "Erro: Numero incorreto de argumentos para o comando '"
             << commandName << "'.\n"
             << "Uso esperado: " << cmd->getHelp() << "\n";
        return;
      }

      // Executa polimorficamente
      cmd->execute(args);
    } else {
      cerr << "Comando desconhecido: " << commandName << "\n";
      printHelp(cerr, /*isError=*/true);
    }
  }
};
