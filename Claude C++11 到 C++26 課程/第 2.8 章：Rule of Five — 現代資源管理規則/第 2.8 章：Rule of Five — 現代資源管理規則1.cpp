// =============================================================================
//  第 2.8 章：Rule of Five 1  —  五個特殊成員函式的完整實作
// =============================================================================
//
// 【主題資訊 Information】
//   類別若「直接持有裸資源」（new 出來的記憶體、FILE*、socket fd…），
//   通常必須自訂以下五個特殊成員函式（special member functions）：
//
//     1. ~T()                              解構子        釋放資源
//     2. T(const T&)                       複製建構子    深拷貝
//     3. T& operator=(const T&)            複製賦值      釋放舊的 + 深拷貝
//     4. T(T&&) noexcept                   移動建構子    偷指標 + 把來源歸零
//     5. T& operator=(T&&) noexcept        移動賦值      釋放舊的 + 偷 + 歸零
//
//   標準版本：解構子與複製兩者是 C++98（Rule of Three）；移動建構、移動賦值
//             與 noexcept 是 **C++11** 新增，五者合稱 Rule of Five。
//   標頭檔：  <utility>（std::move）、<cstring>（strlen/strcpy）
//   複雜度：  複製 O(n)（n = 資源大小）；移動 O(1)（與資料量無關）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「有解構子就要有其他四個」】
//   編譯器隱式產生的複製建構子做的是 memberwise copy（逐成員複製）。
//   對 char* 成員而言，逐成員複製 = 只複製「指標這個數值」，也就是淺拷貝
//   （shallow copy）：兩個物件的 data_ 指向同一塊 heap。
//   接著兩個物件各自解構、各自 delete[] 同一個位址 → double free。
//
//   所以判準不是「我想不想寫」，而是：
//     只要解構子做了非平凡的釋放動作，逐成員複製的語意就一定是錯的。
//   反過來說，若解構子什麼都不必做（資源由成員自己管），那五個都不用寫
//   —— 這正是【概念補充 A】的 Rule of Zero。
//
// 【2. 複製與移動的語意差異：深拷貝 vs 轉移所有權】
//   複製建構子：來源必須維持完全不變 → 只能另外配置一塊、把內容抄過去，O(n)。
//   移動建構子：來源是「即將消失」的物件 → 可以直接把它的指標搶過來，
//               再把來源改成可安全解構的狀態（data_ = nullptr）。
//   關鍵在於 **把來源歸零不是禮貌，是義務**：若不歸零，來源解構時會 delete
//   一塊已被新物件接管的記憶體，同樣是 double free。
//
//   被移動後的物件（moved-from object）處於「valid but unspecified」狀態：
//   標準只保證你可以安全地解構它、或重新賦值給它，不保證裡面剩下什麼。
//   本檔的 String 是我們自己實作的，所以我們知道它會變成空字串——但對標準庫
//   型別（std::string、std::vector）**不可以**假設 moved-from 一定是空的。
//
// 【3. 複製賦值的三個必要動作】
//     String& operator=(const String& other) {
//         if (this != &other) {   // (a) 自我賦值防護
//             delete[] data_;     // (b) 釋放自己原本持有的資源
//             ...深拷貝...        // (c) 取得新資源
//         }
//         return *this;           // 回傳 *this 以支援 a = b = c
//     }
//   (a) 少了自我賦值防護，a = a 會先 delete[] 自己的緩衝區，再從已釋放的
//       記憶體 strcpy → use-after-free（未定義行為）。
//   (b) 少了釋放，舊緩衝區再也沒有人指向它 → memory leak。
//   更穩健的替代寫法是 copy-and-swap（見【概念補充 D】），它把五個縮成三個。
//
// 【4. 為什麼移動函式一定要標 noexcept】
//   std::vector 擴容時要把舊緩衝區的元素搬到新緩衝區。若搬到一半拋例外，
//   舊緩衝區已被破壞、新的又不完整，vector 無法回滾 → 給不出強例外保證。
//   因此 vector 內部使用 std::move_if_noexcept：
//     * 元素的移動建構子是 noexcept → 用移動（快）
//     * 否則且該型別可複製          → **靜默退回複製**（慢，且你不會收到任何警告）
//   換句話說，忘記寫 noexcept 的懲罰不是編譯錯誤，而是效能默默掉回 O(n)。
//   檢查方式：static_assert(std::is_nothrow_move_constructible<T>::value, "");
//
// 【5. 成員初始化順序：只看宣告順序】
//   成員的初始化順序 **完全由類別中的宣告順序決定**，與初始化列表的書寫順序無關。
//   本檔刻意把 size_ 宣告在 data_ 之前，因為 data_ 的初始化式要用到長度。
//   若把 data_ 宣告在前而寫成：
//       char* data_; size_t size_;
//       String(const char* s) : size_(strlen(s)), data_(new char[size_ + 1]) {}
//   則 data_ 會 **先** 被初始化，此時 size_ 仍是未初始化的垃圾值 →
//   配置出錯誤大小的緩衝區 → heap-buffer-overflow。
//   GCC 的 -Wreorder（含在 -Wall 內）會在兩者順序不一致時警告，請不要關掉它。
//
// 【概念補充 Concept Deep Dive】
//
// (A) Rule of Three → Rule of Five → Rule of Zero
//     * Rule of Three（C++98）：解構子、複製建構、複製賦值，自訂其一就三個都要。
//     * Rule of Five（C++11）：再加上移動建構、移動賦值，共五個。
//     * **Rule of Zero（現代預設）**：一個都不要自訂。把裸資源包進 unique_ptr /
//       shared_ptr / vector / string，讓每個成員自己負責自己的生命週期，
//       編譯器隱式產生的五個版本就自動是正確的。
//     為什麼 Rule of Zero 才是預設？因為手寫的五個函式是「五個各自可能寫錯的地方」
//     （忘了自我賦值防護、忘了歸零來源、忘了 noexcept、複製賦值漏了釋放…），
//     而且這些錯誤多半不會在編譯期被抓到。Rule of Five 只在「你正在寫的就是那個
//     RAII 包裝器」時才用——那種類別在整個專案裡通常只有個位數個。
//
// (B) 編譯器什麼時候 **抑制** 隱式產生（最容易踩的規則）
//     * 宣告了解構子、複製建構子或複製賦值中的 **任何一個**
//         → 移動建構子與移動賦值 **不會** 被隱式產生。
//     * 宣告了移動建構子或移動賦值中的任何一個
//         → 複製建構子與複製賦值被隱式 delete。
//     第一條的可怕之處在於它 **不會報錯**：類別仍然可以用 T x = std::move(y);
//     編譯，只是多載解析改挑「複製建構子」（const T& 能繫結到右值）。
//     於是你以為在移動，實際上一直在複製，效能默默退化。
//     本機實測（GCC 15.2，libstdc++）：
//       struct C { std::vector<int> d; ~C(){} };
//       is_move_constructible<C>          → true   ← 看起來「可以移動」
//       is_nothrow_move_constructible<C>  → false  ← 其實走的是複製建構子
//     判斷「有沒有真的移動」要看 is_nothrow_move_constructible，
//     **不能**看 is_move_constructible。
//
// (C) = default 與 = delete
//     * `T(T&&) noexcept = default;` 明寫要編譯器產生逐成員移動；它比手寫空實作好，
//       因為能保留 trivial／constexpr／noexcept 等性質。當你因為【B】的規則被
//       抑制掉移動時，用 = default 把它補回來。
//     * `T(const T&) = delete;` 明確禁止複製，讓類別變成 move-only（unique_ptr
//       正是如此）。比 C++98「宣告成 private 但不定義」好：錯誤在編譯期就報出來。
//     * 注意：`~T() = default;` 也算 user-declared 解構子，同樣會抑制隱式移動。
//       寫了它就得把兩個移動函式也 = default 補回來。
//
// (D) copy-and-swap：用三個函式達成五個的效果
//       String(const String& o) : size_(o.size_), data_(new char[o.size_+1]) { ... }
//       String(String&& o) noexcept : String() { swap(*this, o); }
//       String& operator=(String o) noexcept { swap(*this, o); return *this; } // 傳值
//     賦值運算子 **以傳值接參數**：呼叫端傳左值就走複製建構、傳右值就走移動建構，
//     一個函式同時服務複製賦值與移動賦值。自我賦值天然安全（參數是獨立副本），
//     強例外保證也天然成立（配置在進函式前就完成了）。
//     代價：賦值給「容量已足夠」的物件時會多一次配置，比手寫版稍慢。
//
// (E) 記憶體佈局
//     本檔 String 的物件本體只有 { size_t size_; char* data_; }，本機實測
//     sizeof(String) = 16 bytes（x86-64）。字元內容全在 heap。
//     移動 = 複製這 16 bytes + 把來源指標歸零 → O(1)；
//     複製則要 new + strcpy n bytes → O(n)。
//     std::string 另外做了 SSO（見第 2.9 章），短字串根本不碰 heap。
//
// 【注意事項 Pay Attention】
//   1. 移動函式必須標 noexcept，否則 vector 擴容會靜默退回複製。
//   2. 移動後必須把來源改成可安全解構的狀態（data_ = nullptr），這是義務。
//   3. 對標準庫型別，moved-from 是「valid but unspecified」，不要假設一定是空的。
//   4. 複製賦值要防自我賦值；移動賦值同樣要（this == &other 時不可先 delete）。
//   5. 成員初始化順序只看宣告順序；別關掉 -Wreorder。
//   6. delete[] nullptr 是合法且安全的 no-op，所以歸零後的物件解構不會出事。
//   7. 現代預設是 Rule of Zero；本檔的手寫五件組只用在真的要做 RAII 包裝器時。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Rule of Five
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Rule of Five 是哪五個？Rule of Zero 又是什麼？
//     答：五個是解構子、複製建構子、複製賦值、移動建構子、移動賦值；只要自訂了
//         其中一個（通常是因為類別持有裸資源），其餘四個通常都得一起自訂。
//         Rule of Zero 是現代預設：把資源交給 unique_ptr / vector / string 這些
//         RAII 成員，五個一個都不寫，讓編譯器產生的版本自動正確。
//     追問：那什麼時候還會需要 Rule of Five？（當你正在寫的就是那個最底層的
//         RAII 包裝器——包 FILE*、socket fd、OpenGL handle 之類的 C API 資源。
//         一個專案裡這種類別通常只有個位數個。）
//
// ⚠️ 陷阱 Q2. 我只加了一個空的解構子 ~T() {}，其他都沒動，為什麼效能突然變差？
//     答：因為「宣告了解構子」會 **抑制移動建構子與移動賦值的隱式產生**。
//         類別仍然編得過——T x = std::move(y); 會被多載解析挑到複製建構子
//         （const T& 可以繫結到右值）——所以你不會收到任何錯誤或警告，
//         只是從此每次「移動」都是在深拷貝。
//     為什麼會錯：多數人腦中的模型是「沒寫移動 → 移動就不能用 → 應該會報錯」。
//         實際上 C++ 的多載解析會安靜地退回複製，這是效能陷阱而非編譯錯誤。
//         驗證方法：static_assert(std::is_nothrow_move_constructible<T>::value, "");
//         注意 **不能** 用 is_move_constructible 驗——它對「只能複製」的型別也回 true。
//
// 🔥 Q3. 為什麼移動建構子一定要 noexcept，std::vector 擴容才會用它？
//     答：vector 擴容要把舊元素搬到新緩衝區，若搬一半拋例外，舊緩衝區已被破壞
//         而新的不完整，無法回滾，就給不出強例外保證。所以 vector 用
//         std::move_if_noexcept：移動建構子是 noexcept 才移動，否則退回複製。
//     追問：退回複製時編譯器會警告嗎？（不會，完全靜默。這正是為什麼
//         「移動函式一律加 noexcept」要當成肌肉記憶。）
//
// ⚠️ 陷阱 Q4. 複製賦值裡的 if (this != &other) 只是效能優化，可以省略嗎？
//     答：不行，那是正確性問題。少了它，a = a 會先 delete[] 自己的緩衝區，
//         接著再從那塊已釋放的記憶體 strcpy → use-after-free（未定義行為）。
//     為什麼會錯：大家記得的是「自我賦值很罕見」，於是把它當成優化。
//         但 v[i] = v[j] 在 i == j 時就會發生，而排序、去重這類演算法裡並不罕見。
//         想一勞永逸就用 copy-and-swap：參數傳值，自我賦值天然安全。
//
// 🔥 Q5. 移動建構子把來源的指標偷走之後，為什麼一定要把來源設成 nullptr？
//     答：因為來源物件仍然會被解構。若不歸零，來源的解構子會 delete 一塊已被
//         新物件接管的記憶體，之後新物件解構時再 delete 一次 → double free。
//         歸零後解構走的是 delete[] nullptr，那是合法的 no-op。
//     追問：那 size_ 也要歸零嗎？（要。不歸零的話 size() 會回報一個實際上不存在
//         的長度，物件雖能安全解構，狀態卻自相矛盾。）
// ═══════════════════════════════════════════════════════════════════════════
//
// 註：Rule of Five 是「類別設計規則」，不對應任何一題 LeetCode 演算法題，
//     因此本檔不放【LeetCode 實戰範例】，改以實務 RAII 包裝器示範。

