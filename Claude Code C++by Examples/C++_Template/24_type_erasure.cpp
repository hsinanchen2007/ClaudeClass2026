// ============================================================================
//  24_type_erasure.cpp  ──  Type Erasure
// ============================================================================
//
//  【本篇目標】
//    Type erasure 是「在 runtime 隱藏具體型別、僅暴露介面」的技巧。經典範
//    例：std::function、std::any、std::shared_ptr 的 deleter。本篇手寫一
//    個迷你 std::function，讓你看清楚原理。
//
//  【動機】
//    考慮 callback 容器：你想存一堆「(int) -> int」的 callable，但實際 T
//    可能是 lambda、function pointer、functor，型別都不同。
//
//      std::vector<???> callbacks;     // ??? 是什麼？
//
//    答案：std::function<int(int)>。它能容納「任何 callable」，背後就是
//    type erasure。
//
//  【核心技巧】
//
//    用「外部容器類別」+「內部多型介面」+「具體 wrapper」三層結構：
//
//        // 對外的容器
//        class Function {
//            struct ConceptBase {                    // ① abstract interface
//                virtual ~ConceptBase() = default;
//                virtual int call(int) = 0;
//            };
//            template<typename F>
//            struct Model : ConceptBase {            // ② concrete wrapper
//                F f;
//                Model(F func) : f(std::move(func)) {}
//                int call(int x) override { return f(x); }
//            };
//            std::unique_ptr<ConceptBase> impl_;
//        public:
//            template<typename F>
//            Function(F f) : impl_(std::make_unique<Model<F>>(std::move(f))) {}
//            int operator()(int x) { return impl_->call(x); }
//        };
//
//    具體 F 在 Model<F> 知道，但對外只看見 ConceptBase ── F 被「擦除」掉。
//
//  【代價與時機】
//    - 動態配置 (heap) + 虛擬呼叫 → 比直接 inline 慢一點。
//    - 但換來「容器同質化」、「介面統一」。
//    - 適合：callback 系統、event handler、plugin、observer。
//
//  【對比 std::variant】
//    - variant：closed set，事先列出所有型別 (variant<int, string, ...>)
//    - type erasure：open set，任意 callable / 任意有指定介面的型別都可
//
//  參考：
//    https://en.cppreference.com/cpp/utility/functional/function
// ============================================================================

