.PHONY: \
	all \
	format \
	format-cpp \
	format-python \
	lint \
	lint-cpp \
	venv \
	lint-python \
	clean \
	test

all: lint format test

PYTHON_VERSION := python3.12
PYTHON_FILES := \
	python/telefoniste.py

venv: requirements.txt
	$(PYTHON_VERSION) -m venv venv
	./venv/bin/pip3 install --no-cache-dir --requirement requirements.txt

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

format: format-cpp format-python

format-cpp: \
		include/telefoniste.hpp
	clang-format -i $^

format-python: venv $(PYTHON_FILES)
	./venv/bin/black $(PYTHON_FILES)

lint: lint-cpp lint-python

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

lint-python: venv $(PYTHON_FILES)
	PYTHONPATH=$(PYTHONPATH):./python \
		./venv/bin/pylint $(PYTHON_FILES) ; \
		./venv/bin/flake8 $(PYTHON_FILES)

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

tests/answer_calls_single_thread_test.out: \
		tests/answer_calls_single_thread_test.cpp \
		include/telefoniste.hpp
	$(CPP) -std=c++20 \
		-Wall -Wextra -Werror -Wpedantic \
		-pthread \
		$(SANITIZE_FLAGS) \
		$(OPTIMIZE_FLAGS) \
		$(ASSERTS_FLAGS) \
		$(DEBUG_FLAGS) \
		-I./include \
		./tests/answer_calls_single_thread_test.cpp -o ./tests/answer_calls_single_thread_test.out

tests/answer_calls_multiclient_test.out: \
		tests/answer_calls_multiclient_test.cpp \
		include/telefoniste.hpp
	$(CPP) -std=c++20 \
		-Wall -Wextra -Werror -Wpedantic \
		-pthread \
		$(SANITIZE_FLAGS) \
		$(OPTIMIZE_FLAGS) \
		$(ASSERTS_FLAGS) \
		$(DEBUG_FLAGS) \
		-I./include \
		./tests/answer_calls_multiclient_test.cpp -o ./tests/answer_calls_multiclient_test.out

tests/answer_calls_zero_length_test.out: \
		tests/answer_calls_zero_length_test.cpp \
		include/telefoniste.hpp
	$(CPP) -std=c++20 \
		-Wall -Wextra -Werror -Wpedantic \
		-pthread \
		$(SANITIZE_FLAGS) \
		$(OPTIMIZE_FLAGS) \
		$(ASSERTS_FLAGS) \
		$(DEBUG_FLAGS) \
		-I./include \
		./tests/answer_calls_zero_length_test.cpp -o ./tests/answer_calls_zero_length_test.out

tests/answer_calls_large_payload_test.out: \
		tests/answer_calls_large_payload_test.cpp \
		include/telefoniste.hpp
	$(CPP) -std=c++20 \
		-Wall -Wextra -Werror -Wpedantic \
		-pthread \
		$(SANITIZE_FLAGS) \
		$(OPTIMIZE_FLAGS) \
		$(ASSERTS_FLAGS) \
		$(DEBUG_FLAGS) \
		-I./include \
		./tests/answer_calls_large_payload_test.cpp -o ./tests/answer_calls_large_payload_test.out

tests/py_server_test.out: \
		tests/py_server_test.cpp \
		include/telefoniste.hpp
	$(CPP) -std=c++20 \
		-Wall -Wextra -Werror -Wpedantic \
		-pthread \
		$(SANITIZE_FLAGS) \
		$(OPTIMIZE_FLAGS) \
		$(ASSERTS_FLAGS) \
		$(DEBUG_FLAGS) \
		-I./include \
		./tests/py_server_test.cpp -o ./tests/py_server_test.out

test: venv \
		tests/echo_test.out \
		tests/answer_calls_test.out \
		tests/answer_calls_single_thread_test.out \
		tests/answer_calls_multiclient_test.out \
		tests/answer_calls_zero_length_test.out \
		tests/answer_calls_large_payload_test.out \
		tests/py_server_test.out
	./tests/echo_test.out
	./tests/answer_calls_test.out
	./tests/answer_calls_single_thread_test.out
	./tests/answer_calls_multiclient_test.out
	./tests/answer_calls_zero_length_test.out
	./tests/answer_calls_large_payload_test.out
	PYTHONPATH=$(PYTHONPATH):./python \
		./venv/bin/python3 ./tests/py_client_test.py

clean:
	rm -f cppcheck_report.txt
	rm -f tests/*.out
	rm -rf venv
	rm -rf python/__pycache__
