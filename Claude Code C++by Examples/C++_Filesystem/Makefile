# Makefile — 一鍵編譯所有 filesystem 範例
#
# 注意：GCC 8 之前可能需要 -lstdc++fs；GCC 9+ 已不需要。

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
LDLIBS   :=

SRCS := $(wildcard [0-9][0-9]_*.cpp)
BINS := $(SRCS:.cpp=)

.PHONY: all run clean

all: $(BINS)

%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDLIBS)

run: all
	@for b in $(BINS); do \
		echo "================ $$b ================"; \
		./$$b; \
		echo; \
	done

clean:
	rm -rf $(BINS) tmp_fs_dir tmp_fs_*.txt
