// ============================================================
// 第 2.2 章 總結：右值參考 && — 基本語法與用途
// ============================================================
//
// 【主題資訊 Information】
//   語法    ：T&&        右值參考（只能綁定 rvalue）
//             const T&   常數左值參考（lvalue 與 rvalue 都能綁）
//             T&         左值參考（只能綁定 lvalue）
//   標準版本：右值參考 T&& / std::move / 移動建構子   皆為 C++11
//             生命週期延長（const T& 綁定臨時物件）    C++98 就有
//   標頭檔  ：<utility>（std::move）
//   複雜度  ：綁定本身零成本；移動 O(1)、深拷貝 O(N)
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 三種參考的分工，其實是三種「意圖」】
//       T&        「我要改你，而且你必須是個有名字的東西」
//       const T&  「我只是要讀你，誰都可以給我」   ← 最泛用
//       T&&       「你反正要死了，資源給我吧」     ← 移動語意的入口
//   注意 const T& 與 T&& 都能接住臨時物件，差別在於「能不能改它」：
//   const T& 不能改，所以只能複製；T&& 能改，所以可以把來源掏空。
//   移動建構子之所以是 T&& 而非 const T&，原因就在這裡。
//
// 【2. 生命週期延長：兩者都做得到】
//       const Verbose& r1 = make_object();   // 臨時物件活到 r1 離開作用域
//       Verbose&&      r2 = make_object();   // 同樣延長，而且 r2 可以修改
//   規則：把臨時物件綁定到「參考」上，它的壽命就延長到該參考的壽命。
//   但這個延長只發生一次、也只對「直接綁定」有效 ——
//   若中間經過函式回傳，延長規則不適用（見第 4 點）。
//
// 【3. 全章最重要的一條：有名字的右值參考是左值】
//       void outer(string&& s) {   // s 的「型別」是 string&&
//           inner(s);              // 但 s 這個「表達式」是 lvalue → 選 const T&
//           inner(std::move(s));   // 要選 T&& 必須再 move 一次
//       }
//   為什麼？因為 s 有名字，函式內可能用它很多次。
//   若它自動是右值，第一次呼叫就把它掏空，第二行就讀到空物件。
//   標準選擇「安全優先」：要放棄它，你必須明寫 std::move。
//   一句口訣：型別是 && 不代表表達式是右值。
//
// 【4. 絕不能回傳區域變數的右值參考】
//       string&& bad()  { string s = "x"; return std::move(s); }  // ❌ 懸空
//       string   good() { string s = "x"; return s; }             // ✅ 回傳值
//   bad() 回傳的參考指向已銷毀的區域變數。生命週期延長規則救不了它 ——
//   那條規則只適用於「臨時物件直接綁定到參考」，不適用於「函式回傳參考」。
//   使用該回傳值是未定義行為，其後果不固定，不可依賴任何一次觀察結果。
//   good() 則會觸發 RVO 或移動建構，既安全又有效率。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼重載決議會讓 rvalue 選中 T&&
//     傳入 rvalue 時，const T& 與 T&& 兩個版本都「可行」。
//     標準規定：對 rvalue 引數，T&& 是比 const T& 更好的匹配。
//     這條規則正是「複製 vs 移動」自動分流的基礎 ——
//     你不必在呼叫端做任何事，編譯器會依 value category 自動選對版本。
//
// (B) 為什麼 const T&& 幾乎沒有意義
//     T&& 的用途是掏空來源，但 const 讓你不能修改來源，兩者互相抵消。
//     實務後果：對 const 物件 std::move 會安靜退回複製，
//     這是「加了 move 卻沒變快」的常見原因。
//
// (C) T&& 與「轉發引用」長得一樣但完全不同
//     只有在「模板參數推導」的情境下，T&& 才是轉發引用（forwarding reference）：
//         template<class T> void f(T&& x);   // 轉發引用：可綁 lvalue 也可綁 rvalue
//         void g(std::string&& x);           // 純右值參考：只能綁 rvalue
//     差別在於 T 是否由本次呼叫推導而來。
//     轉發引用要搭配 std::forward（見第 2.7 章），純右值參考搭配 std::move。
//
// 【注意事項 Pay Attention】
//   1. 有名字的右值參考是 lvalue；要繼續傳遞其右值性必須再寫一次 std::move。
//   2. 絕不要回傳區域變數的右值參考 —— 那是懸空參考，使用它屬未定義行為。
//   3. const T& 與 T&& 都能延長臨時物件壽命，但只有 T&& 能修改它。
//   4. 對 const 物件 std::move 會退回複製。
//   5. 移動後的來源處於「有效但未指定」狀態，不可假設它一定變空。
//   6. 模板中的 T&& 是轉發引用，不是右值參考，兩者規則完全不同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】右值參考 &&
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. const T& 已經能接住臨時物件了，為什麼還需要 T&&？
//     答：因為 const T& 不能「修改」來源。移動的本質是「把資源搬走、
//         再把來源設成空」，這需要修改來源，const 做不到。
//         T&& 既能接住臨時物件、又能修改它，才使移動語意成為可能。
//     追問：那為什麼不用非 const 的 T&？→ T& 不能綁定 rvalue，
//         而臨時物件正是最該被移動的對象，所以必須有新的 T&&。
//
// 🔥 Q2. void f(std::string&& s) 裡，把 s 傳給另一個有重載的函式，會選哪個版本？
//     答：會選 const T& 版本。因為 s 有名字，它這個「表達式」是 lvalue，
//         即使型別是 string&&。要選中 T&& 版本必須寫 inner(std::move(s))。
//     追問：這樣設計的理由？→ 安全。s 在函式內可能被使用多次，
//         若自動當右值，第一次使用就被掏空，後面全部讀到空物件。
//
// 🔥 Q3. std::string&& f() { std::string s = "x"; return std::move(s); } 有什麼問題？
//     答：回傳的是指向區域變數 s 的參考，函式返回時 s 已銷毀 → 懸空參考，
//         呼叫端使用它是未定義行為。生命週期延長規則不適用於函式回傳的參考。
//         正解是回傳值：std::string f() { std::string s = "x"; return s; }
//         這樣會觸發 RVO 或移動建構，安全且高效。
//     追問：那在「回傳值」的版本裡要加 std::move 嗎？→ 不要。
//         加了反而阻止 NRVO，多做一次移動。回傳區域變數時什麼都別加。
//
// ⚠️ 陷阱. 模板裡的 T&& 也是右值參考嗎？
//     答：不是。template<class T> void f(T&& x) 中的 T&& 是「轉發引用」，
//         它可以綁定 lvalue（此時 T 推導為 U&）也可以綁定 rvalue（T 推導為 U）。
//         只有「型別已確定」的 T&&（例如 void g(std::string&&)）才是純右值參考。
//     為什麼會錯：只看語法長相 && 就判定是右值參考。
//         真正的判準是「T 是否由本次呼叫推導而來」——
//         會推導的是轉發引用（配 std::forward），不推導的是右值參考（配 std::move）。
// ═══════════════════════════════════════════════════════════════════════════
//
// 編譯：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
// ============================================================
// 【三種引用的綁定規則】
//   T&        只能綁定 lvalue
//   const T&  可綁定 lvalue 和 rvalue（萬能讀取）
//   T&&       只能綁定 rvalue
//
// 【右值參考的特性】
//   1. 綁定後延長臨時物件的生命週期（和 const T& 一樣）
//   2. 可以修改綁定的值（const T& 做不到！）
//   3. 右值參考變數本身是 lvalue！（有名字 → 是 lvalue）
//
// 【函數重載：const T& vs T&&】
//   傳入 lvalue → const T& 版本
//   傳入 rvalue → T&& 版本（更精確匹配）
//   這是實現「拷貝 vs 移動」語義的基礎
//
// 【重要陷阱：右值參考參數在函數內是 lvalue】
//   void func(string&& s) {
//       inner(s);            // const T& 版本！s 是 lvalue
//       inner(std::move(s)); // T&& 版本！需要 std::move 轉回 rvalue
//   }
//
// 【絕不能回傳局部變數的右值參考！】
//   string&& bad() { string s = "x"; return move(s); }  // ❌ 懸空參考！
//   string good() { string s = "x"; return s; }          // ✅ 回傳值 + RVO
// ============================================================

