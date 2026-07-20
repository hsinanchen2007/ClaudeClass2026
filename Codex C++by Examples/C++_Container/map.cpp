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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 981. Time Based Key-Value Store（基於時間的鍵值儲存）
// 題目：set(key,value,t) 保存版本，get(key,t) 取時間不超過 t 的最新值；如 t=10 設 safe，t=20 查得 safe。
// 為何使用本章主題：內層 map 依 timestamp 排序，upper_bound 可直接找到查詢時間之後的第一個版本。
// 思路：外層以 key 找版本表；對 timestamp 做 upper_bound；若不是 begin 就退一步取得最後有效版本。
// 複雜度：set/get 皆為 O(log K + log V)，額外空間 O(M)，K 為 key 數、V 為該 key 版本數、M 為總版本數。
// 易錯點：upper_bound 等於 begin 代表沒有有效版本；缺 key 回空字串；同 timestamp 由 insert_or_assign 覆寫。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】依用量門檻選擇階梯費率
// 情境：費率表以最低適用用量為 key，查詢任意用量時要取得不超過該用量的最高門檻費率。
// 為何使用本章主題：map 維持門檻排序並支援 upper_bound，比每次線性掃描或手動排序更直接。
// 設計：找第一個大於 units 的門檻；退回前一項；回傳該區間的費率。
// 成本：每次查詢時間 O(log R)、額外空間 O(1)，R 為費率區間數。
// 上線注意：表必須非空且含涵蓋最小輸入的門檻；assert 不能取代正式錯誤處理，浮點費率也應定義精度政策。
// -----------------------------------------------------------------------------
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