#include <iostream>
#include <cstring>
#include <utility>
#include <cstdio>
#include <string>
#include <type_traits>

class String {
    // ⚠️ 宣告順序 = 初始化順序。size_ 必須宣告在 data_ 之前，
    //    因為 data_ 的初始化式要用到長度。反過來寫的話 data_ 會先被初始化，
    //    此時 size_ 還是垃圾值 → 配置出錯誤大小的緩衝區。
    size_t size_;
    char* data_;

public:
    // ===== 建構子 =====
    String(const char* s = "")
        : size_(std::strlen(s))
        , data_(new char[size_ + 1])
    {
        std::strcpy(data_, s);
    }

    // ===== 1. 解構子 =====
    ~String() {
        delete[] data_;              // delete[] nullptr 是合法的 no-op
    }

    // ===== 2. 複製建構子 =====（深拷貝，O(n)）
    String(const String& other)
        : size_(other.size_)
        , data_(new char[other.size_ + 1])
    {
        std::strcpy(data_, other.data_);
    }

    // ===== 3. 複製賦值 =====
    String& operator=(const String& other) {
        if (this != &other) {        // ← 自我賦值防護：正確性，不是優化
            delete[] data_;          // 先釋放自己原本的資源，否則洩漏
            size_ = other.size_;
            data_ = new char[size_ + 1];
            std::strcpy(data_, other.data_);
        }
        return *this;
    }

