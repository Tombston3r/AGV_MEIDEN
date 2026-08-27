# Tests natifs du moteur de planning — aucun matériel requis.
CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -O1 -g -MMD -MP
BUILD = build
SRC = moteur/planning.cpp test/test_planning.cpp
OBJ = $(SRC:%.cpp=$(BUILD)/%.o)
BIN = $(BUILD)/tests_planning

test: $(BIN)
	./$(BIN)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I. -c $< -o $@

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

# Sans le suivi des en-têtes, modifier planning.h ne recompile rien et l'on
# relie des objets incohérents — déjà vécu sur le dépôt principal.
-include $(OBJ:.o=.d)

clean:
	rm -rf $(BUILD)

.PHONY: test clean
