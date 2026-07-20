// ============================================================================
// map：依 key 排序的唯一鍵關聯容器
// ============================================================================
// std::map<Key, Value> 通常以平衡搜尋樹實作。元素依 Compare 排序，不是依插入順序；
// find/contains/insert/lower_bound/upper_bound 為 O(log n)。erase(pos) 是 amortized O(1)，
// erase(key) 為 O(log n + count(key))，erase(first,last) 為 O(log n + N)。節點式儲存
// 使插入不讓既有 iterator/reference 失效，erase 只使被刪元素失效。
//
// 查詢 API：
// - find/contains：不插入。
// - at：不存在丟 out_of_range，不插入。
// - operator[]：不存在會插入「預設建構 Value」；計數方便，唯讀查詢可能是 bug。
// - try_emplace：只有 key 不存在才建構 value。
// - insert_or_assign：不存在插入、存在覆寫。
// lower_bound(k) 回第一個 key 不小於 k，這是 unordered_map 做不到的 ordered query。

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

void basic_demo()
{
    std::map<int, std::string> status{{100, "created"}, {300, "done"}};
    status.try_emplace(200, "running");
    status.insert_or_assign(300, "completed");
    assert(status.at(300) == "completed");
    assert(!status.contains(250));

    const auto next = status.lower_bound(250);
    assert(next != status.end() && next->first == 300);
}

// ----------------------------------------------------------------------------
// LeetCode 981：Time Based Key-Value Store（核心查詢）
// ----------------------------------------------------------------------------
// 每個 key 的 timestamp 有序；upper_bound(t) 找第一個 >t，再退一步得最後有效版本。
// set O(log versions)，get O(log versions)。
class TimeMap {
public:
    void set(std::string key, std::string value, int timestamp)
    {
        history_[std::move(key)].insert_or_assign(timestamp, std::move(value));
    }

    std::string get(const std::string& key, int timestamp) const
    {
        const auto key_it = history_.find(key);
        if (key_it == history_.end()) {
            return {};
        }
        const auto after = key_it->second.upper_bound(timestamp);
        if (after == key_it->second.begin()) {
            return {};
        }
        return std::prev(after)->second;
    }

private:
    std::map<std::string, std::map<int, std::string>> history_;
};

void leetcode_demo()
{
    TimeMap store;
    store.set("mode", "safe", 10);
    store.set("mode", "fast", 30);
    assert(store.get("mode", 5).empty());
    assert(store.get("mode", 20) == "safe");
    assert(store.get("mode", 30) == "fast");
}

// ----------------------------------------------------------------------------
// 實務：依門檻選費率區間
// ----------------------------------------------------------------------------
double rate_for(const std::map<int, double>& rate_by_minimum_units, int units)
{
    const auto after = rate_by_minimum_units.upper_bound(units);
    assert(after != rate_by_minimum_units.begin());
    return std::prev(after)->second;
}

void practical_demo()
{
    const std::map<int, double> rates{{0, 0.10}, {100, 0.08}, {1'000, 0.05}};
    assert(rate_for(rates, 50) == 0.10);
    assert(rate_for(rates, 500) == 0.08);
    assert(rate_for(rates, 2'000) == 0.05);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "map：ordered query、版本資料與費率查詢測試通過\n";
}

// 【陷阱】const map 沒有 operator[]，因為 [] 可能修改容器；唯讀請用 find/at。
// 【陷阱】自訂 Compare 必須形成 strict weak ordering，不能寫 <=。
// 【面試】map 與 unordered_map 的選擇不能只答 O(log n) vs O(1)：還要排序、最差情況、
//         記憶體、iterator 穩定性與 deterministic output。
// 【練習】加入 erase 某 timestamp，驗證其他 iterator 不失效。

/*
 * 【教科書補充：ordered lookup 的 begin 邊界】
 * - `prev(upper_bound(x))` 只在 map 非空且 upper_bound(x)!=begin() 時合法；assert 不是 production guard。
 * - 找不到下界可回 optional/expected 或丟 domain exception，不可解參考 begin 前位置。
 * - `outer[key]` 會先建立空 inner container；後續配置失敗時可能留下空項目，僅有 basic guarantee。
 * - node-based map 插入不使既有 iterator/reference 失效，但 erase 仍使被刪節點 handle 失效。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'map.cpp' -o '/tmp/codex_cpp_C_Container_map' && '/tmp/codex_cpp_C_Container_map'
//
// === 預期輸出（節錄）===
// map：ordered query、版本資料與費率查詢測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
