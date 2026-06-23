#pragma once
#include "ICommand.hpp"
#include <iostream>
#include <map>
#include <memory>
#include <string>

class CLIApplication {
private:
  std::map<std::string, std::unique_ptr<ICommand>> commands;
  std::string programName;

  void printHelp() const {
    std::cerr << "Uso incorreto ou comando invalido.\n\n"
              << "Comandos disponiveis:\n";
    for (const auto &pair : commands) {
      std::cerr << "  " << pair.second->getHelp() << "\n";
    }
  }

public:
  CLIApplication(const std::string &progName) : programName(progName) {}

  // Permite "injetar" comandos na aplicação (Inversão de Dependência)
  void registerCommand(const std::string &commandName,
                       std::unique_ptr<ICommand> command) {
    commands[commandName] = std::move(command);
  }

  void run(int argc, char *argv[]) {
    if (argc < 2) {
      printHelp();
      return;
    }

    std::string commandName = argv[1];
    auto it = commands.find(commandName);

    // Se o comando existe no mapa
    if (it != commands.end()) {
      ICommand *cmd = it->second.get();

      // Extrai os argumentos subsequentes
      std::vector<std::string> args;
      for (int i = 2; i < argc; ++i) {
        args.push_back(argv[i]);
      }

      // Valida a quantidade de argumentos baseada no contrato da interface
      if (args.size() != cmd->getExpectedArgCount()) {
        std::cerr << "Erro: Numero incorreto de argumentos para o comando '"
                  << commandName << "'.\n"
                  << "Uso esperado: " << cmd->getHelp() << "\n";
        return;
      }

      // Executa polimorficamente
      cmd->execute(args);
    } else {
      std::cerr << "Comando desconhecido: " << commandName << "\n";
      printHelp();
    }
  }
};