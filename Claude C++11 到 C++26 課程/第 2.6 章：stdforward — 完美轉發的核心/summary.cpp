// =============================================================================
//  summary.cpp  —  第 2.6 章總結：std::forward，把「值類別」原封不動送到下一層
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<utility>
//   簽名（C++11 起；C++14 起為 constexpr，C++20 起加上 [[nodiscard]]）：
//     template<class T> constexpr T&& forward(std::remove_reference_t<T>& t) noexcept;
//     template<class T> constexpr T&& forward(std::remove_reference_t<T>&& t) noexcept;
//   複雜度：O(1)，而且是「零成本」——它只是一個 static_cast，不產生任何機器碼。
//   關鍵前提：T 必須是「由 T&& 推導出來的模板參數」，否則 forward 沒有資訊可用。
//
// 【詳細解釋 Explanation】
//
// 【1. 問題的根源：具名的右值參考，本身是左值】
//   void wrapper(T&& arg) { target(arg); }   // 不論傳什麼進來，target 都收到左值
//   這不是編譯器的 bug，而是刻意的安全設計。arg 有名字，代表你可以在函式裡用它
//   很多次；如果「有名字的右值參考」自動當右值用，那麼
//       target1(arg);   // 假設這裡就把 arg 搬空
//       target2(arg);   // 這裡就拿到一個被掏空的物件
//   會變成家常便飯。C++ 的規則是：**能取名字的東西就是左值**（有身分、可重複使用）。
//   要放棄這個保護、明確宣告「我不再用它了」，必須自己寫出來——這就是 move/forward。
//
// 【2. std::forward 做了什麼：它其實只是一個「有條件的 static_cast」】
//   libstdc++ 的實作本體就一行 static_cast<T&&>(t)。神奇之處全在 T 的推導 +
//   引用折疊（reference collapsing）：
//     傳入左值 string s    → T 推導為 string& → 回傳型別 string& && = string& → 左值
//     傳入右值 string("x") → T 推導為 string  → 回傳型別 string&&           → 右值
//   引用折疊規則只有一條要記：**只要有一個是 &，結果就是 &；兩個都是 && 才是 &&**
//   （& + & = &,  & + && = &,  && + & = &,  && + && = &&）。
//   所以「值類別」的資訊其實一直都在，只是被鎖在 T 裡面；forward 負責把它解出來。
//
// 【3. forward vs move：不是兩個工具，是同一個 cast 的兩種用法】
//   move(x)       ≡ static_cast<remove_reference_t<X>&&>(x)   → 無條件變右值
//   forward<T>(x) ≡ static_cast<T&&>(x)                       → 依 T 決定
//   結論：**參數型別是 T&&（轉發參考）就用 forward；型別已經確定就用 move。**
//   反過來在轉發參考上用 move 是真正危險的——呼叫端傳進來的左值會被無聲搬空。
//
// 【4. 為什麼一定要寫 forward<T>，不能讓編譯器自己推導】
//   std::forward 的參數型別刻意寫成 remove_reference_t<T>&，這是一個
//   non-deduced context（T 出現在 :: 左邊，無法反推）。作者是故意的：
//   如果 T 可以被推導，forward(arg) 就會永遠推成左值，靜默失去作用。
//   強迫你寫 <T> 反而讓錯誤在編譯期就爆出來。
//
// 【5. 典型應用場景】
//   1. 泛型工廠：make<T>(args...) → 完美轉發到 T 的建構子（std::make_unique 的原理）
//   2. emplace_back：直接在容器記憶體上建構，省掉一次臨時物件
//   3. 包裝器（計時／日誌／重試）：轉發到被包裝的函式，不改變其行為
//   4. 多層轉發：只要每一層都寫 forward，值類別可以穿越任意多層
//   5. auto&&：range-based for 中以轉發參考接住元素，不論左右值都不複製
//
// 【概念補充 Concept Deep Dive】
//   (A) forward 是純粹的編譯期概念，執行期什麼都沒做。
//       static_cast 只改變「編譯器怎麼看這個運算式」，不生成指令。開了最佳化之後
//       forward 連函式呼叫都不會留下（它是 inline + constexpr）。因此
//       「加了 forward 會不會變慢」是不成立的問題——它只會讓你少複製一次。
//   (B) T&& 何時是「轉發參考」，何時是「真右值參考」？
//       只有在 **T 是這個函式自己推導出來的模板參數** 時才是轉發參考。
//         template<class T> void f(T&& x);               // 轉發參考
//         template<class T> void f(std::vector<T>&& x);  // 真右值參考（T 不是裸的）
//         template<class T> struct S { void f(T&& x); }; // 真右值參考（T 已固定）
//       auto&& 也是轉發參考，因為 auto 的推導規則與模板參數相同。
//   (C) 為什麼 forward 是 noexcept？因為它只是型別轉換，不可能丟例外；
//       標成 noexcept 讓呼叫端的 noexcept 運算能正確往上傳遞。
//
// 【注意事項 Pay Attention】
//   1. forward 只能用在「該參數的最後一次使用」。轉發完就當它已經被搬走。
//   2. 迴圈裡不可以 forward 同一個參數——第一圈之後它可能已經是空殼。
//   3. forward<T> 的 T 要寫模板參數本身，不是 decay 過的型別。寫成
//      forward<std::decay_t<T>>(arg) 會把左值變成右值，等於偷偷 move。
//   4. 轉發參考做建構子會搶走複製建構子（見第 2.7 章的 enable_if 解法）。
//   5. 回傳值也要轉發：decltype(auto) 或 -> decltype(...) 才不會把右值降級。
//   6. 「被移動後的物件」處於 valid but unspecified 狀態：可以安全解構與再賦值，
//      但不可假設它一定變成空字串。本檔輸出中被移動的 std::string 顯示為空，
//      是 libstdc++ 的實作行為，不是標準保證。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::forward 與完美轉發
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 void f(T&& arg) { g(arg); } 沒有做到完美轉發？
//     答：因為 arg 是「具名」的右值參考，而具名的東西一律是左值。
//         值類別的資訊還在 T 裡（左值時 T = X&，右值時 T = X），
//         但 arg 這個運算式本身已經是左值，所以 g 永遠選到左值多載。
//     追問：那為什麼標準不讓具名右值參考自動當右值？
//         → 因為它可以被使用多次；自動 move 會讓 g1(arg); g2(arg); 的第二行拿到空殼。
//
// 🔥 Q2. std::forward 和 std::move 的差別是什麼？
//     答：兩者都是 static_cast，差在條件。move 是無條件轉成右值；
//         forward<T> 依 T 決定——T 推導成 X& 時折疊回左值，T 推導成 X 時才是右值。
//         用法準則：參數寫成 T&&（轉發參考）就用 forward，型別已固定就用 move。
//     追問：在 T&& 參數上寫 std::move 會怎樣？
//         → 呼叫端傳進來的左值會被無聲搬空，這是實務上最常見的 move 事故。
//
// 🔥 Q3. 為什麼一定要寫 std::forward<T>(arg)，不能寫 std::forward(arg)？
//     答：forward 的參數型別是 remove_reference_t<T>&，T 位於 :: 左側，
//         屬於 non-deduced context，本來就推導不出來。這是刻意設計——
//         若可推導，forward(arg) 會永遠推成左值而靜默失效，變成很難查的 bug。
//
// ⚠️ 陷阱. 「模板參數寫成 T&& 就一定是轉發參考」——為什麼這是錯的？
//     答：只有當 T 是「這個函式自己要推導的模板參數」時才是轉發參考。
//         template<class T> void f(std::vector<T>&& v);   // 真右值參考，不吃左值
//         template<class T> struct S { void g(T&& x); };  // T 由類別固定，真右值參考
//     為什麼會錯：多數人把「&&」當成語法特徵去記，但轉發參考是「推導行為」的
//         特徵而非語法特徵。判準只有一個：這個 && 前面的型別，是不是正在被推導。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>
#include <memory>
#include <vector>
#include <chrono>
#include <list>
#include <unordered_map>

