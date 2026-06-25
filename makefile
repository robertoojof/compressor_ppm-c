# Compilador e flags
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Isrc/ppm-c  # -I para encontrar headers

# Diretórios
SRC_DIR = src
OBJ_DIR = obj

# Encontra automaticamente todos os .cpp em subpastas
SRCS = $(shell find $(SRC_DIR) -name '*.cpp')
# Converte src/foo/bar.cpp -> obj/foo/bar.o
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# Executável final
TARGET = main

# Regra padrão (make)
all: $(TARGET)

# Link: junta os .o no executável
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compilação: cada .cpp vira .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpa tudo (make clean)
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean