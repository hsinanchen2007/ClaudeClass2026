/*
 * Lambda 面試前總複習：closure、capture、callable abstraction 與 lifetime
 * ============================================================================
 * 這是可獨立閱讀的面試速讀章。若只剩十五分鐘，先讀「選擇表、生命週期、高頻陷阱、
 * 面試問答」，再執行下方整合案例確認自己能把規則用在演算法與實務 callback。
 * 【LeetCode】下方 Merge Intervals 真正執行排序與合併；【複雜度】是 O(n log n) time、
 * O(n) output space。實務 router 查找為 O(log n)，weak callback lock 為常數級共享狀態操作。
 *
 * 【一、完整語法模型】
 * [captures]<template-params>(params) specifiers -> return_type requires-clause { body }
 * - lambda expression 建立匿名 closure object；closure type 由 compiler 產生。
 * - 每一個 lambda expression 都是唯一型別，即使文字相同。
 * - `auto fn = lambda` 保存具體型別，通常最容易 inline、沒有 type erasure。
 * - captureless lambda 可轉相容 function pointer；capturing lambda 不可，因需要 state。
 * - generic lambda 的 operator() 是 function template；C++20 可明寫 `<class T>`。
 *
 * 【二、capture 選擇表】
 * ┌───────────────────────┬───────────────────────────────────────────────────┐
 * │ 寫法                  │ ownership / lifetime 意義                         │
 * ├───────────────────────┼───────────────────────────────────────────────────┤
 * │ []                    │ 無外部 state，可轉 function pointer。             │
 * │ [x]                   │ 建立時 copy x；closure 擁有自己的副本。           │
 * │ [&x]                  │ 借用 x；callback 不得活過 x，並注意 data race。    │
 * │ [=] / [&]             │ 隱式預設 capture；短 scope 尚可，長期 callback 差。│
 * │ [this]                │ copy pointer，不延長 object lifetime。            │
 * │ [*this]               │ copy object snapshot；成本/可複製性需審查。       │
 * │ [p=std::move(p)]      │ init-capture 轉移 ownership；closure 可能 move-only│
 * │ [weak=weak_ptr(obj)]  │ non-owning async callback；每次 lock，可能失敗。  │
 * └───────────────────────┴───────────────────────────────────────────────────┘
 *
 * 最重要問題不是「語法能不能編譯」，而是 closure 可能活多久：
 * - 只在同一 expression/algorithm 立即用：reference capture 通常容易證明安全。
 * - 存進 container/router/event loop/thread：視為跨 scope，優先 value/owner/weak_ptr。
 * - capture shared_ptr 會延長 lifetime，可能形成 cycle；雙向 callback 常用 weak_ptr 斷環。
 *
 * 【三、mutable 與狀態】
 * - value-captured member 在預設 `operator() const` 內不可修改。
 * - mutable 讓 closure 修改自己的副本，不會修改原變數。
 * - closure copy 各有一份 state；若 capture shared object/reference 才會共享。
 * - mutable 不等於 thread-safe；同一 closure concurrent write 是 data race。
 * - generator/backoff 可用 stateful lambda；domain state 複雜時改用命名 class。
 *
 * 【四、callable 儲存方式】
 * ┌─────────────────────────┬─────────────────────────────────────────────────┐
 * │ 方式                    │ 選擇與成本                                      │
 * ├─────────────────────────┼─────────────────────────────────────────────────┤
 * │ template/auto parameter │ 保留具體型別、可 inline；最適合短期 generic call│
 * │ function pointer        │ 小、C ABI 友善、無 state；可搭 void* context。  │
 * │ std::function           │ copyable type erasure、間接呼叫、可能 heap。    │
 * │ std::bind               │ partial application；預設 decay-copy，lambda較清│
 * │ std::invoke             │ 統一 callable/member pointer 呼叫，不負責儲存。 │
 * └─────────────────────────┴─────────────────────────────────────────────────┘
 *
 * C++20 std::function 要求 target CopyConstructible；move-captured unique_ptr lambda 不可直接放。
 * empty std::function 在 bool context 為 false，呼叫會丟 std::bad_function_call。
 * small-buffer optimization 常見但標準不保證，效能敏感 code 要量測。
 *
 * 【五、STL algorithm 規則】
 * - predicate 最好純函式；不要依賴演算法 call 次數、順序或外部可變 key。
 * - sort comparator 必須 strict weak ordering：comp(x,x)==false，因此不能用 <=。
 * - sort 通常 O(n log n) comparisons；find_if/remove_if/transform O(n)。
 * - reference-capture output 可行，但 algorithm 中途 throw 時可能留下 partial state。
 * - erase-remove：remove_if 只把保留元素搬前並回 new logical end，容器仍需 erase。
 *
 * 【六、std::bind 快速辨識】
 * using namespace std::placeholders;
 * auto by_two = std::bind(multiply, _1, 2); // 未來第一參數代入 _1
 * - bound x 預設 copy；要綁 reference 用 std::ref(x)/std::cref(x)。
 * - std::ref 不延長 lifetime。
 * - member pointer 可綁 object，但 lambda `[&obj](x){ return obj.f(x); }` 通常更易讀。
 * - nested bind 會把 bind-expression 參數再展開，維護成本高，優先 lambda。
 *
 * 【七、高頻陷阱清單】
 * 1. 回傳 `[&local]`：函式結束 local 消失，callback dangling。
 * 2. event callback `[this]`：owner 析構後仍被排程，use-after-free。
 * 3. `[=]` 以為複製 object，實際舊語意常只 capture this pointer。
 * 4. init-capture `std::move` 後仍使用原 unique_ptr，原值已 null。
 * 5. mutable closure 同時被多 threads 呼叫，未同步造成 data race。
 * 6. comparator 用 <= 或每次回傳不同結果，sort 前置條件被破壞。
 * 7. std::function 保存巨大 capture，出現未預期 allocation/cache miss。
 * 8. empty std::function 未檢查便呼叫，丟 bad_function_call。
 * 9. bind 忘記 std::ref，實際修改 closure 內副本而非原物件。
 * 10. algorithm lambda 累積外部 state，卻假設 exception 時操作具交易性。
 *
 * 【八、面試問答】
 * Q1: lambda 是 function 嗎？
 * A1: expression 產生 closure object，呼叫通常走其 operator()；captureless 才可轉 function pointer。
 * Q2: 為何每個 lambda 型別不同？
 * A2: 每個 expression 可有不同 captures/layout/call operator，標準指定 unique unnamed type。
 * Q3: value capture 何時取值？
 * A3: closure 建立當下，不是呼叫當下。
 * Q4: mutable 能修改 reference-captured object 嗎？
 * A4: reference capture 本來就可依 referent constness 修改；mutable主要影響 value members。
 * Q5: generic lambda 如何 perfect forward？
 * A5: `[](auto&& x)->decltype(auto){ return f(std::forward<decltype(x)>(x)); }`。
 * Q6: std::function 與 function pointer？
 * A6: pointer 無 state且 ABI 簡單；std::function 可擦除有 state callable但有間接/可能配置成本。
 * Q7: weak_ptr callback 為何可能不執行？
 * A7: owner 已死亡時 lock 失敗，正確行為是略過/回報取消，而不是解參考。
 *
 * 【九、練習】
 * 1. 把 practical_router 的 std::function 換 template immediate call，說明為何不能存 heterogeneous。
 * 2. 替 leetcode_merge 寫 property test：輸出不重疊且覆蓋所有輸入區間。
 * 3. 將 weak callback 的 sentinel -1 改成 std::optional<int>。
 * 4. 寫 C callback + void* context 版 accumulator，列出 context lifetime contract。
 */

