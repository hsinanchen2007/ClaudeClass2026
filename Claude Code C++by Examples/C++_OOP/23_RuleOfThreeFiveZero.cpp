/*=============================================================================
 * 檔名：23_RuleOfThreeFiveZero.cpp
 * 主題：Rule of Three / Five / Zero - 何時要自己寫複製/移動/解構？
 * 適合：學完前面所有特殊成員 (建構、複製、賦值、解構、移動) 後想做總整理的人
 *
 * 【課題介紹】
 *   一個 class 有「五個特殊成員函式」，編譯器會在某些條件下自動產生它們：
 *
 *     1. 解構子           ~T()
 *     2. 複製建構子        T(const T&)
 *     3. 複製賦值          operator=(const T&)
 *     4. 移動建構子        T(T&&)
 *     5. 移動賦值          operator=(T&&)
 *
 *   問題：什麼時候該自己寫？什麼時候交給編譯器就好？
 *   業界濃縮成三條經驗法則：
 *
 *   ─────────────────────── Rule of Three (C++98 起就有) ───────────────────────
 *   如果你需要自己寫上面 (1)(2)(3) 的「任何一個」，
 *   那你大概也要寫「另外兩個」。
 *
 *   原因：通常需要自寫這三者的場景都是「物件持有 raw pointer 或檔案/socket 等資源」，
 *   三者中只寫其中一個會留下不一致 — 例如有 dtor 但用預設 copy ctor → 雙重 delete。
 *
 *   ─────────────────────── Rule of Five (C++11 起) ───────────────────────────
 *   有了 move semantics 後，把 move ctor / move 賦值 也加進來，
 *   要寫就寫五個 (或正確配套至少其中一些)。
 *
 *   特別注意：「自寫了 dtor / copy ctor / copy 賦值 任何一個」會抑制
 *   編譯器自動生成 move 系列 → 你的物件會被迫走 copy，效能損失。
 *   解法：要嘛全寫，要嘛用 = default 顯式要求生成。
 *
 *   ─────────────────────── Rule of Zero (推薦) ──────────────────────────────
 *   最理想：「五個都不要自己寫，交給編譯器」。
 *   做法：物件內所有成員都用「會自己管理資源」的 RAII 型別 — 例如
 *   std::string, std::vector, std::unique_ptr, std::shared_ptr。
 *   這樣編譯器產生的預設五個函式就已經正確 → 你完全不用煩惱。
 *
 *   現代 C++ 的最佳實務：
 *       「能符合 Rule of Zero 就盡量符合；
 *        非要自己管資源時，就寫滿 Rule of Five (或寫到 Rule of Three 也行)。」
 *
 * 【= default 與 = delete】
 *   - = default：「我要一個預設的」，告訴編譯器「請保留生成」。
 *   - = delete： 「我禁止使用這個」，呼叫該函式會編譯錯誤。
 *
 *   常見組合：
 *     class NonCopyable {
 *     public:
 *         NonCopyable() = default;
 *         NonCopyable(const NonCopyable&) = delete;          // 禁止複製
 *         NonCopyable& operator=(const NonCopyable&) = delete;
 *         NonCopyable(NonCopyable&&) = default;              // 允許移動
 *         NonCopyable& operator=(NonCopyable&&) = default;
 *     };
 *
 * 【對應 Leetcode】1480. Running Sum of 1d Array
 *   為什麼選這題：把累加器寫成 Rule of Zero 形式 (成員都是 RAII 型別)，示範
 *   它能被自然地複製、賦值、移動、銷毀，而我們什麼都沒寫。這就是 Rule of Zero 的價值。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/rule_of_three
 *   https://cplusplus.com/doc/tutorial/classes2/
 *=============================================================================*/