    // ===== 4. 移動建構子 =====（偷指標，O(1)；noexcept 是效能關鍵）
    String(String&& other) noexcept
        : size_(other.size_), data_(other.data_)
    {
        other.data_ = nullptr;       // 歸零是義務：否則來源解構時 double free
        other.size_ = 0;
    }

    // ===== 5. 移動賦值 =====
    String& operator=(String&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    const char* c_str() const { return data_ ? data_ : ""; }
    size_t size() const { return size_; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】用 Rule of Five 包裝 C API 的 FILE*
//   情境：專案要寫 log，但底層只有 C 的 fopen/fclose。裸 FILE* 一旦中途 return
//         或丟例外就會漏掉 fclose，檔案描述子（fd）會慢慢用光。
//   為什麼這裡「真的」需要 Rule of Five：FILE* 是裸資源，逐成員複製會讓兩個物件
//         持有同一個 FILE*，各自 fclose 一次 → 對同一個 stream 關兩次（未定義行為）。
//   設計選擇：檔案 handle 的「複製」本來就沒有明確語意（要複製讀寫位置嗎？），
//         所以採用實務上最常見的做法——**禁止複製、只允許移動**。
//         這是 Rule of Five 的合法變體：把不合語意的那兩個明確 = delete 掉。
// -----------------------------------------------------------------------------
class LogFile {
    std::FILE* fp_;
    std::string path_;

public:
    explicit LogFile(std::string path, const char* mode = "a")
        : fp_(std::fopen(path.c_str(), mode)), path_(std::move(path)) {}

    // 1. 解構子：唯一負責釋放的地方
    ~LogFile() {
        if (fp_) std::fclose(fp_);
    }

    // 2 & 3. 複製沒有合理語意 → 明確禁止（比留給編譯器做淺拷貝安全得多）
    LogFile(const LogFile&) = delete;
    LogFile& operator=(const LogFile&) = delete;

    // 4. 移動建構子：轉移 handle 所有權
    LogFile(LogFile&& other) noexcept
        : fp_(other.fp_), path_(std::move(other.path_))
    {
        other.fp_ = nullptr;         // 來源不再擁有 → 它的解構子不會 fclose
    }

    // 5. 移動賦值
    LogFile& operator=(LogFile&& other) noexcept {
        if (this != &other) {
            if (fp_) std::fclose(fp_);   // 先關掉自己手上這個
            fp_   = other.fp_;
            path_ = std::move(other.path_);
            other.fp_ = nullptr;
        }
        return *this;
    }

    bool is_open() const { return fp_ != nullptr; }
    const std::string& path() const { return path_; }

    void write(const char* line) {
        if (fp_) std::fprintf(fp_, "%s\n", line);
    }
};

int main() {
    std::cout << "=== 1. Rule of Five 五種操作 ===\n";
    String a("Hello");
    String b = a;              // 複製建構
    String c;
    c = a;                     // 複製賦值
    String d = std::move(a);   // 移動建構
    String e;
    e = std::move(b);          // 移動賦值

    // 注意：a、b 是「我們自己實作的」String，所以我們知道移動後會變成空字串。
    //       對 std::string / std::vector 不可以這樣假設（valid but unspecified）。
    std::cout << "a: \"" << a.c_str() << "\" (moved-from)\n";
    std::cout << "b: \"" << b.c_str() << "\" (moved-from)\n";
    std::cout << "c: \"" << c.c_str() << "\"\n";
    std::cout << "d: \"" << d.c_str() << "\"\n";
    std::cout << "e: \"" << e.c_str() << "\"\n";

    std::cout << "\n=== 2. 自我賦值必須安全 ===\n";
    String s("self");
    String& alias = s;
    s = alias;                 // 複製賦值：靠 this != &other 擋下
    std::cout << "s = s 之後: \"" << s.c_str() << "\"\n";

    std::cout << "\n=== 3. 編譯期驗證移動是否真的存在 ===\n";
    std::cout << "String   可移動建構        : "
              << std::is_move_constructible<String>::value << "\n";
    std::cout << "String   移動建構為 noexcept: "
              << std::is_nothrow_move_constructible<String>::value << "\n";
    // 只宣告解構子的類別：看起來可移動，實際走的是複製建構子
    struct OnlyDtor { std::string s; ~OnlyDtor() {} };
    std::cout << "OnlyDtor 可移動建構        : "
              << std::is_move_constructible<OnlyDtor>::value
              << "  <- true 是假象（挑到複製建構子）\n";
    std::cout << "OnlyDtor 移動建構為 noexcept: "
              << std::is_nothrow_move_constructible<OnlyDtor>::value
              << "  <- false 才是真相\n";

    std::cout << "\n=== 4. 實務：move-only 的 FILE* 包裝器 ===\n";
    {
        LogFile log("/tmp/rule_of_five_demo.log", "w");
        std::cout << "開檔成功: " << log.is_open() << ", path = " << log.path() << "\n";
        log.write("[INFO] service started");

        LogFile moved = std::move(log);   // 移動：handle 所有權轉給 moved
        moved.write("[INFO] still writable after move");
        std::cout << "移動後 moved.is_open() = " << moved.is_open() << "\n";
        std::cout << "移動後 log.is_open()   = " << log.is_open()
                  << "  <- 來源已交出 handle，解構時不會重複 fclose\n";
        // LogFile bad = moved;           // ❌ 編譯錯誤：複製建構子已 = delete
    }
    std::cout << "離開 scope，解構子自動 fclose（只會關一次）\n";

    std::cout << "\n=== 5. 物件本體大小（本機實測，x86-64）===\n";
    std::cout << "sizeof(String) = " << sizeof(String)
              << " bytes（size_t + char*，字元內容都在 heap）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.8 章：Rule of Five — 現代資源管理規則1.cpp" -o rof5_1

// === 預期輸出 ===
// === 1. Rule of Five 五種操作 ===
// a: "" (moved-from)
// b: "" (moved-from)
// c: "Hello"
// d: "Hello"
// e: "Hello"
//
// === 2. 自我賦值必須安全 ===
// s = s 之後: "self"
//
// === 3. 編譯期驗證移動是否真的存在 ===
// String   可移動建構        : 1
// String   移動建構為 noexcept: 1
// OnlyDtor 可移動建構        : 1  <- true 是假象（挑到複製建構子）
// OnlyDtor 移動建構為 noexcept: 0  <- false 才是真相
//
// === 4. 實務：move-only 的 FILE* 包裝器 ===
// 開檔成功: 1, path = /tmp/rule_of_five_demo.log
// 移動後 moved.is_open() = 1
// 移動後 log.is_open()   = 0  <- 來源已交出 handle，解構時不會重複 fclose
// 離開 scope，解構子自動 fclose（只會關一次）
//
// === 5. 物件本體大小（本機實測，x86-64）===
// sizeof(String) = 16 bytes（size_t + char*，字元內容都在 heap）
