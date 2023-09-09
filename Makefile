TARGET = runme

CXX = g++ -std=c++2a
CPPFLAGS = -W -Wall -Wextra -Wunused -Wcast-align -Werror -pedantic -pedantic-errors \
	-Wfloat-equal -Wpointer-arith -Wwrite-strings -Wcast-align \
	-Wno-format -Wno-long-long -Wmissing-declarations -Warray-bounds -Wdiv-by-zero

TESTING = ./TESTING

SRC_PATHS = $(shell find . -name "*.cpp" -not -path "$(TESTING)*")
SRC_NAMES = $(shell basename -a $(SRC_PATHS))

PREF_OBJ = ObjectFiles/
OBJ = $(patsubst %.cpp, $(PREF_OBJ)%.o, $(SRC_NAMES))

.PHONY: all
all: $(TARGET)
	@echo Running server...
	@echo
	@./$(TARGET)

$(TARGET): $(OBJ)
	@echo Linking project...
	@$(CXX) $^ -o $@
	@echo Done!
	@echo

DEPS = $(OBJ:.o=.d)

.SECONDEXPANSION:
$(PREF_OBJ)%.o: $$(shell echo $(SRC_PATHS) | tr ' ' '\n' | grep %)
	@echo Compiling $(patsubst $(PREF_OBJ)%.o, %, $@)...
	@$(CXX) -MMD -MP $(CPPFLAGS) -c $< -o $@
	@echo Done!
	@echo

-include $(DEPS)

$(PREF_OBJ)%.d:

.PHONY: tests
tests:
	@chmod +x do-tests.sh
	@./do-tests.sh

.PHONY: stress
stress:
	@cd $(shell dirname `find $(TESTING) -name "Makefile"`) && $(MAKE)
	@echo
	@echo Running...
	@echo
	@echo -n Time for inserts:
	@time python3 client.py <$(shell find $(TESTING) -name "insert.txt") >/dev/null
	@echo
	@echo -n Time for mix:
	@time python3 client.py <$(shell find $(TESTING) -name "mix.txt") >/dev/null

.PHONY: clean
clean:
	@echo Cleaning...
	@rm -f $(PREF_OBJ)*.o $(PREF_OBJ)*.d $(TARGET)
	@echo Done!

