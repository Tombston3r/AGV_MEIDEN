# Build natif des tests (brief §11). La cible embarquée reste PlatformIO :
#   pio run -e agv        firmware/agv       (MQTT/LTE-M)
#   pio run -e agv-sms    firmware/agv       (SMS, comparaison seulement)
#   pio run -e poste      firmware/poste-esp32
#
# Ici on ne compile que ce qui tourne sur PC : tout /common, /sim et les tests.
# Aucun fichier de /common ne doit dépendre d'un en-tête ESP-IDF ou Arduino en
# dehors d'un bloc #ifdef — cette cible en est le garde-fou.

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
