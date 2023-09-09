TARGET = a.out

CXX = g++ -std=c++2a
CPPFLAGS = -W -Wall -Wextra -Wunused -Wcast-align -Werror -pedantic -pedantic-errors \
	-Wfloat-equal -Wpointer-arith -Wwrite-strings -Wcast-align \
	-Wno-format -Wno-long-long -Wmissing-declarations -Warray-bounds -Wdiv-by-zero

PREF_OBJ = obj/

SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp, $(PREF_OBJ)%.o, $(SRC))

.PHONY: all
all: $(TARGET)
	@./$(TARGET)

$(TARGET): $(OBJ)
	@echo Linking...
	@$(CXX) $^ -o $@
	@echo Done!
	@echo

DEPS = $(OBJ:.o=.d)

$(PREF_OBJ)%.o: %.cpp 
	@echo Compiling $(patsubst $(PREF_OBJ)%.o, %, $@)...
	@$(CXX) -MMD -MP $(CPPFLAGS) -c $< -o $@
	@echo Done!
	@echo

-include $(DEPS)

.PHONY: clean
clean:
	@echo Cleaning...
	@rm -f $(PREF_OBJ)*.o $(PREF_OBJ)*.d $(TARGET)
	@echo Done!
	@echo
