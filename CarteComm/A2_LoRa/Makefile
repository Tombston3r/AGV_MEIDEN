# Build natif des tests (brief §11). Les cibles embarquées passent par
# PlatformIO :
#   pio run -e esp32      ESP32 de la carte V5.0.1 (Wi-Fi + MQTT)
#   pio run -e mega       ATmega2560 de la carte (séquenceur + file)
#
# Le cœur métier compile à l'identique en natif, pour ESP-IDF et pour l'AVR :
# c'est ce qui permet de tester le séquenceur contre le simulateur avant de le
# flasher sur un microcontrôleur difficile à déboguer.
#
# Ici on ne compile que ce qui tourne sur PC : /common, /sim et les tests.
# Aucun fichier de /common ne doit dépendre d'un en-tête ESP-IDF, Arduino ou AVR
# en dehors d'un bloc #ifdef — cette cible en est le garde-fou.

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O1 -g
INCLUDES  = -Ifirmware/common -Isim -Itest/native

PROFILE  ?= profiles/default.yaml
GENERATED = firmware/common/config/generated_profile.h

COMMON_SRC = $(wildcard firmware/common/*/*.cpp)
SIM_SRC    = $(wildcard sim/*.cpp)
TEST_SRC   = $(wildcard test/native/*.cpp)

BUILD_DIR = build/native
OBJ = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(COMMON_SRC) $(SIM_SRC) $(TEST_SRC))
BIN = $(BUILD_DIR)/agv_tests

.PHONY: all test config clean lint

all: $(BIN)

config $(GENERATED):
	python3 tools/genconfig.py $(PROFILE) $(GENERATED)

$(BUILD_DIR)/%.o: %.cpp $(GENERATED)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

test: $(BIN)
	./$(BIN) $(FILTER)

lint:
	ruff check poste-unipi tools || true
	mypy --strict poste-unipi || true

clean:
	rm -rf build