#include <iostream>
#include <string>
#include <cstring>
#include <utility>
using namespace std;

// ============================================================
// 【LeetCode 實戰範例】—— 本章刻意從缺，理由如下
// ============================================================
// 右值參考是「資源所有權」的語法機制，不是演算法。
// LeetCode 題目的參數與回傳值都由平台準備好，
// 解題過程不會出現「要不要接管這個物件的資源」這種設計問題。
// 因此本章不提供 LeetCode 範例，改以一個真實工程情境示範。
// ============================================================

// ------------------------------------------------------------
// 【日常實務範例】連線設定的 setter：以重載自動分流複製與移動
//   情境：設定物件要記錄主機名稱。呼叫端可能是
//         (a) 一個之後還要用的變數 → 必須複製
//         (b) 當場拼接出來的字串   → 應該直接接管，不必複製
//   為什麼用到本主題：
//     這是 const T& / T&& 重載在真實程式碼中最典型的用途。
//     呼叫端什麼都不用做，編譯器依引數的 value category 自動選版本 ——
//     這正是移動語意「零侵入」的價值所在。
// ------------------------------------------------------------
class ConnectionConfig {
    string m_host;
    int    m_copies = 0;
    int    m_moves  = 0;

public:
    // 左值版本：來源之後還要用 → 只能複製
    void setHost(const string& h) {
        m_host = h;
        ++m_copies;
    }

