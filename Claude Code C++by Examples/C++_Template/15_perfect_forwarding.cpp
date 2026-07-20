// ============================================================================
//  15_perfect_forwarding.cpp  ──  Universal Reference + Perfect Forwarding
// ============================================================================
//
//  【本篇目標】
//    這是現代 C++ 最重要的觀念之一，但也是最容易搞混的。
//    重點：
//      ① T&& 在 template 內叫「universal reference / forwarding reference」，
//         不是 rvalue reference。
//      ② std::forward<T>(x) 的角色：把 x 「按原本的 lvalue/rvalue 屬性」傳出去。
//      ③ Reference collapsing rules：T& + & = T&，T&& + & = T&，T& + && = T&，
//         T&& + && = T&&。
//      ④ 為什麼 std::move 不夠：move 永遠把參數變 rvalue；forward 才會「視
//         情況保留」。
//
// ----------------------------------------------------------------------------
//  【術語區別 ── rvalue reference vs forwarding reference】
//
//    這兩個語法都長 T&&，但意義天差地別：
//
//    (a) rvalue reference (具體型別)：
//          void f(int&& x);          // x 一定綁右值
//          std::string&& s = ...;
//
//    (b) forwarding reference (template 推導)：
//          template<typename T>
//          void f(T&& x);            // x 是「forwarding reference」
//
//    判斷標準：T&& 中的 T 必須是「正在被推導的 template 參數」，否則就是
//    一般 rvalue reference。例如：
//
//          template<typename T>
//          void g(std::vector<T>&& v);   // ← 這是 rvalue reference!
//                                        //   T 在這裡不是頂層 template 參數
//                                        //   是 vector<T> 的 T
//
// ----------------------------------------------------------------------------
//  【Reference Collapsing】
//
//    當 T&& 被推導時：
//      - 傳入 lvalue (例如 int x; f(x))：T 推為 int& → T&& 變 int& && → 折疊為 int&
//      - 傳入 rvalue (例如 f(42))：       T 推為 int  → T&& 變 int&&
//
//    結果：
//      - 傳 lvalue → x 是 lvalue reference
//      - 傳 rvalue → x 是 rvalue reference
//      → 完美保留呼叫端的「值類別 (value category)」!
//
// ----------------------------------------------------------------------------
//  【std::forward<T>(x) 的角色】
//
//    forward 是個有條件的 move：
//      - 若 T 推導為 lvalue reference (T = int&)：forward 回傳 lvalue reference
//      - 若 T 推導為 rvalue (T = int)：forward 回傳 rvalue reference
//
//    一句話：「依 T 而定，把參數的 lvalue/rvalue 性質還原回去再傳遞」。
//
//    為什麼不能用 std::move？
//      因為 move 不分青紅皂白，永遠回傳 rvalue reference。如果使用者傳 lvalue，
//      你卻 move 出去 → 偷了人家的物件，可能毀掉對方的後續邏輯。
//
//    寫法：
//          template<typename T>
//          void wrapper(T&& x) {
//              callee(std::forward<T>(x));   // ← T 必須帶上！否則無法判斷
//          }
//
// ----------------------------------------------------------------------------
//  【經典應用：emplace / make_unique / factory】
//
//    所有「把任意參數轉發給 constructor」的工具都是這個模式：
//
//          template<typename T, typename... Args>
//          unique_ptr<T> make_unique(Args&&... args) {
//              return unique_ptr<T>(new T(std::forward<Args>(args)...));
//          }
//
//  參考：
//    https://en.cppreference.com/cpp/utility/forward
//    https://en.cppreference.com/cpp/language/reference#Forwarding_references
// ============================================================================