/*
==============================================================================
【面試深挖：Lambda】

L1｜lambda 的本質是什麼？
答：compiler 產生唯一 unnamed closure class，captures 成為 data members，body 成為
operator()；無 capture lambda 可轉相容 function pointer。layout/名稱仍屬實作細節。

L2｜`[=]` 與 `[&]` 最大風險不是語法，而是什麼？
答：lifetime 與 ownership。reference capture 的 closure 若逃出 scope 就 dangling；
value capture 是建立 closure 當下的 snapshot，不會自動追蹤外部後續變更。

L3｜為何 value-captured variable 預設不能修改？
答：closure::operator() 預設 const；`mutable` 移除該 const，允許改 closure 自己的 member，
不是修改原外部變數。reference capture 則可透過 reference 改外部。

L4｜捕獲 `this` 與 `*this`？
答：this capture 保存 pointer，object 先銷毀就 dangling；C++17 `[*this]` 捕獲 object copy，
有 snapshot/成本且型別需可 copy。預設 [=] 對 this 的歷史語意也易誤讀，最好明寫。

L5｜如何捕獲 move-only object？
答：C++14 init-capture `[p=std::move(p)] mutable {...}`。closure 因 member move-only 而不可 copy，
因此不能放進要求 CopyConstructible target 的舊 `std::function`；C++23 可考慮 move_only_function。

L6｜generic lambda 與 function template 的關係？
答：generic lambda 的 call operator 是 template，但 closure 還可保存 state 並作 value 傳遞。
C++20 explicit template parameter list 可直接寫 constraints/type relationships。

L7｜直接傳 lambda template parameter 與包進 `std::function` 的差別？
答：template 保留 concrete closure type，利於 inline 且無必然 allocation；std::function type-erases
並有 indirect call/可能配置，但提供固定 copyable runtime interface。按 API 邊界選。

L8｜lambda capture 的初始化時機？
答：建立 closure object 時；不是呼叫 lambda 時。value capture 看到舊值，reference capture
呼叫時看目前 object，但前提是仍存活且無 data race。

L9｜在 sort comparator 中捕獲 mutable state 為何危險？
答：sort 可複製 comparator、呼叫次數/順序未規定；若比較結果隨 state 變動，破壞 strict weak
ordering。comparator 應對同一 pair 穩定，昂貴 key 可先預計算。

L10｜`std::bind` 為何現代程式多偏好 lambda？
答：lambda 型別/捕獲/placeholder 關係更直觀，支援 move capture 與 overload disambiguation；
bind 有 eager/lazy binding、nested bind 與 value category 陷阱。bind 仍可用，但需理由。

L11｜async callback 捕獲 local reference 怎麼修？
答：若 task 可能晚於 scope，搬移所需 value/ownership 進 closure；需要共同 lifetime 可捕獲
shared_ptr，但若 callback 被 object 保存可能形成 cycle，改 weak_ptr + lock。
==============================================================================
*/

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace review {
using Interval = std::pair<int, int>;

std::vector<Interval> leetcode_merge(std::vector<Interval> intervals) {
    std::sort(intervals.begin(), intervals.end(),
              [](const Interval& left, const Interval& right) {
                  if (left.first != right.first) return left.first < right.first;
                  return left.second < right.second;  // strict，不能寫 <=
              });
    std::vector<Interval> output;
    for (const Interval& current : intervals) {
        if (output.empty() || output.back().second < current.first) {
            output.push_back(current);
        } else {
            output.back().second = std::max(output.back().second, current.second);
        }
    }
    return output;
}

class Router {
public:
    using Handler = std::function<std::string(const std::string&)>;

    // 回傳是否真的插入。這裡選擇「拒絕重複 route」；若產品需求是更新，應明確改用
    // insert_or_assign。忽略 emplace 的 bool 會讓新 handler 無聲消失。
    [[nodiscard]] bool add(std::string path, Handler handler) {
        return handlers_.emplace(std::move(path), std::move(handler)).second;
    }

    std::optional<std::string> practical_dispatch(const std::string& path,
                                                  const std::string& body) const {
        if (const auto found = handlers_.find(path); found != handlers_.end()) {
            return std::invoke(found->second, body);
        }
        return std::nullopt;
    }

private:
    std::map<std::string, Handler> handlers_;
};

class Session {
public:
    explicit Session(int initial_id) : id_(initial_id) {}
    int id() const noexcept { return id_; }
private:
    int id_;
};

auto practical_safe_callback(const std::shared_ptr<Session>& session) {
    const std::weak_ptr<Session> weak = session;
    return [weak]() -> std::optional<int> {
        if (const auto alive = weak.lock()) return alive->id();
        return std::nullopt;
    };
}

void integrated_test() {
    const std::vector<Interval> merged = leetcode_merge({{1, 3}, {2, 6}, {8, 10}});
    assert(merged == std::vector<Interval>({{1, 6}, {8, 10}}));

    Router router;
    const std::string prefix = "echo:";
    const bool route_added = router.add("/echo", [prefix](const std::string& body) {
        return prefix + body;
    });
    const bool duplicate_rejected =
        !router.add("/echo", [](const std::string&) { return "replacement"; });
    assert(route_added);
    assert(duplicate_rejected);
    assert(router.practical_dispatch("/echo", "ok") == std::optional<std::string>("echo:ok"));
    assert(!router.practical_dispatch("/missing", "").has_value());

    auto session = std::make_shared<Session>(42);
    const auto callback = practical_safe_callback(session);
    assert(callback() == std::optional<int>(42));
    session.reset();
    assert(!callback().has_value());
}
}  // namespace review

int main() {
    review::integrated_test();
    std::cout << "Lambda 面試總複習：capture、algorithm、type erasure、lifetime 皆通過\n";
}