    // 右值版本：來源即將消亡 → 直接接管
    void setHost(string&& h) {
        m_host = std::move(h);   // h 有名字是 lvalue，必須再 move 一次
        ++m_moves;
    }

    void report() const {
        cout << "  最終 host = \"" << m_host << "\"\n";
        cout << "  期間共複製 " << m_copies << " 次、移動 " << m_moves << " 次\n";
    }
};

// ============================================================
// 函數重載：const T& vs T&&
// ============================================================
void process(const string& s) {
    cout << "  [const T& 版本] \"" << s << "\"\n";
}
void process(string&& s) {
    cout << "  [T&& 版本]      \"" << s << "\"\n";
}

// ============================================================
// 右值參考參數在函數內是 lvalue
// ============================================================
void inner(const string& /*s*/) { cout << "    inner: const T&\n"; }  // 只看選中哪個重載
void inner(string&& /*s*/)      { cout << "    inner: T&&\n"; }       // 同上

void outer(string&& s) {
    cout << "  直接傳 s（s 是 lvalue！）：\n";
    inner(s);               // const T& 版本！

    cout << "  傳 std::move(s)：\n";
    inner(std::move(s));    // T&& 版本！
}

// ============================================================
// Buffer 類別：展示移動 vs 複製的效能差異
// ============================================================
class Buffer {
    char* data_;
    size_t size_;
public:
    Buffer(const char* str) : size_(strlen(str)) {
        data_ = new char[size_ + 1];
        strcpy(data_, str);
        cout << "    [建構] \"" << data_ << "\"\n";
    }
    ~Buffer() {
        if (data_) cout << "    [解構] \"" << data_ << "\"\n";
        else       cout << "    [解構] (空)\n";
        delete[] data_;
    }
    Buffer(const Buffer& o) : size_(o.size_) {
        data_ = new char[size_ + 1];
        strcpy(data_, o.data_);
        cout << "    [複製建構💰] \"" << data_ << "\"\n";
    }
    Buffer(Buffer&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
        cout << "    [移動建構⚡] \"" << data_ << "\"\n";
    }
    const char* c_str() const { return data_ ? data_ : "(null)"; }
};