/*
補充筆記：type_erasure
  - type erasure 把具體型別藏在統一介面後面，例如 std::function、std::any。
  - 它換來 runtime 彈性，但可能付出間接呼叫、配置與錯誤延後的成本。
  - 若型別集合封閉，variant 往往比 type erasure 更可檢查。
  - type_erasure 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Type Erasure
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 type erasure？標準庫的例子？
//     答：把具體型別藏起來、對外只留統一介面，內部靠虛擬呼叫（或函式指標表）分派。
//         典型就是本檔的三層結構：Concept（抽象介面）+ Model<F>（具體包裝）
//         + 對外容器類別。標準庫例子：std::function、std::any、
//         以及 std::shared_ptr 的 deleter。
//     追問：和 std::variant 差在哪？（variant 是封閉集合、編譯期決定、不需 heap；
//         type erasure 是開放集合，任何符合介面的型別都能塞進來）
//
// 🔥 Q2. 為什麼 shared_ptr 的 deleter 不在型別裡，unique_ptr 的卻在？
//     答：shared_ptr<T> 把 deleter 用 type erasure 收進控制區塊，所以帶不同 deleter
//         的 shared_ptr<T> 仍是同一個型別、可互相賦值；unique_ptr<T, D> 把 D 寫進
//         型別，換 deleter 就是換型別，代價是型別不相容，好處是無狀態 deleter 可被
//         EBO 吃掉、也沒有多一層間接呼叫。
//
// 🔥 Q3. type erasure vs CRTP vs virtual 三者怎麼選？
//     答：virtual＝侵入式（對方必須繼承你的基底）+ 執行期多型；
//         CRTP＝非侵入但型別編譯期固定，做不出異質容器；
//         type erasure＝非侵入 + 執行期多型，代價是一次間接呼叫與可能的堆積配置。
//         「別人的型別（含 lambda）一行都不用改就能放進同一個容器」只有它做得到。
//
// ⚠️ 陷阱. std::function 一定會做 heap allocation 嗎？
//     答：不一定。多數實作有 small buffer optimization，夠小的 callable 會直接放在
//         function 內部緩衝區。但「多小才算小」是 implementation-defined、
//         標準未規定，也沒有承諾任何大小以下必定不配置，因此不能拿來當效能保證。
//     為什麼會錯：看到「type erasure 要 heap」就以為每次都 new，
//         或反過來以為只捕獲一兩個 int 的 lambda 一定不會配置。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// ─── 1. 自製 mini Function<R(Args...)> ───────────────────────────────────
template <typename Sig> class Function;          // primary template

template <typename R, typename... Args>
class Function<R(Args...)> {
public:
    Function() = default;

    // 接受任何 callable 物件
    template <typename F>
    Function(F f) : impl_(std::make_unique<Model<F>>(std::move(f))) {}

    R operator()(Args... a) const {
        return impl_->call(std::forward<Args>(a)...);
    }

    bool valid() const { return impl_ != nullptr; }

private:
    struct Concept {
        virtual ~Concept() = default;
        virtual R call(Args...) = 0;
    };

    template <typename F>
    struct Model : Concept {
        F f;
        Model(F func) : f(std::move(func)) {}
        R call(Args... a) override {
            return f(std::forward<Args>(a)...);
        }
    };

    std::unique_ptr<Concept> impl_;
};

// 普通函式：容易測試 callback
int square(int x) { return x * x; }

// ─── 2. Leetcode 384 ── Shuffle an Array (用 Function 做 RNG callback) ──
//   題目：
//     Solution(nums)：建構，存一份 nums。
//     reset()：回原始。
//     shuffle()：回隨機洗牌結果，每種排列等機率。
//
//   經典解法：Fisher–Yates shuffle (倒序)：對每個 i，從 [0..i] 隨機選 j 交換。
//
//   時間：reset O(1)；shuffle O(n)
//   空間：O(n)
//
//   為什麼放這裡？
//     在 shuffle 內，我們需要一個「隨機數產生器」。把它做成 Function<int(int)>
//     允許 caller 注入 RNG (測試方便、不依賴全域亂數)：把任何 lambda、std::mt19937
//     wrapper、確定性 mock 都能傳進來。
class Solution {
public:
    explicit Solution(std::vector<int> nums) : orig_(std::move(nums)) {}
    std::vector<int> reset() const { return orig_; }

    // rand_ub(n) 必須回傳 [0..n) 範圍內整數
    std::vector<int> shuffle(Function<int(int)> rand_ub) const {
        std::vector<int> r = orig_;
        for (int i = (int)r.size() - 1; i > 0; --i) {
            int j = rand_ub(i + 1);
            std::swap(r[i], r[j]);
        }
        return r;
    }

private:
    std::vector<int> orig_;
};

// ─── 3. 工作實用：Event handler 系統 ────────────────────────────────────
//   存一堆同一介面的 callable，事件來時逐一呼叫。
//   用 vector<Function<void(string)>>：lambda、自定 functor、函式指標都能塞。
class EventBus {
public:
    using Handler = Function<void(const std::string&)>;

    void subscribe(Handler h) { handlers_.push_back(std::move(h)); }

    void publish(const std::string& msg) const {
        for (const auto& h : handlers_) h(msg);
    }

private:
    std::vector<Handler> handlers_;
};

// ─── 4. Leetcode 1672 ── Richest Customer Wealth (用 Function 注入聚合策略) ─
//   難度: easy
//   題目：accounts[i][j] = 第 i 顧客在第 j 銀行的存款；回傳最有錢者的總財產。
//
//   為什麼放在這裡？
//     用 type erasure 把「列聚合策略」抽象為 Function<int(const vector<int>&)>，
//     使用者可注入「sum / max / 自定」等任意聚合方式 — 例如「全部相加」、
//     「只看最大筆存款」等。
int maximum_wealth_with_agg(
    const std::vector<std::vector<int>>& accounts,
    Function<int(const std::vector<int>&)> aggregate)
{
    int best = 0;
    for (const auto& row : accounts) {
        int v = aggregate(row);
        if (v > best) best = v;
    }
    return best;
}

// ─── 5. 工作實用：泛型 Cache，用 Function 做 「miss-fetcher」 ───────────
//   把「快取找不到時要怎麼取得新值」抽成 callable，使用者注入；缓存類
//   不必知道資料來源 (檔案、API、DB 都可)。
template <typename K, typename V>
class LazyCache {
public:
    using Fetcher = Function<V(const K&)>;
    explicit LazyCache(Fetcher f) : fetch_(std::move(f)) {}

    V get(const K& key) {
        auto it = data_.find(key);
        if (it != data_.end()) return it->second;
        V v = fetch_(key);
        data_[key] = v;
        return v;
    }

    std::size_t size() const { return data_.size(); }

private:
    Fetcher fetch_;
    std::unordered_map<K, V> data_;
};

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) mini Function
    Function<int(int)> f1 = square;                             // 函式指標
    Function<int(int)> f2 = [](int x) { return x + 1; };        // lambda
    struct Triple { int operator()(int x) const { return x * 3; } };
    Function<int(int)> f3 = Triple{};                           // functor
    std::cout << "f1(5) = " << f1(5) << "\n";
    std::cout << "f2(5) = " << f2(5) << "\n";
    std::cout << "f3(5) = " << f3(5) << "\n";

    // (2) Leetcode 384
    Solution sol(std::vector<int>{1, 2, 3, 4, 5});
    std::mt19937 gen(42);                  // 固定 seed → 結果可重現
    auto rng = [&gen](int n) {
        return std::uniform_int_distribution<int>(0, n - 1)(gen);
    };
    auto shuffled = sol.shuffle(rng);
    std::cout << "shuffled: ";
    for (int x : shuffled) std::cout << x << ' ';
    std::cout << "\n";
    auto resetted = sol.reset();
    std::cout << "reset:    ";
    for (int x : resetted) std::cout << x << ' ';
    std::cout << "\n";

    // (3) EventBus
    EventBus bus;
    bus.subscribe([](const std::string& m) { std::cout << "[A] " << m << "\n"; });
    bus.subscribe([](const std::string& m) { std::cout << "[B] " << m << "\n"; });
    bus.publish("hello");
    bus.publish("world");

    // (4) Leetcode 1672 — 用 sum 策略
    std::vector<std::vector<int>> accounts{{1, 5}, {7, 3}, {3, 5}};
    auto sum_row = [](const std::vector<int>& row) {
        int s = 0; for (int x : row) s += x; return s;
    };
    std::cout << "maximum_wealth(sum)   = "
              << maximum_wealth_with_agg(accounts, sum_row) << " (expect 10)\n";

    // (5) LazyCache：value = key 的平方，且只算一次
    int calls = 0;
    LazyCache<int, int> sqcache([&calls](const int& k) {
        ++calls;
        return k * k;
    });
    std::cout << "sqcache(5)  = " << sqcache.get(5)  << " calls=" << calls << "\n";
    std::cout << "sqcache(5)  = " << sqcache.get(5)  << " calls=" << calls << " (hit)\n";
    std::cout << "sqcache(10) = " << sqcache.get(10) << " calls=" << calls << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::function 是怎麼實作的？SBO 又是什麼？
    //    A：典型實作是「concept + model」：抽象基底定義 invoke()/clone()/destroy()
    //       virtual 介面，模板衍生類別包住具體 callable。為了避免每次都 heap 配
    //       置，多數 STL 採 Small Buffer Optimization：物件夠小就直接放在
    //       function 內部 buffer，否則 fallback 到 heap。
    //
    //  Q2：type erasure 比 CRTP / virtual 直接繼承有什麼優勢？
    //    A：使用者不必繼承我們的基底，任何符合「duck typing」介面的型別（甚至
    //       lambda）都能塞進來；介面與實作解耦，library author 換資料結構不影響
    //       client。代價是一個 virtual call + 可能的 heap allocation。
    //
    //  Q3：std::any 與 std::function、std::variant 的關鍵差異？
    //    A：std::any 存「任意型別」但無預設操作介面，要 any_cast 取出原型別才能
    //       用；std::function 存「可呼叫的東西」並提供統一 operator()；std::variant
    //       則是「有限封閉集合」，型別是編譯期決定，不需 heap、可用 visit。三者
    //       目的不同：unbounded any、callable、closed sum type。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. Type erasure = 隱藏具體型別、暴露固定介面、底層用虛擬呼叫實作。
//    2. std::function、std::any、std::shared_ptr deleter 都是同樣模式。
//    3. 缺點：堆配置 + 虛擬呼叫 → 比 inline 慢；但換來容器同質化。
//    4. 工作上 callback / event 系統幾乎一定會用到。
//
//  【下一篇】
//    25_template_compilation_model.cpp ── 為什麼 template 要放 header？
// ============================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 24_type_erasure.cpp -o 24_type_erasure

// === 預期輸出 ===
// f1(5) = 25
// f2(5) = 6
// f3(5) = 15
// shuffled: 5 1 3 4 2
// reset:    1 2 3 4 5
// [A] hello
// [B] hello
// [A] world
// [B] world
// maximum_wealth(sum)   = 10 (expect 10)
// sqcache(5)  = 25 calls=1
// sqcache(5)  = 25 calls=1 (hit)
// sqcache(10) = 100 calls=2