/*
補充筆記：RuleOfThreeFiveZero
  - Rule of Zero 是首選：讓標準庫成員自己管理資源。
  - 只要手寫 destructor/copy/move 其中之一，就要檢查其他特殊成員是否也需要定義。
  - 資源類別若可複製，必須定義清楚是 deep copy、shared ownership 還是禁止 copy。
  - Rule of Three：若需要手寫 destructor、copy constructor、copy assignment 其中之一，通常三者都要檢查，因為類別多半管理資源。
  - Rule of Five 是 C++11 加入 move constructor 和 move assignment 後的延伸；資源型別若能搬移，應定義清楚 move 語意。
  - Rule of Zero 是現代 C++ 最推薦方向：把資源交給標準 RAII 成員，讓編譯器自動產生特殊成員函式。
  - 手寫 destructor 可能抑制或改變編譯器產生 move operation 的條件；因此只要寫了一個特殊成員，就要檢查其他成員是否仍符合預期。
  - = default 表示明確要求編譯器產生預設版本；= delete 表示明確禁止某操作，例如禁止複製獨占資源。
  - copy 語意和 move 語意要和類別概念一致：值型別通常可複製，獨占 owner 通常不可複製但可 move。
  - 測試特殊成員函式時，要涵蓋複製、賦值、自我賦值、move 後狀態、容器中 reallocation 等情境。
  - 看到裸指標成員不要立刻假設它擁有資源；先判斷它是 owning pointer 還是 non-owning observer，特殊成員函式設計完全不同。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Rule of Three / Five / Zero
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Rule of Three / Five / Zero 各是什麼？
//     答：**Rule of Three** —— 若你需要自訂 destructor、copy constructor、
//         copy assignment 其中一個，通常三個都要自訂（代表你在管理裸資源）。
//         **Rule of Five** —— C++11 後再加上 move constructor、move assignment。
//         **Rule of Zero** —— 最佳實踐：把資源封裝進專責型別（smart pointer、
//         container），自己的 class 一個特殊成員都不寫，全部讓編譯器產生。
//         本檔 MyStringV5 是前者（自管 char*），Person 是後者（成員都是 RAII 型別）。
//     追問：`= default` 與 `= delete` 的意義？（前者要求編譯器保留生成、後者禁止該操作，
//           是現代 C++ 表達意圖最直接的工具，例如本檔 FileGuard 禁 copy 允許 move）
//
// 🔥 Q2. 深拷貝與淺拷貝的差別？
//     答：編譯器產生的 copy 是「成員逐一複製」，對裸指標成員而言只複製指標值
//         → 兩個物件指向同一塊記憶體 → double free / dangling。深拷貝要自己配置
//         新記憶體並複製內容（本檔 MyStringV5 的 copy constructor）。這正是
//         Rule of Three 存在的理由；現代做法是改持有 string / vector / unique_ptr。
//     追問：`std::string` 是深拷貝嗎？（是；C++11 起標準已禁止 COW 實作）
//
// Q3. 多型 base class 的五個特殊成員該怎麼寫？
//     答：多型 base 幾乎一定要 `virtual ~Base()`，而這個「使用者宣告的 destructor」
//         會抑制隱式 move（見下方陷阱），所以慣例是把五個特殊成員「全部顯式
//         `= default`」，至少把兩個 move 補回來。另外 base 的 copy 通常應設成
//         `protected` 或 `= delete`，避免呼叫端不小心做出 object slicing。
//
// ⚠️ 陷阱. 只是寫了 `virtual ~Base() = default;`，應該什麼都沒改變吧？
//     答：改變很大。只要「使用者宣告」了 destructor —— 即使是 `= default` ——
//         就會抑制隱式 move constructor 與 move assignment 的產生。copy 雖然還是
//         會生成，但標準已把「有使用者宣告 destructor 時仍隱式生成 copy」標記為
//         deprecated。結果是：你的類別靜默退化成 copy，效能全失，而且不會有任何警告。
//     為什麼會錯：多數人腦中「`= default` 就等於沒寫」。實際上編譯器看的是
//         「有沒有被使用者宣告（user-declared）」，不是「有沒有寫函式本體」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <utility>

// -----------------------------------------------------------------------------
// 範例 1：Rule of Five — 自管 char* 必須寫完整五個
// -----------------------------------------------------------------------------
class MyStringV5 {
private:
    char*  data_ = nullptr;
    size_t size_ = 0;
public:
    // (0) 一般建構子
    MyStringV5(const char* s = "") : size_(std::strlen(s)) {
        data_ = new char[size_ + 1];
        std::strcpy(data_, s);
    }

    // (1) 解構子：釋放資源
    ~MyStringV5() { delete[] data_; }

    // (2) 複製建構：深拷貝
    MyStringV5(const MyStringV5& other) : data_(new char[other.size_ + 1]), size_(other.size_) {
        std::strcpy(data_, other.data_);
    }

    // (3) 複製賦值：自我賦值防護 + 釋放舊 + 配置新 + 複製
    MyStringV5& operator=(const MyStringV5& other) {
        if (this == &other) return *this;
        delete[] data_;
        size_ = other.size_;
        data_ = new char[size_ + 1];
        std::strcpy(data_, other.data_);
        return *this;
    }

    // (4) 移動建構：搬指標 + 把對方掏空，noexcept
    MyStringV5(MyStringV5&& other) noexcept : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    // (5) 移動賦值
    MyStringV5& operator=(MyStringV5&& other) noexcept {
        if (this == &other) return *this;
        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
        return *this;
    }

    const char* c_str() const { return data_ ? data_ : ""; }
};

// -----------------------------------------------------------------------------
// 範例 2：Rule of Zero — 全部用 RAII 成員，自己什麼都不寫
// -----------------------------------------------------------------------------
class Person {
private:
    std::string         name_;        // 自帶 RAII，不需手動管
    int                 age_;
    std::vector<int>    scores_;      // 同上

public:
    Person(std::string name, int age, std::vector<int> scores)
        : name_(std::move(name)), age_(age), scores_(std::move(scores)) {}

    void show() const {
        std::cout << name_ << " (" << age_ << ") scores: ";
        for (int s : scores_) std::cout << s << " ";
        std::cout << "\n";
    }
    // 沒寫 dtor / copy / move — 全部交給編譯器自動產生，且全部正確！
};

// -----------------------------------------------------------------------------
// 範例 3：禁止複製、允許移動 — 常用於 RAII handle
// -----------------------------------------------------------------------------
class FileGuard {
public:
    FileGuard()                                = default;
    ~FileGuard()                               = default;
    FileGuard(const FileGuard&)                = delete;     // 禁止複製
    FileGuard& operator=(const FileGuard&)     = delete;
    FileGuard(FileGuard&&) noexcept            = default;    // 允許移動
    FileGuard& operator=(FileGuard&&) noexcept = default;
};

int main() {
    std::cout << "===== Rule of Five 自管資源 =====" << std::endl;
    MyStringV5 a("hello");
    MyStringV5 b = a;                  // copy ctor
    MyStringV5 c = std::move(a);       // move ctor
    std::cout << "b = " << b.c_str() << "\n";
    std::cout << "c = " << c.c_str() << "\n";
    b = c;                              // copy =
    std::cout << "b = " << b.c_str() << "\n";

    std::cout << "===== Rule of Zero 全部交給編譯器 =====" << std::endl;
    Person p("Alice", 30, {90, 85, 95});
    Person p2 = p;          // 預設 copy 已正確深拷貝 (因為 string/vector 自帶深拷貝)
    p2.show();

    Person p3 = std::move(p);   // 預設 move 直接 O(1)，因為 string/vector 都支援 move
    p3.show();

    std::cout << "===== 禁止複製、允許移動 =====" << std::endl;
    FileGuard f1;
    // FileGuard f2 = f1;          // ← 編譯錯誤：禁止複製
    FileGuard f3 = std::move(f1);   // OK：允許移動
    (void)f3;

    std::cout << "===== Leetcode 1480 - Rule of Zero 累加器 =====" << std::endl;
    // 這個累加器只用 std::vector + int 當成員，沒寫 ~ / copy / move 任何一個 →
    // 編譯器產生的五個函式全都是「自動正確」的版本：
    //   copy / move 都會深拷貝 / 完整搬移 vector，銷毀時也不會洩漏。
    // 這就是 Rule of Zero 的最佳示範。
    class RunningSum {
    public:
        std::vector<int> history;
        int total = 0;
        std::vector<int> compute(const std::vector<int>& nums) {
            for (int x : nums) {
                total += x;
                history.push_back(total);
            }
            return history;
        }
    };
    RunningSum acc;
    auto out = acc.compute({1, 2, 3, 4});       // {1, 3, 6, 10}

    RunningSum acc2 = acc;                       // 預設 copy 直接深拷貝
    RunningSum acc3 = std::move(acc);            // 預設 move 直接搬移 vector
    std::cout << "out: ";
    for (int x : out) std::cout << x << " ";
    std::cout << "\nacc2.total = " << acc2.total
              << ", acc3.total = " << acc3.total << "\n";

    std::cout << "===== Leetcode 1396 Underground System - Rule of Zero =====" << std::endl;
    // 即便類別內部有兩個 unordered_map，依然只需要寫成員：copy/move/destruct 都自動正確。
    // 比 Min Stack 更能展示 Rule of Zero 的好處 — 多個 RAII 容器並存。
    class UndergroundSystem {
    public:
        std::vector<std::pair<int, std::pair<std::string, int>>> checkIns;
        std::vector<std::pair<std::string, std::pair<long, int>>> stats;

        // 沒寫 dtor / copy / move / assign，編譯器全部產生正確版本
    };
    UndergroundSystem u1;
    u1.checkIns.push_back({45, {"Leyton", 3}});
    UndergroundSystem u2 = u1;              // 深拷貝
    UndergroundSystem u3 = std::move(u1);   // O(1) 搬移
    std::cout << "u2.checkIns 大小 = " << u2.checkIns.size()
              << ", u3.checkIns 大小 = " << u3.checkIns.size()
              << ", u1 (已搬走) = " << u1.checkIns.size() << std::endl;

    std::cout << "===== 日常實用：NonCopyable 基類 =====" << std::endl;
    // 工作上常需要「禁止複製」的物件 (mutex、unique handle、connection)，
    // 把這組規則抽成一個基類繼承使用。
    class NonCopyable {
    public:
        NonCopyable() = default;
        NonCopyable(const NonCopyable&)            = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
        NonCopyable(NonCopyable&&)                 = default;
        NonCopyable& operator=(NonCopyable&&)      = default;
    };

    class DBSession : public NonCopyable {
    public:
        std::string sessionId;
        explicit DBSession(std::string id) : sessionId(std::move(id)) {}
    };

    DBSession s1("abc-123");
    // DBSession s2 = s1;             // 編譯錯誤：禁止複製
    DBSession s2 = std::move(s1);     // OK：允許移動
    std::cout << "s2.sessionId = " << s2.sessionId << std::endl;
    return 0;
}

/* 預期輸出：
 * ===== Rule of Five 自管資源 =====
 * b = hello
 * c = hello
 * b = hello
 * ===== Rule of Zero 全部交給編譯器 =====
 * Alice (30) scores: 90 85 95
 * Alice (30) scores: 90 85 95
 * ===== 禁止複製、允許移動 =====
 * ===== Leetcode 1480 - Rule of Zero 累加器 =====
 * out: 1 3 6 10
 * acc2.total = 10, acc3.total = 10
 * ===== Leetcode 1396 Underground System - Rule of Zero =====
 * u2.checkIns 大小 = 1, u3.checkIns 大小 = 1, u1 (已搬走) = 0
 * ===== 日常實用：NonCopyable 基類 =====
 * s2.sessionId = abc-123
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 五個特殊成員：dtor / copy ctor / copy = / move ctor / move =。
 *   2. Rule of Three：要寫 (1)(2)(3) 中的一個，通常三個都得寫。
 *   3. Rule of Five：加上 move 系列；自寫 dtor / copy 會抑制 move 自動生成。
 *   4. Rule of Zero：用 RAII 成員 (string, vector, smart pointer)，全部不用自己寫。
 *   5. = default / = delete 是現代 C++ 表達意圖最直接的工具。
 *
 * 【下一篇預告】
 *   24_ClassTemplate.cpp
 *   類別模板 (Class Template) — 怎麼寫一個「裝任何型別」的容器？
 *=============================================================================*/

// 編譯: g++ -std=c++20 -Wall -Wextra 23_RuleOfThreeFiveZero.cpp -o 23_RuleOfThreeFiveZero

// === 預期輸出 ===
// ===== Rule of Five 自管資源 =====
// b = hello
// c = hello
// b = hello
// ===== Rule of Zero 全部交給編譯器 =====
// Alice (30) scores: 90 85 95
// Alice (30) scores: 90 85 95
// ===== 禁止複製、允許移動 =====
// ===== Leetcode 1480 - Rule of Zero 累加器 =====
// out: 1 3 6 10
// acc2.total = 10, acc3.total = 10
// ===== Leetcode 1396 Underground System - Rule of Zero =====
// u2.checkIns 大小 = 1, u3.checkIns 大小 = 1, u1 (已搬走) = 0
// ===== 日常實用：NonCopyable 基類 =====
// s2.sessionId = abc-123
