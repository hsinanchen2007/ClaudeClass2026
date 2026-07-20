/*
================================================================================
【C++_String/summary.cpp】

本目錄主題：std::string（常用 API、效能、陷阱）

你需要熟的幾大類：
  - capacity/size：size, empty, reserve, shrink_to_fit
  - element access：operator[], at, front, back, data
  - search：find, rfind, find_first_of/not_of, find_last_of/not_of, starts_with/ends_with(C++20)
  - modify：append, insert, erase, replace, substr
  - compare：compare, ==, <（字典序）
  - 與 cctype 搭配做分類：isdigit/isalpha...（注意 unsigned char）

本 summary 原則：
  - 不加入 題庫 類範例
  - 以 C++17 可編譯（C++20 的 contains/starts_with/ends_with 只做提示）

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_String/C++_String summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - std::string 是擁有字元資料的容器，負責配置與釋放記憶體；std::string_view 則只是借用一段字元範圍。
  - 字串 API 要分清楚會不會改變 size、capacity、內容和 iterator 有效性；append/insert/erase/replace 都可能重新配置。
  - find 系列失敗時回傳 npos，不是 -1；npos 是 size_type 的最大值，和有號整數混用容易出錯。
  - c_str()/data() 回傳的指標只在字串未被修改且物件仍活著時有效；不要把它長期保存。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_String/C++_String summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string 整體設計(SSO / COW / ABI)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 SSO?為什麼現代 std::string 都採用它?
//     答:短字串直接存在 string 物件自身的內嵌 buffer,不呼叫 operator new,
//     超過門檻才轉 heap 配置。門檻是 implementation-defined、標準未規定任何數字:
//     libstdc++ 為 15 字元、libc++ 為 22 字元,不可依賴這些數字寫程式。
//     追問:SSO 對 move 有什麼影響?→ 短字串無法只搬指標,必須真的 memcpy buffer,
//     所以 std::string 的 move 不是無條件 O(1)。
//
// 🔥 Q2. 什麼是 COW(Copy-On-Write)?為什麼 C++11 之後不能用了?
//     答:COW 讓多個 string 共享同一塊 buffer + 引用計數,寫入時才真的複製;
//     GCC 5 之前的 libstdc++ 就是 COW。C++11 並非明文一句話禁止它,而是新要求
//     讓 COW 無法合法實作:非 const 的 operator[]/at()/begin()/data() 不得使
//     指標與迭代器失效(COW 必須在此時分家)、對不同 string 物件的並行操作不得
//     產生 data race、operator[] 要求攤還常數時間。結果現代實作全面改用 SSO。
//     追問:COW 在多執行緒下具體壞在哪?→ refcount 必須 atomic,連單執行緒程式
//     也要付原子操作代價。
//
// Q3. GCC 的 std::string Dual ABI 是怎麼回事?
//     答:GCC 5 為符合 C++11 改寫了 basic_string,ABI 因此斷裂。libstdc++ 採
//     Dual ABI:新實作放在 inline namespace std::__cxx11,舊實作保留,兩者共存
//     於同一個 libstdc++.so。由巨集 _GLIBCXX_USE_CXX11_ABI 控制(預設 1)。
//     典型症狀是連結時出現 undefined reference to foo(std::__cxx11::basic_string...)
//     ——型別名字裡帶 __cxx11 就是 ABI 不一致的指紋。
//     追問:為什麼跨 .so / DLL 邊界傳 std::string 被視為壞設計?→ ABI 太脆弱,
//     換標準庫版本、編譯器或旗標就爆;C API 邊界應該傳 const char* + 長度。
// ═══════════════════════════════════════════════════════════════════════════

#include <cctype>
#include <iostream>
#include <string>

// 註：本檔以 C++17 為主；C++20 的 starts_with/ends_with/contains 只在附錄提示。

static void print_header(const char* title) {
    std::cout << "\n[" << title << "]\n";
}

// -----------------------------------------------------------------------------
// 【重點 1】建構與指定（construct / assign）
// -----------------------------------------------------------------------------
static void demo_construct_and_assign() {
    print_header("demo_construct_and_assign");

    // (1) 各種建構子（常用版）
    std::string s1;                         // 空字串
    std::string s2("hello");                // C-string
    std::string s3 = std::string("hi");     // 明確建構
    std::string s4(5, 'A');                 // "AAAAA"
    std::string s5 = s2;                    // copy
    std::string s6 = std::move(s3);         // move（s3 變成 moved-from）

    std::cout << "  s1.size=" << s1.size() << "\n";
    std::cout << "  s2=" << s2 << "\n";
    std::cout << "  s4=" << s4 << "\n";
    std::cout << "  s5(copy)=" << s5 << "\n";
    std::cout << "  s6(move)=" << s6 << "\n";

    // (2) assign：整批替換內容（比先 clear 再 append 更直覺）
    std::string a = "xxx";
    a.assign("key=value");
    std::cout << "  assign(\"key=value\") => " << a << "\n";
    a.assign(3, '!');
    std::cout << "  assign(3,'!') => " << a << "\n";
}

static void demo_basic_and_capacity() {
    print_header("demo_basic_and_capacity");

    std::string s = "hello";
    std::cout << "  s=\"" << s << "\" size=" << s.size() << " cap=" << s.capacity() << "\n";

    s.reserve(100);
    std::cout << "  after reserve(100): cap=" << s.capacity() << "\n";

    // shrink_to_fit：請求釋放多餘容量（實作可選擇不縮小）
    s.shrink_to_fit();
    std::cout << "  after shrink_to_fit: cap=" << s.capacity() << " (may or may not shrink)\n";
}

static void demo_access_and_safe_at() {
    print_header("demo_access_and_safe_at");

    std::string s = "abc";
    std::cout << "  s[1]=" << s[1] << ", at(1)=" << s.at(1) << "\n";
    std::cout << "  front=" << s.front() << ", back=" << s.back() << "\n";

    // data()/c_str()：取得底層字元陣列指標
    // - c_str() 保證以 '\0' 結尾（C API 最常用）
    // - data() 自 C++98 就能讀取；C++11 起保證 data()[size()]=='\0' 與連續儲存；
    //   C++17 加的是「非 const（可寫入）」多載（完整演進見 data.cpp）
    std::cout << "  c_str()=\"" << s.c_str() << "\"\n";

    try {
        (void)s.at(100); // 會丟 out_of_range
    } catch (const std::out_of_range& e) {
        std::cout << "  at(100) out_of_range: " << e.what() << "\n";
    }
}

static void demo_find_and_substr() {
    print_header("demo_find_and_substr");

    std::string s = "key=value";
    auto pos = s.find('=');
    if (pos != std::string::npos) {
        std::string key = s.substr(0, pos);
        std::string val = s.substr(pos + 1);
        std::cout << "  key=" << key << ", val=" << val << "\n";
    }

    // rfind：從尾端找
    std::string path = "C:/tmp/a.txt";
    auto dot = path.rfind('.');
    if (dot != std::string::npos) std::cout << "  ext=" << path.substr(dot) << "\n";

    // find_first_of / find_first_not_of：常用於剖析/跳過空白
    std::string line = "   hello world";
    auto first_not_space = line.find_first_not_of(' ');
    std::cout << "  trim-left => \"" << line.substr(first_not_space) << "\"\n";
}

static void demo_erase_insert_replace() {
    print_header("demo_erase_insert_replace");

    std::string s = "HelloWorld";
    s.insert(5, " ");            // Hello World
    std::cout << "  insert: " << s << "\n";

    s.replace(0, 5, "Hi");       // Hi World
    std::cout << "  replace: " << s << "\n";

    s.erase(2, 1);               // HiWorld（刪空白）
    std::cout << "  erase: " << s << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 5】append / push_back / pop_back / clear / resize / swap
// -----------------------------------------------------------------------------
static void demo_append_and_modifiers() {
    print_header("demo_append_and_modifiers");

    std::string s = "hi";
    s.append("!!");              // append 字串
    s.push_back('?');            // append 單一字元
    std::cout << "  after append/push_back: " << s << "\n";

    s.pop_back();                // 移除最後字元（空字串呼叫是 UB，所以務必先檢查 empty）
    std::cout << "  after pop_back: " << s << "\n";

    s.resize(10, '.');           // 擴大：用指定字元補齊
    std::cout << "  resize(10,'.'): " << s << "\n";
    s.resize(2);                 // 縮小：砍尾端
    std::cout << "  resize(2): " << s << "\n";

    std::string other = "world";
    s.swap(other);
    std::cout << "  swap: s=" << s << ", other=" << other << "\n";

    other.clear();
    std::cout << "  clear: other.size=" << other.size() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 6】比較 compare / operator== / operator<
// -----------------------------------------------------------------------------
static void demo_compare() {
    print_header("demo_compare");

    std::string a = "apple";
    std::string b = "banana";

    std::cout << "  (a==b) " << (a == b) << "\n";
    std::cout << "  (a<b)  " << (a < b) << " (字典序)\n";
    std::cout << "  a.compare(b) = " << a.compare(b)
              << " (負數: a<b, 0: 相等, 正數: a>b)\n";
}

static void demo_cctype_pitfall() {
    print_header("demo_cctype_pitfall");

    // cctype 函式（isdigit/isalpha...）的參數應該轉成 unsigned char，
    // 否則遇到 char 為 signed、且值為負時可能 UB。
    std::string s = "A1b2";
    int digits = 0;
    for (unsigned char ch : s) {
        // ⚠️ <cctype> 參數先轉 unsigned char
        if (std::isdigit(static_cast<unsigned char>(ch))) ++digits;
    }
    std::cout << "  digits=" << digits << "\n";
}

static void demo_cpp20_string_note() {
    print_header("demo_cpp20_string_note");
#if __cplusplus >= 202002L
    std::cout << "  C++20: string::starts_with / ends_with / contains\n";
#else
    std::cout << "  (C++17：若要 starts_with/ends_with，可用 rfind/find 搭配位置判斷)\n";
#endif
}

int main() {
    demo_construct_and_assign();
    demo_basic_and_capacity();
    demo_access_and_safe_at();
    demo_find_and_substr();
    demo_erase_insert_replace();
    demo_append_and_modifiers();
    demo_compare();
    demo_cctype_pitfall();
    demo_cpp20_string_note();

    std::cout << "\n[done]\n";
    return 0;
}

/*
================================================================================
【附錄：std::string (C++17) 常用 member functions 速查表】

（此表以 cppreference 的分類整理；C++20+ 新增者在本檔主線不強推。）

Constructors / assignment
  - string()
  - string(const char*)
  - string(size_t count, char ch)
  - string(const string&) / string(string&&)
  - operator= / assign(...)

Capacity
  - size(), length(), empty()
  - capacity(), reserve(), shrink_to_fit()
  - max_size()

Element access
  - operator[], at()
  - front(), back()
  - data(), c_str()

Modifiers
  - clear()
  - insert(...), erase(...), replace(...)
  - append(...), operator+=
  - push_back(), pop_back()
  - resize()
  - swap()

Operations
  - find / rfind
  - find_first_of / find_last_of
  - find_first_not_of / find_last_not_of
  - substr()
  - compare()

Non-member（常用）
  - operator==, <, ...
  - std::getline(istream, string)

C++20 延伸（不在主線強推）
  - starts_with / ends_with / contains
================================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 (節錄) ===
//
// [demo_construct_and_assign]
//   s1.size=0
//   s2=hello
//   s4=AAAAA
//   s5(copy)=hello
//   s6(move)=hi
//   assign("key=value") => key=value
//   assign(3,'!') => !!!
//
// [demo_basic_and_capacity]
//   s="hello" size=5 cap=15
//   after reserve(100): cap=100
//   after shrink_to_fit: cap=15 (may or may not shrink)
//
// [demo_access_and_safe_at]
//   s[1]=b, at(1)=b
//   front=a, back=c
//   c_str()="abc"
//   at(100) out_of_range: basic_string::at: __n (which is 100) >= this->size() (which is 3)
// …（後略，完整輸出共 51 行）