// ============================================================
// 目標函式（重載左值/右值版本）
// ============================================================
// 參數刻意不命名：本示範只關心「選到哪個多載」，不使用參數值。
// 命名但不使用會觸發 -Wextra 的 -Wunused-parameter。
void target(const std::string&) { std::cout << "  target(const T&) 左值\n"; }
void target(std::string&&)      { std::cout << "  target(T&&) 右值\n"; }

// ============================================================
// 錯誤寫法：三種失敗的嘗試
// ============================================================
template<typename T>
void wrapper_bad1(const T& arg) {
    target(arg);  // 永遠是左值（const T& 遮蓋了右值資訊）
}

template<typename T>
void wrapper_bad2(T&& arg) {
    target(arg);  // arg 有名字 → 永遠是左值
}

// ============================================================
// 正確寫法：std::forward
// ============================================================
template<typename T>
void wrapper_good(T&& arg) {
    target(std::forward<T>(arg));  // ★ 保持原始值類別
}

// ============================================================
// 比較三種行為
// ============================================================
void check(const std::string&) { std::cout << "  → 左值\n"; }
void check(std::string&&)      { std::cout << "  → 右值\n"; }

template<typename T>
void compare_three(T&& arg) {
    std::cout << "  直接傳 arg:       ";
    check(arg);                          // 永遠左值

    std::cout << "  std::move(arg):   ";
    check(std::move(arg));               // 永遠右值

    std::cout << "  std::forward<T>:  ";
    check(std::forward<T>(arg));         // 保持原始 ✅
}

