/*
 * std::sort：通用原地不穩定排序
 * ==============================
 * C++11 起保證 O(N log N) 比較複雜度；需要 random-access iterator。等價元素相對
 * 順序不保證。預設 operator<，自訂 comparator 必須是 strict weak ordering。
 *
 * 演算法不改 size，但會大量 move/swap；位置 iterator 仍可能有效，卻不再代表原物件。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 912：Sort an Array；標準 introspective sort 作可靠基準解。
std::vector<int> leetcode_sort_array(std::vector<int> nums) {
    std::sort(nums.begin(), nums.end());
    return nums;
}

struct Employee {
    int id;
    std::string team;
    int score;
};

// 實務：多鍵排序；score 降冪、team 升冪、id 升冪，讓結果 deterministic。
void practical_rank_employees(std::vector<Employee>& employees) {
    std::sort(employees.begin(), employees.end(),
              [](const Employee& lhs, const Employee& rhs) {
                  if (lhs.score != rhs.score) {
                      return lhs.score > rhs.score;
                  }
                  if (lhs.team != rhs.team) {
                      return lhs.team < rhs.team;
                  }
                  return lhs.id < rhs.id;
              });
}

int main() {
    assert((leetcode_sort_array({5, 2, 3, 1}) == std::vector<int>{1, 2, 3, 5}));
    assert((leetcode_sort_array({5, 1, 1, 2, 0, 0}) ==
            std::vector<int>{0, 0, 1, 1, 2, 5}));

    std::vector<Employee> employees{{3, "B", 90}, {2, "A", 90},
                                     {1, "A", 90}, {4, "A", 80}};
    practical_rank_employees(employees);
    assert(employees[0].id == 1 && employees[1].id == 2);
    assert(employees[2].id == 3 && employees[3].id == 4);
    assert(std::is_sorted(employees.begin(), employees.end(),
                          [](const Employee& lhs, const Employee& rhs) {
                              if (lhs.score != rhs.score) return lhs.score > rhs.score;
                              if (lhs.team != rhs.team) return lhs.team < rhs.team;
                              return lhs.id < rhs.id;
                          }));

    std::cout << "sort：LC912 與 deterministic 多鍵排名測試通過\n";
}

/*
 * 易錯陷阱：
 * - comparator 不可 `return lhs.score >= rhs.score`；comp(x,x) 必須 false。
 * - 不穩定表示只比較 score 時，同分者順序不保證；加 tie-breaker 或 stable_sort。
 * - pointer/reference 指向的物件內容可能因 swap 改變，不要以 index 當永恆 identity。
 * - 浮點 NaN 破壞一般排序期待；先清理或定義 total-order comparator。
 * - 大型物件搬動昂貴時可排序 index/pointer，但多一層 indirect access。
 *
 * 面試：std::sort 常用 introsort（quick sort + heap fallback + insertion 小區段），但
 * 應以標準保證 O(N log N) 為答題核心，不把某 library 實作當語言契約。
 *
 * 練習：用 std::tie 寫相同多鍵 comparator；注意 descending 欄位不能直接全部 tie。
 * 為 comparator 寫 property test：irreflexive、asymmetric、transitive。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'sort.cpp' -o '/tmp/codex_cpp_C_Algorithm_sorting_sort' && '/tmp/codex_cpp_C_Algorithm_sorting_sort'
//
// === 預期輸出（節錄）===
// sort：LC912 與 deterministic 多鍵排名測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
