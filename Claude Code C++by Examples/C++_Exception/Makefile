# Makefile — 一鍵編譯所有 exception 範例

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

SRCS := $(wildcard [0-9][0-9]_*.cpp)
BINS := $(SRCS:.cpp=)

.PHONY: all run clean

all: $(BINS)

%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

run: all
	@for b in $(BINS); do \
		echo "================ $$b ================"; \
		./$$b; \
		echo; \
	done

clean:
	rm -f $(BINS)