/*
補充筆記：perfect_forwarding
  - perfect forwarding 的目標是讓 wrapper 保留呼叫端傳入參數的 lvalue/rvalue 屬性。
  - T&& 只有在 T 被推導時才是 forwarding reference。
  - std::forward<T>(x) 依 T 還原 value category；std::move 則無條件轉成 rvalue。
  - perfect_forwarding 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】perfect forwarding / forwarding reference
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. T&& 什麼時候是 forwarding reference，什麼時候只是右值參考？
//     答：只有當 T 是「這個函式模板正在推導」的參數時，T&& 才是 forwarding
//         reference（可綁左值也可綁右值）。以下三種都只是普通右值參考：
//         const T&&（加了 const 就不是，本機實測傳左值直接編譯錯）、
//         std::vector<T>&&（T 不在頂層）、類別模板成員函式裡已固定的 T&&。
//         另外 auto&& 也是 forwarding reference。
//     追問：推導規則？（傳左值 T 推成 U&，傳右值 T 推成 U，再套 reference collapsing）
//
// 🔥 Q2. 引用摺疊規則是什麼？std::move 為什麼要先 remove_reference？
//     答：四條規則 & & → &、& && → &、&& & → &、&& && → &&，一句話：
//         「只要有一個左值參考，結果就是左值參考」。std::move 若不先剝掉參考，
//         傳左值時 T 推成 U&，回傳型別 T&& 會摺疊回 U&——根本 cast 不出右值
//         （本機以 static_assert 實測確認會得到 lvalue reference）。
//     追問：那 forward 為何反而靠這個摺疊？（它就是用 T 帶不帶 & 來決定還原成
//         左值還是右值，摺疊是它的運作機制而不是障礙）
//
// 🔥 Q3. 為什麼 std::forward 一定要顯式寫 <T>，std::move 卻不用？
//     答：forward 是「條件式」轉型，判斷依據就是 T 帶不帶 &。而函式內的 x 是
//         具名變數、表達式永遠是左值，光看實參推不出呼叫端原本的值類別，
//         所以 T 不能省——省略會編譯失敗（本機實測：no matching function for
//         call to forward(int&)）。std::move 是無條件轉型，不需要這個資訊。
//
// ⚠️ 陷阱 1. template<class T> Person(T&&) 這種建構子為什麼會「吃掉」複製建構？
//     答：forwarding reference 是貪婪的精準匹配。傳 non-const 左值時，複製建構子
//         要求 const Person& 需要加上 const，而模板版 T=Person& 是精準匹配 →
//         選中模板（本機實測：non-const 左值走模板、const 左值才走 copy ctor）。
//         同理 g(short) 也會被 g(T&&) 搶走，因為非模板的 g(int) 需要整數提升。
//     為什麼會錯：以為「複製建構子是特殊成員、一定優先」。它一點都不特殊，就是
//         重載決議裡的一個候選，而且經常輸給精準匹配的模板。
//         解法：enable_if 排除自身型別（!is_base_of_v<Person, decay_t<T>>）、
//         tag dispatch，或 C++20 concepts。
//
// ⚠️ 陷阱 2. 「完美」轉發什麼時候並不完美？
//     答：至少五種（EMC++ Item 30）：(a) braced initializer——f({1,2,3}) 的
//         {...} 是 non-deduced context，T 根本推不出來（解法：先 auto il={1,2,3};
//         再 f(il)）；(b) 0 或 NULL 當空指標會被推成 int 而非 nullptr_t；
//         (c) 只宣告未定義的 static const 成員，轉發會取址 → 連結錯誤；
//         (d) 重載函式名或模板名無法決定選哪一個，需顯式轉型；(e) bitfield
//         無法綁定非 const 參考。
//     為什麼會錯：「完美」二字讓人以為它能原封不動轉發任何東西。實際上它轉發的是
//         「已經推導成功的型別」——推導不出來的東西，根本進不了轉發這一關。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

// ─── 1. 觀察 lvalue / rvalue：印出 「value category」 ─────────────────────
//   利用 overload 區分。一個吃 lvalue ref，一個吃 rvalue ref。
void inspect(const std::string& s) { std::cout << "got lvalue: \"" << s << "\"\n"; }
void inspect(std::string&& s)      { std::cout << "got rvalue: \"" << s << "\"\n"; }

// 不用 forward 的 wrapper：完全失真，pack 內變數本身永遠是 lvalue。
template <typename T>
void wrapper_bad(T&& x) {
    inspect(x);                             // 這裡 x 是有名字的變數 → 永遠 lvalue!
}

// 正確的 wrapper：用 forward 還原
template <typename T>
void wrapper_good(T&& x) {
    inspect(std::forward<T>(x));
}

// ─── 2. 自製 make_unique (展示 variadic + forward) ───────────────────────
//   標準庫從 C++14 起已經有，這裡是練習。
template <typename T, typename... Args>
std::unique_ptr<T> make_unique_t(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// ─── 3. 自製 emplace_back 風格 helper ────────────────────────────────────
//   把任意參數塞進 std::list 的尾端。
template <typename T, typename... Args>
void emplace_back_t(std::list<T>& lst, Args&&... args) {
    lst.emplace_back(std::forward<Args>(args)...);
}

// ─── 4. Leetcode 146 ── LRU Cache (用 forward 建構節點) ──────────────────
//   題目：實作 LRU (Least Recently Used) cache，支援 get(key) 和 put(key,value)，
//        都要 O(1)。當容量滿，淘汰最久未使用的。
//
//   經典解法：std::list 存 (key,value) 配 std::unordered_map<key, list iterator>。
//        - get：把命中節點移到 list 前端
//        - put：插入或更新後同樣移到前端；超過容量就 pop_back
//
//   時間：O(1)
//   空間：O(capacity)
//
//   為什麼放這裡？
//     插入時用 forward 模式 (emplace_front + std::forward) 可以對任意可
//     構造的 K, V 型別運作，避免不必要 copy。
//
//   為了示範簡單，這裡只實作 int -> int，但 put_kv 支援任意可構造參數。
class LRUCache {
public:
    explicit LRUCache(int cap) : cap_(cap) {}

    int get(int key) {
        auto it = idx_.find(key);
        if (it == idx_.end()) return -1;
        list_.splice(list_.begin(), list_, it->second);   // 移到 front
        return it->second->second;
    }

    template <typename K, typename V>
    void put_kv(K&& key, V&& value) {
        auto it = idx_.find(key);
        if (it != idx_.end()) {
            it->second->second = std::forward<V>(value);
            list_.splice(list_.begin(), list_, it->second);
            return;
        }
        if ((int)list_.size() == cap_) {
            int old_key = list_.back().first;
            list_.pop_back();
            idx_.erase(old_key);
        }
        // emplace_front + forward：避免兩次 copy
        list_.emplace_front(std::forward<K>(key), std::forward<V>(value));
        idx_[list_.front().first] = list_.begin();
    }

private:
    int cap_;
    std::list<std::pair<int,int>> list_;
    std::unordered_map<int, std::list<std::pair<int,int>>::iterator> idx_;
};

// ─── 5. Leetcode 705 ── Design HashSet (用 forward 插入元素避免 copy) ────
//   難度: easy
//   題目：實作 HashSet：add(key) / remove(key) / contains(key)。
//
//   為什麼放在這裡？
//     用 emplace + std::forward 接受 key 的 lvalue 或 rvalue。傳 string
//     rvalue 進來時不會多一次 copy，比 const T& 的版本更高效。
//
//   時間：平均 O(1)；空間：O(n)。
template <typename T>
class MyHashSet {
public:
    template <typename U>
    void add(U&& key) {
        auto& bucket = bucket_of(key);
        for (const auto& x : bucket) if (x == key) return;
        bucket.emplace_back(std::forward<U>(key));
    }

    void remove(const T& key) {
        auto& bucket = bucket_of(key);
        bucket.remove(key);
    }

    bool contains(const T& key) const {
        const auto& bucket = bucket_of(key);
        for (const auto& x : bucket) if (x == key) return true;
        return false;
    }

private:
    static constexpr std::size_t kBuckets = 1024;
    mutable std::list<T> data_[kBuckets];

    std::list<T>& bucket_of(const T& k) {
        return data_[std::hash<T>{}(k) % kBuckets];
    }
    const std::list<T>& bucket_of(const T& k) const {
        return data_[std::hash<T>{}(k) % kBuckets];
    }
};

// ─── 6. 工作實用：delayed_invoke (把 callable 與引數綁起來日後執行) ──────
//   類似 std::bind / packaged_task 概念。用 forward 接住任何 callable 與引數。
template <typename F, typename... Args>
auto delayed_invoke(F&& f, Args&&... args) {
    // 用 lambda + by-value capture，把 args 安全包起來 (decay 後存值)
    return [f = std::forward<F>(f),
            tup = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        return std::apply(f, tup);
    };
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    std::string s = "hello";

    // (1) 觀察 lvalue / rvalue 在 wrapper 內外的差異
    std::cout << "--- direct call ---\n";
    inspect(s);                          // lvalue
    inspect(std::move(s));               // rvalue

    s = "world";  // 重置，因為 move 後 s 已 valid-but-unspecified
    std::cout << "--- wrapper_bad ---\n";
    wrapper_bad(s);                      // lvalue → lvalue OK
    wrapper_bad(std::move(s));           // rvalue → 但 wrapper_bad 內 x 仍是 lvalue → 印 lvalue!

    s = "world";
    std::cout << "--- wrapper_good ---\n";
    wrapper_good(s);                     // lvalue 保留
    wrapper_good(std::move(s));          // rvalue 保留

    // (2) make_unique_t
    struct Pt { int x; double y; Pt(int a, double b) : x(a), y(b) {} };
    auto p = make_unique_t<Pt>(3, 4.5);
    std::cout << "Pt(" << p->x << "," << p->y << ")\n";

    // (3) emplace_back_t
    std::list<std::pair<int, std::string>> lst;
    emplace_back_t(lst, 1, std::string("alpha"));
    emplace_back_t(lst, 2, "beta");                     // 接受任何可構造
    for (const auto& kv : lst) std::cout << kv.first << "->" << kv.second << ' ';
    std::cout << "\n";

    // (4) LRU Cache
    LRUCache c(2);
    c.put_kv(1, 1);
    c.put_kv(2, 2);
    std::cout << "get(1) = " << c.get(1) << " (expect 1)\n";
    c.put_kv(3, 3);                       // 淘汰 2
    std::cout << "get(2) = " << c.get(2) << " (expect -1)\n";
    c.put_kv(4, 4);                       // 淘汰 1
    std::cout << "get(1) = " << c.get(1) << " (expect -1)\n";
    std::cout << "get(3) = " << c.get(3) << " (expect 3)\n";
    std::cout << "get(4) = " << c.get(4) << " (expect 4)\n";

    // (5) Leetcode 705 HashSet
    MyHashSet<std::string> hs;
    std::string k1 = "alice";
    hs.add(k1);                                  // lvalue
    hs.add(std::string("bob"));                  // rvalue → emplace forward
    std::cout << "contains(alice) = " << hs.contains("alice") << "\n";
    std::cout << "contains(bob)   = " << hs.contains("bob")   << "\n";
    hs.remove("alice");
    std::cout << "after remove, contains(alice) = " << hs.contains("alice") << "\n";

    // (6) delayed_invoke
    auto job = delayed_invoke([](int x, int y) { return x + y; }, 3, 4);
    std::cout << "delayed_invoke() result = " << job() << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::forward 跟 std::move 差別在哪？
    //    A：std::move 無條件把任何東西轉成 rvalue（其實只是 static_cast 到 T&&）；
    //       std::forward<T>(x) 則「有條件」轉發：若 T 推導為 lvalue ref 就保持
    //       lvalue，否則才轉成 rvalue。Perfect forwarding 必須用 forward，否則
    //       會把 lvalue 也偷偷 move 掉。
    //
    //  Q2：什麼情況下 perfect forwarding 會「失敗」？
    //    A：常見幾種：(a) 把 forwarding ref 套到非 deduced context（例如顯式
    //       指定 T）就不再是 universal reference；(b) 傳遞 0、NULL、{}、bitfield、
    //       overloaded function name 等無法直接推導型別的東西；(c) 把 args 在多
    //       個地方 forward 兩次（會 use-after-move）。
    //
    //  Q3：T&& 為何有時是 rvalue ref，有時是 forwarding (universal) reference？
    //    A：判斷規則：當 T 是 function template 的「待推導」型別參數時，T&& 才是
    //       forwarding ref，會套用 reference collapsing（& + && = &）。若 T 已經
    //       固定（例如 vector<T> 的成員函式裡），T&& 就純粹是 rvalue ref。
    //
    return 0;
}

// ============================================================================
//  【經驗法則】
//    1. 看到「template<typename T> ... f(T&& x)」，T&& 是 forwarding reference，
//       搭配 std::forward<T>(x) 使用。
//    2. 不在 template 推導裡的 T&& (e.g. void g(int&&))，是普通 rvalue
//       reference，搭配 std::move 使用。
//    3. forwarding reference + forward 是「完美轉發」的標準工具組。
//    4. 寫 wrapper / factory / emplace 風格函式時，記得引數要 (T&& x) 而
//       非 (const T& x)，並用 std::forward<T>(x) 傳遞。
//
//  【下一篇】
//    16_sfinae.cpp ── SFINAE 是什麼？怎麼用 enable_if 做型別篩選？
// ============================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 15_perfect_forwarding.cpp -o 15_perfect_forwarding

// === 預期輸出 ===
// --- direct call ---
// got lvalue: "hello"
// got rvalue: "hello"
// --- wrapper_bad ---
// got lvalue: "world"
// got lvalue: "world"
// --- wrapper_good ---
// got lvalue: "world"
// got rvalue: "world"
// Pt(3,4.5)
// 1->alpha 2->beta
// get(1) = 1 (expect 1)
// get(2) = -1 (expect -1)
// get(1) = -1 (expect -1)
// get(3) = 3 (expect 3)
// get(4) = 4 (expect 4)
// contains(alice) = 1
// contains(bob)   = 1
// after remove, contains(alice) = 0
// delayed_invoke() result = 7
