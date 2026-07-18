# Makefile — 一鍵編譯所有 iterator 範例
# 使用：
#   make            編譯所有
#   make run        編譯並依序執行
#   make 01         單獨編譯 01_input_iterator
#   make clean      清掉執行檔

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
