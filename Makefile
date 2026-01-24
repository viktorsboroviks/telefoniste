.PHONY: \
	all \
	format \
	format-cpp \
	lint \
	lint-cpp \
	clean

all: lint format

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

clean:
	rm -f cppcheck_report.txt
