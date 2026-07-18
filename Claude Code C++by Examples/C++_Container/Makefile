# Makefile — 一次編譯所有 STL container demo
# ----------------------------------------------------------------------------
#  make             編譯全部 .cpp
#  make run         編譯並執行全部 demo
#  make clean       清除執行檔
#  make <name>      只編譯某一支 (e.g. make vector)
# ----------------------------------------------------------------------------

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -O2

# 所有 demo 對應的執行檔名稱 (= 來源檔的 basename)
TARGETS = \
	array vector deque list forward_list \
	set multiset map multimap \
	unordered_set unordered_multiset unordered_map unordered_multimap \
	stack queue priority_queue \
	span

.PHONY: all run clean
all: $(TARGETS)

# 一個通用規則:foo  ←  foo.cpp
%: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

run: all
	@for t in $(TARGETS); do \
		echo "════════════════════ ./$$t ════════════════════"; \
		./$$t || echo "  ✗ $$t exit non-zero"; \
		echo; \
	done

clean:
	@rm -f $(TARGETS)
	@echo "cleaned: $(TARGETS)"
