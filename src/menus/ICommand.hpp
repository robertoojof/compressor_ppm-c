#pragma once
#include <string>
#include <vector>

class ICommand {
public:
  virtual ~ICommand() = default;

  // Executa a lógica principal do comando
  virtual void execute(const std::vector<std::string> &args) = 0;

  // Retorna a quantidade de argumentos que este comando espera (excluindo o
  // nome do comando)
  virtual size_t getExpectedArgCount() const = 0;

  // Retorna uma breve descrição para o menu de ajuda
  virtual std::string getHelp() const = 0;
};