// ============================================================
// 應用 1：泛型工廠函式
// ============================================================
class Person {
    std::string name_;
    int age_;
public:
    Person(const std::string& n, int a) : name_(n), age_(a) {
        std::cout << "    Person(const string&) 複製\n";
    }
    Person(std::string&& n, int a) : name_(std::move(n)), age_(a) {
        std::cout << "    Person(string&&) 移動\n";
    }
    void print() const { std::cout << "    " << name_ << ", age " << age_ << "\n"; }
};

template<typename T, typename... Args>
std::unique_ptr<T> make(Args&&... args) {
    return std::unique_ptr<T>(
        new T(std::forward<Args>(args)...)  // ★ 完美轉發所有引數
    );
}

// ============================================================
// 應用 2：多層轉發
// ============================================================
void final_target(const std::string&) { std::cout << "  最終: 左值\n"; }
void final_target(std::string&&)      { std::cout << "  最終: 右值\n"; }

template<typename T>
void layer3(T&& arg) { final_target(std::forward<T>(arg)); }

template<typename T>
void layer2(T&& arg) { layer3(std::forward<T>(arg)); }

template<typename T>
void layer1(T&& arg) { layer2(std::forward<T>(arg)); }

// ============================================================
// 應用 3：計時包裝器
// ============================================================
template<typename Func, typename... Args>
auto timed_call(Func&& func, Args&&... args)
    -> decltype(std::forward<Func>(func)(std::forward<Args>(args)...))
{
    auto start = std::chrono::high_resolution_clock::now();
    auto result = std::forward<Func>(func)(std::forward<Args>(args)...);
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    // 耗時寫到 stderr：每次執行都不同，不該混進可重現的 stdout。
    std::cerr << "    [計時] 耗時: " << us << " us\n";
    return result;
}

