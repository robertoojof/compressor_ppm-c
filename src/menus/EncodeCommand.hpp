#pragma once
#include "ICommand.hpp"
#include "ReadData.hpp"
#include <iostream>
#include <chrono>

class EncodeCommand : public ICommand {
public:
    void execute(const std::vector<std::string>& args) override {
        std::string filename = args[0];
        std::cout << "Comprimindo arquivo: " << filename << "..." << std::endl;
        
        std::string message;
        ReadData data;
        data.readtxt(message, filename.c_str());
    
        // ... (Insira aqui toda a sua lógica original de compressão, timers, e prints)
    }

    size_t getExpectedArgCount() const override {
        return 1; // Espera 1 argumento: <arquivo a ser comprimido>
    }

    std::string getHelp() const override {
        return "-encode <arquivo> : Comprime o arquivo especificado.";
    }
};