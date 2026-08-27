# Tests natifs du moteur et du codec, banc API local — aucun matériel requis.
CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -O1 -g -MMD -MP
BUILD = build

MOTEUR = moteur/planning.cpp moteur/serialisation.cpp atelier/agvdump.cpp
OBJ_MOTEUR = $(MOTEUR:%.cpp=$(BUILD)/%.o)

BIN_PLANNING = $(BUILD)/tests_planning
BIN_SERIAL = $(BUILD)/tests_serialisation
BIN_BANC = $(BUILD)/serveur_banc_api
BIN_DUMP = $(BUILD)/tests_agvdump

test: $(BIN_PLANNING) $(BIN_SERIAL) $(BIN_DUMP) $(BIN_BANC)
	./$(BIN_PLANNING)
	./$(BIN_SERIAL)
	./$(BIN_DUMP)
	BANC_API_BIN=$(BIN_BANC) python3 banc_api/tests/test_api.py

banc: $(BIN_BANC)
	./$(BIN_BANC) --port 8081

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I. -Iatelier -c $< -o $@

$(BIN_PLANNING): $(OBJ_MOTEUR) $(BUILD)/test/test_planning.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_SERIAL): $(OBJ_MOTEUR) $(BUILD)/test/test_serialisation.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DUMP): $(OBJ_MOTEUR) $(BUILD)/test/test_agvdump.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_BANC): $(OBJ_MOTEUR) $(BUILD)/banc_api/serveur.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Sans le suivi des en-têtes, modifier planning.h ne recompile rien et l'on
# relie des objets incohérents — déjà vécu sur le dépôt principal.
-include $(OBJ_MOTEUR:.o=.d) $(BUILD)/test/test_planning.d $(BUILD)/test/test_serialisation.d $(BUILD)/test/test_agvdump.d $(BUILD)/banc_api/serveur.d

clean:
	rm -rf $(BUILD)

.PHONY: test banc clean