void store(const Buffer& buf) {
    cout << "  store(const Buffer&)：\n";
    Buffer local(buf);  // 複製建構
}
void store(Buffer&& buf) {
    cout << "  store(Buffer&&)：\n";
    Buffer local(std::move(buf));  // 移動建構
}

// ============================================================
// 生命週期延長
// ============================================================
class Verbose {
    string name_;
public:
    Verbose(const string& n) : name_(n) { cout << "    [建構] " << name_ << "\n"; }
    ~Verbose() { cout << "    [解構] " << name_ << "\n"; }
    const string& name() const { return name_; }
};
Verbose make_object() { return Verbose("臨時物件"); }

int main() {
    // ============================================================
    // 1. 綁定規則
    // ============================================================
    cout << "===== 1. 綁定規則 =====\n";
    int a = 10;
    [[maybe_unused]] int& lref = a;          // ✅ T& → lvalue
    // int& bad = 42;       // ❌ T& → rvalue（不行）

    [[maybe_unused]] const int& clref = 42;  // ✅ const T& → rvalue（特殊規則）

    int&& rref = 42;        // ✅ T&& → rvalue
    // int&& bad = a;       // ❌ T&& → lvalue（不行）

    rref = 100;             // ✅ 右值參考可以修改！
    cout << "  rref = " << rref << "\n";
    cout << "  &rref = " << &rref << " ← 可取址，所以 rref 本身是 lvalue\n\n";

    // ============================================================
    // 2. 函數重載
    // ============================================================
    cout << "===== 2. 函數重載 =====\n";
    string name = "Alice";
    process(name);                   // const T& — lvalue
    process(string("Bob"));          // T&& — rvalue（臨時物件）
    process("Charlie");              // T&& — 隱含轉型產生臨時物件
    process(std::move(name));        // T&& — std::move 轉為 rvalue
    cout << "  name after move: \"" << name << "\"\n\n";

    // ============================================================
    // 3. 右值參考參數在函數內是 lvalue（重要陷阱！）
    // ============================================================
    cout << "===== 3. T&& 參數在函數內是 lvalue =====\n";
    outer(string("Hello"));
    cout << "\n";

    // ============================================================
    // 4. 生命週期延長
    // ============================================================
    cout << "===== 4. 生命週期延長 =====\n";
    {
        cout << "  沒綁定 → 臨時物件立即銷毀：\n";
        make_object();
        cout << "  （已銷毀）\n\n";

        cout << "  const T& 綁定 → 延長生命週期：\n";
        const Verbose& cref = make_object();
        cout << "  cref 仍有效：" << cref.name() << "\n\n";

        cout << "  T&& 綁定 → 也延長生命週期（且可修改）：\n";
        Verbose&& rrv = make_object();
        cout << "  rrv 仍有效：" << rrv.name() << "\n";
    }
    cout << "\n";

    // ============================================================
    // 5. 移動 vs 複製的效能差異
    // ============================================================
    cout << "===== 5. 移動 vs 複製 =====\n";
    {
        Buffer original("Important Data Here");

        cout << "\n  傳入 lvalue（複製）：\n";
        store(original);

        cout << "\n  傳入 rvalue（移動）：\n";
        store(Buffer("Temporary Data"));

        cout << "\n  用 std::move（移動）：\n";
        store(std::move(original));
        cout << "  original after move: " << original.c_str() << "\n";
    }

    cout << "\n=== 重點整理 ===\n";
    cout << "  T& → 只綁 lvalue    const T& → 萬能    T&& → 只綁 rvalue\n";
    cout << "  T&& 變數本身是 lvalue（有名字）\n";
    cout << "  函數內要轉發 T&& 參數，必須 std::move\n";
    cout << "  T&& 和 const T& 都能延長臨時物件生命週期\n";
    cout << "  絕不能回傳局部變數的 T&&（懸空參考！）\n";

    // ========================================================================
    // 日常實務：設定物件的兩種 setter（複製 vs 移動）
    // ========================================================================
    cout << "\n=== 日常實務：連線設定的 setter 重載 ===\n";
    {
        ConnectionConfig cfg;

        // (1) 傳既有變數 → 選 const T& → 複製（呼叫端之後還要用）
        string sharedHost = "db-primary.internal";
        cfg.setHost(sharedHost);
        cout << "  傳左值後 sharedHost 仍可用: \"" << sharedHost << "\"\n";

        // (2) 傳當場組出來的字串 → 選 T&& → 移動（沒有其他使用者）
        cfg.setHost(string("db-replica-") + to_string(3) + ".internal");

        // (3) 明確交出所有權
        string oldName = "legacy-cluster";
        cfg.setHost(std::move(oldName));

        cfg.report();
        cout << "  重點：同一個 setName 呼叫，走哪個重載完全由引數的值類別決定。\n";
        cout << "        呼叫端不必做任何特別的事，編譯器自動分流複製與移動。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "summary.cpp" -o rvref_summary
//  ⚠️ 注意：下列輸出中的記憶體位址「每次執行都不同」（受堆積/堆疊配置與 ASLR 影響），
//     此處僅為某一次實際執行的樣本，不具重現性；其餘文字內容則是穩定可重現的。

// === 預期輸出 ===
// ===== 1. 綁定規則 =====
//   rref = 100
//   &rref = 0x7ffdf6759cbc ← 可取址，所以 rref 本身是 lvalue
//
// ===== 2. 函數重載 =====
//   [const T& 版本] "Alice"
//   [T&& 版本]      "Bob"
//   [T&& 版本]      "Charlie"
//   [T&& 版本]      "Alice"
//   name after move: "Alice"
//
// ===== 3. T&& 參數在函數內是 lvalue =====
//   直接傳 s（s 是 lvalue！）：
//     inner: const T&
//   傳 std::move(s)：
//     inner: T&&
//
// ===== 4. 生命週期延長 =====
//   沒綁定 → 臨時物件立即銷毀：
//     [建構] 臨時物件
//     [解構] 臨時物件
//   （已銷毀）
//
//   const T& 綁定 → 延長生命週期：
//     [建構] 臨時物件
//   cref 仍有效：臨時物件
//
//   T&& 綁定 → 也延長生命週期（且可修改）：
//     [建構] 臨時物件
//   rrv 仍有效：臨時物件
//     [解構] 臨時物件
//     [解構] 臨時物件
//
// ===== 5. 移動 vs 複製 =====
//     [建構] "Important Data Here"
//
//   傳入 lvalue（複製）：
//   store(const Buffer&)：
//     [複製建構💰] "Important Data Here"
//     [解構] "Important Data Here"
//
//   傳入 rvalue（移動）：
//     [建構] "Temporary Data"
//   store(Buffer&&)：
//     [移動建構⚡] "Temporary Data"
//     [解構] "Temporary Data"
//     [解構] (空)
//
//   用 std::move（移動）：
//   store(Buffer&&)：
//     [移動建構⚡] "Important Data Here"
//     [解構] "Important Data Here"
//   original after move: (null)
//     [解構] (空)
//
// === 重點整理 ===
//   T& → 只綁 lvalue    const T& → 萬能    T&& → 只綁 rvalue
//   T&& 變數本身是 lvalue（有名字）
//   函數內要轉發 T&& 參數，必須 std::move
//   T&& 和 const T& 都能延長臨時物件生命週期
//   絕不能回傳局部變數的 T&&（懸空參考！）
//
// === 日常實務：連線設定的 setter 重載 ===
//   傳左值後 sharedHost 仍可用: "db-primary.internal"
//   最終 host = "legacy-cluster"
//   期間共複製 1 次、移動 2 次
//   重點：同一個 setName 呼叫，走哪個重載完全由引數的值類別決定。
//         呼叫端不必做任何特別的事，編譯器自動分流複製與移動。
