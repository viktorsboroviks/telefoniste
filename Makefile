.PHONY: \
	all \
	format \
	format-cpp \
	lint \
	lint-cpp \
	clean \
	test \
	run-test

all: lint format test

UNAME_S := $(shell uname -s)
# on macos use clang++

ifeq ($(UNAME_S),Darwin)
CPP := clang++
# on linux use g++
else
CPP := g++
endif

#SANITIZE := yes
ifeq ($(SANITIZE),yes)
SANITIZE_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer
ifeq ($(UNAME_S),Darwin)
# add nothing
else
SANITIZE_FLAGS += -fsanitize=leak
endif
endif

#ASSERTS := yes
ifeq ($(ASSERTS),yes)
# do nothing
else
ASSERTS_FLAGS := -DNDEBUG
endif

#DEBUG := yes
ifeq ($(DEBUG),yes)
DEBUG_FLAGS := -g
OPTIMIZE_FLAGS := -O0
else
OPTIMIZE_FLAGS := -O3
endif

format: format-cpp

format-cpp: \
		include/telefoniste.hpp
	clang-format -i $^

lint: lint-cpp

lint-cpp: \
		include/telefoniste.hpp
	cppcheck \
		--enable=warning,portability,performance \
		--enable=style,information \
		--enable=missingInclude \
		--inconclusive \
		--library=std,posix,gnu \
		--platform=unix64 \
		--language=c++ \
		--std=c++20 \
		--inline-suppr \
		--check-level=exhaustive \
		--suppress=missingIncludeSystem \
		--suppress=checkersReport \
		--checkers-report=cppcheck_report.txt \
		-I./include \
		$^

tests/echo_test.out: \
		tests/echo_test.cpp \
		include/telefoniste.hpp
	$(CPP) -std=c++20 \
		-Wall -Wextra -Werror -Wpedantic \
		-pthread \
		$(SANITIZE_FLAGS) \
		$(OPTIMIZE_FLAGS) \
		$(ASSERTS_FLAGS) \
		$(DEBUG_FLAGS) \
		-I./include \
		./tests/echo_test.cpp -o ./tests/echo_test.out

test: tests/echo_test.out

tests/answer_calls_test.out: \
		tests/answer_calls_test.cpp \
		include/telefoniste.hpp
	$(CPP) -std=c++20 \
		-Wall -Wextra -Werror -Wpedantic \
		-pthread \
		$(SANITIZE_FLAGS) \
		$(OPTIMIZE_FLAGS) \
		$(ASSERTS_FLAGS) \
		$(DEBUG_FLAGS) \
		-I./include \
		./tests/answer_calls_test.cpp -o ./tests/answer_calls_test.out

test: \
		tests/echo_test.out \
		tests/answer_calls_test.out
	./tests/echo_test.out
	./tests/answer_calls_test.out

clean:
	rm -f cppcheck_report.txt
	rm -f tests/*.out
