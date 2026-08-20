CC ?= cc
CPPFLAGS ?= -Isrc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow
LDFLAGS ?=
LDLIBS ?= -lm

TARGET ?= lume
TEST_TARGETS := test_lexer test_expression test_program test_control_flow test_functions test_cli test_repl test_diagnostics test_lists test_education test_analyzer test_modules test_project test_dependencies test_stdlib test_stability test_v020
SOURCES := $(wildcard src/*.c)
CORE_SOURCES := $(filter-out src/main.c,$(SOURCES))
OBJECTS := $(SOURCES:.c=.o)
CORE_OBJECTS := $(CORE_SOURCES:.c=.o)

.PHONY: all clean test sanitize
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

test_lexer: $(CORE_OBJECTS) tests/test_lexer.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_expression: $(CORE_OBJECTS) tests/test_expression.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_program: $(CORE_OBJECTS) tests/test_program.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_control_flow: $(CORE_OBJECTS) tests/test_control_flow.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_functions: $(CORE_OBJECTS) tests/test_functions.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_cli: $(CORE_OBJECTS) tests/test_cli.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_repl: $(CORE_OBJECTS) tests/test_repl.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_diagnostics: $(CORE_OBJECTS) tests/test_diagnostics.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_lists: $(CORE_OBJECTS) tests/test_lists.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_education: $(CORE_OBJECTS) tests/test_education.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_analyzer: $(CORE_OBJECTS) tests/test_analyzer.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_modules: $(CORE_OBJECTS) tests/test_modules.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_project: $(CORE_OBJECTS) tests/test_project.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_dependencies: $(CORE_OBJECTS) tests/test_dependencies.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_stdlib: $(CORE_OBJECTS) tests/test_stdlib.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_stability: $(CORE_OBJECTS) tests/test_stability.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test_v020: $(CORE_OBJECTS) tests/test_v020.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

tests/%.o: tests/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGETS)
	./test_lexer
	./test_expression
	./test_program
	./test_control_flow
	./test_functions
	./test_cli
	./test_repl
	./test_diagnostics
	./test_lists
	./test_education
	./test_analyzer
	./test_modules
	./test_project
	./test_dependencies
	./test_stdlib
	./test_stability
	./test_v020

sanitize: CFLAGS += -g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined
sanitize: LDFLAGS += -fsanitize=address,undefined
sanitize: clean $(TEST_TARGETS)
	./test_lexer
	./test_expression
	./test_program
	./test_control_flow
	./test_functions
	./test_cli
	./test_repl
	./test_diagnostics
	./test_lists
	./test_education
	./test_analyzer
	./test_modules
	./test_project
	./test_dependencies
	./test_stdlib
	./test_stability
	./test_v020

clean:
	$(RM) $(OBJECTS) tests/*.o $(TARGET) $(TARGET).exe $(TEST_TARGETS) $(addsuffix .exe,$(TEST_TARGETS))