std::string process(const std::string& input, int repeat) {
    std::string result;
    for (int i = 0; i < repeat; ++i) result += input;
    return result;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   題目：設計一個容量固定的 LRU 快取，get 與 put 都要 O(1)。
//   為什麼用到本主題：標準解法是 list<pair<int,int>> + unordered_map。
//     每次 put 新鍵都要在 list 前端生出一個節點——用 emplace_front(key, value)
//     可以「把 key 與 value 直接轉發到 pair 的建構子」，在節點記憶體上就地建構，
//     省掉「先做一個 pair 臨時物件、再搬進 list」的那一次搬移。
//     emplace 家族的整個能力來源就是本章的完美轉發。
// -----------------------------------------------------------------------------
class LRUCache {
    using Item = std::pair<int, int>;                 // (key, value)
    std::size_t cap_;
    std::list<Item> order_;                           // 前端 = 最近使用
    std::unordered_map<int, std::list<Item>::iterator> pos_;

public:
    explicit LRUCache(std::size_t capacity) : cap_(capacity) {}

    int get(int key) {
        auto it = pos_.find(key);
        if (it == pos_.end()) return -1;
        // splice 只改指標、不搬資料，是 list 專屬的 O(1) 搬移
        order_.splice(order_.begin(), order_, it->second);
        return it->second->second;
    }

    void put(int key, int value) {
        auto it = pos_.find(key);
        if (it != pos_.end()) {
            it->second->second = value;
            order_.splice(order_.begin(), order_, it->second);
            return;
        }
        if (order_.size() == cap_) {
            pos_.erase(order_.back().first);
            order_.pop_back();
        }
        // ★ 完美轉發：key/value 直接送進 pair<int,int> 的建構子，不做臨時 pair
        order_.emplace_front(key, value);
        pos_[key] = order_.begin();
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】以完美轉發實作「連線池取用包裝器」
//   情境：服務啟動時要建立多種資源（DB 連線、快取連線、訊息佇列），
//         每種建構參數的數量與型別都不同。若不用完美轉發，工廠函式就得為
//         「幾個參數 × 左值/右值」的每種組合各寫一個多載（2^n 爆炸）。
//   用完美轉發後，一個 acquire<T>(args...) 就涵蓋全部，而且左值照樣複製、
//   右值照樣移動，呼叫端的語意完全不被包裝器改變。
// -----------------------------------------------------------------------------
class DbConnection {
    std::string dsn_;
    int timeout_ms_;
public:
    DbConnection(const std::string& dsn, int timeout_ms)
        : dsn_(dsn), timeout_ms_(timeout_ms) {
        std::cout << "    建立連線(複製 dsn): " << dsn_
                  << " timeout=" << timeout_ms_ << "ms\n";
    }
    DbConnection(std::string&& dsn, int timeout_ms)
        : dsn_(std::move(dsn)), timeout_ms_(timeout_ms) {
        std::cout << "    建立連線(移動 dsn): " << dsn_
                  << " timeout=" << timeout_ms_ << "ms\n";
    }
};

template<typename Resource, typename... Args>
Resource acquire(Args&&... args) {
    std::cout << "  [pool] 取用資源\n";
    return Resource(std::forward<Args>(args)...);   // ★ 原封不動轉發
}

int main() {
    std::string s = "Hello";

    // ============================================================
    // 1. 問題展示：不用 forward 的結果
    // ============================================================
    std::cout << "===== 1. 不用 forward 的問題 =====\n";
    std::cout << "  wrapper_bad1（const T&）:\n";
    wrapper_bad1(s);                     // 左值 ✅
    wrapper_bad1(std::string("tmp"));    // 應右值 → 卻是左值 ❌
    std::cout << "  wrapper_bad2（T&& 但不 forward）:\n";
    wrapper_bad2(s);                     // 左值 ✅
    wrapper_bad2(std::string("tmp"));    // 應右值 → 卻是左值 ❌
    std::cout << "\n";

    // ============================================================
    // 2. 正確寫法：std::forward
    // ============================================================
    std::cout << "===== 2. std::forward =====\n";
    std::cout << "  傳入左值:\n";
    wrapper_good(s);                     // 左值 ✅
    std::cout << "  傳入右值:\n";
    wrapper_good(std::string("tmp"));    // 右值 ✅
    std::cout << "  傳入 std::move:\n";
    wrapper_good(std::move(s));          // 右值 ✅
    s = "Hello";  // 恢復
    std::cout << "\n";

    // ============================================================
    // 3. 三種行為比較
    // ============================================================
    std::cout << "===== 3. 三種行為比較 =====\n";
    std::cout << "  傳入左值:\n";
    compare_three(s);
    std::cout << "  傳入右值:\n";
    compare_three(std::string("tmp"));
    std::cout << "\n";

    // ============================================================
    // 4. 泛型工廠函式
    // ============================================================
    std::cout << "===== 4. 泛型工廠函式 =====\n";
    std::string name = "Alice";
    std::cout << "  傳入左值:\n";
    auto p1 = make<Person>(name, 30);     // 複製
    std::cout << "  傳入右值:\n";
    auto p2 = make<Person>(std::string("Bob"), 25);  // 移動
    std::cout << "\n";

    // ============================================================
    // 5. 多層轉發
    // ============================================================
    std::cout << "===== 5. 多層轉發（穿越三層）=====\n";
    std::cout << "  傳入左值:\n";
    layer1(s);                            // 穿越三層後仍是左值
    std::cout << "  傳入右值:\n";
    layer1(std::string("tmp"));           // 穿越三層後仍是右值
    std::cout << "\n";

    // ============================================================
    // 6. emplace_back vs push_back
    // ============================================================
    std::cout << "===== 6. emplace_back（內部使用 forward）=====\n";
    {
        struct Entry {
            std::string key;
            int val;
            Entry(const std::string& k, int v) : key(k), val(v) {
                std::cout << "    Entry(const string&)\n";
            }
            Entry(std::string&& k, int v) : key(std::move(k)), val(v) {
                std::cout << "    Entry(string&&)\n";
            }
        };

        std::vector<Entry> vec;
        vec.reserve(2);
        std::string key = "alpha";

        std::cout << "  emplace_back 傳左值:\n";
        vec.emplace_back(key, 1);       // 完美轉發 → const string&

        std::cout << "  emplace_back 傳右值:\n";
        vec.emplace_back(std::string("beta"), 2);  // 完美轉發 → string&&
    }
    std::cout << "\n";

    // ============================================================
    // 7. 計時包裝器
    // ============================================================
    std::cout << "===== 7. 計時包裝器 =====\n";
    {
        std::string input = "Hi";
        auto result = timed_call(process, input, 10000);
        std::cout << "    結果長度: " << result.size() << "\n";
    }

    // ============================================================
    // 8. LeetCode 146. LRU Cache（emplace_front = 完美轉發）
    // ============================================================
    std::cout << "\n=== LeetCode 146. LRU Cache ===\n";
    {
        LRUCache cache(2);
        cache.put(1, 1);
        cache.put(2, 2);
        std::cout << "  get(1) = " << cache.get(1) << "\n";   // 1
        cache.put(3, 3);                                      // 淘汰 key 2
        std::cout << "  get(2) = " << cache.get(2) << "\n";   // -1（已被淘汰）
        cache.put(4, 4);                                      // 淘汰 key 1
        std::cout << "  get(1) = " << cache.get(1) << "\n";   // -1
        std::cout << "  get(3) = " << cache.get(3) << "\n";   // 3
        std::cout << "  get(4) = " << cache.get(4) << "\n";   // 4
    }

    // ============================================================
    // 9. 日常實務：連線池取用包裝器
    // ============================================================
    std::cout << "\n=== 日常實務：連線池取用包裝器 ===\n";
    {
        std::string dsn = "postgres://10.0.0.7:5432/orders";
        auto c1 = acquire<DbConnection>(dsn, 3000);                    // 左值 → 複製
        auto c2 = acquire<DbConnection>(std::string("redis://cache:6379"), 500);  // 右值 → 移動
        (void)c1; (void)c2;
        std::cout << "  呼叫端的 dsn 仍完好: " << dsn << "\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  直接傳 arg → 永遠左值\n";
    std::cout << "  std::move(arg) → 永遠右值\n";
    std::cout << "  std::forward<T>(arg) → 保持原始值類別 ✅\n";
    std::cout << "  必須搭配 T&& 模板參數使用\n";
    std::cout << "  典型應用：工廠函式、emplace_back、包裝器\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// 註：timed_call 的耗時數字每次執行都不同，已改寫到 stderr，
//     因此不出現在下方（stdout）的預期輸出中。
//     被 std::move 之後的 std::string 顯示為空字串，是 libstdc++ 的實作行為；
//     標準只保證「valid but unspecified」，不保證一定變成空。

// === 預期輸出 ===
// ===== 1. 不用 forward 的問題 =====
//   wrapper_bad1（const T&）:
//   target(const T&) 左值
//   target(const T&) 左值
//   wrapper_bad2（T&& 但不 forward）:
//   target(const T&) 左值
//   target(const T&) 左值
//
// ===== 2. std::forward =====
//   傳入左值:
//   target(const T&) 左值
//   傳入右值:
//   target(T&&) 右值
//   傳入 std::move:
//   target(T&&) 右值
//
// ===== 3. 三種行為比較 =====
//   傳入左值:
//   直接傳 arg:         → 左值
//   std::move(arg):     → 右值
//   std::forward<T>:    → 左值
//   傳入右值:
//   直接傳 arg:         → 左值
//   std::move(arg):     → 右值
//   std::forward<T>:    → 右值
//
// ===== 4. 泛型工廠函式 =====
//   傳入左值:
//     Person(const string&) 複製
//   傳入右值:
//     Person(string&&) 移動
//
// ===== 5. 多層轉發（穿越三層）=====
//   傳入左值:
//   最終: 左值
//   傳入右值:
//   最終: 右值
//
// ===== 6. emplace_back（內部使用 forward）=====
//   emplace_back 傳左值:
//     Entry(const string&)
//   emplace_back 傳右值:
//     Entry(string&&)
//
// ===== 7. 計時包裝器 =====
//     結果長度: 20000
//
// === LeetCode 146. LRU Cache ===
//   get(1) = 1
//   get(2) = -1
//   get(1) = -1
//   get(3) = 3
//   get(4) = 4
//
// === 日常實務：連線池取用包裝器 ===
//   [pool] 取用資源
//     建立連線(複製 dsn): postgres://10.0.0.7:5432/orders timeout=3000ms
//   [pool] 取用資源
//     建立連線(移動 dsn): redis://cache:6379 timeout=500ms
//   呼叫端的 dsn 仍完好: postgres://10.0.0.7:5432/orders
//
// === 重點整理 ===
//   直接傳 arg → 永遠左值
//   std::move(arg) → 永遠右值
//   std::forward<T>(arg) → 保持原始值類別 ✅
//   必須搭配 T&& 模板參數使用
//   典型應用：工廠函式、emplace_back、包裝器
