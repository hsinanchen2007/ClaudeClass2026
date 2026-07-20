// =============================================================================
//  12_practical_flags.cpp  —  工作上「flag enum」的標準做法
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/language/enum               (enum class)
//    - https://en.cppreference.com/w/cpp/language/operator_arithmetic (位元運算子)
//    - https://en.cppreference.com/w/cpp/utility/to_underlying       (C++23 to_underlying)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 為什麼工作上常見？                                         │
//  └────────────────────────────────────────────────────────────┘
//
//  許多 API 用「位元 flag」來組合多個布林開關，例如：
//
//    open() 的 flags：O_RDONLY | O_CREAT | O_TRUNC
//    Win32 CreateFile 的 access mode
//    OpenGL glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
//
//  這類 flag 比「一堆 bool 參數」省空間、可組合、跨 ABI 穩定。在自家專
//  案內部設計也常用。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 在 C++ 用 enum class 包 flag — 但要先解決運算子問題        │
//  └────────────────────────────────────────────────────────────┘
//
//  如果直接寫：
//
//      enum class Perm : unsigned { Read = 1, Write = 2, Exec = 4 };
//      auto rw = Perm::Read | Perm::Write;   // ❌ enum class 沒有 operator|
//
//  enum class 是強型別，「不會自動跟整數互轉」，所以 | 不通。要嘛
//  (a) 改用普通 enum（沒有命名空間隔離），(b) 自己 overload operator|、&
//  等。慣用做法是 (b)，並提供 has() 之類的 helper：
//
//      constexpr Perm operator|(Perm a, Perm b) {
//          using U = std::underlying_type_t<Perm>;
//          return static_cast<Perm>(U(a) | U(b));
//      }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：定義 flag 型別、提供 |, &, ~ 與 has() / set() / clear()
//   * Demo 2：用 flag 設定一個「檔案開啟模式」並查詢
// =============================================================================

/*
補充筆記：practical_flags
  - practical_flags 屬於位元操作；位元運算適合旗標、遮罩、集合狀態、低階資料格式和某些整數技巧。
  - 位移前要確認位數合法；位移負數或位移量大於等於型別寬度會造成未定義行為。
  - signed integer 的位元表示和右移行為有細節；做遮罩和位元技巧時通常使用 unsigned 型別較清楚。
  - x & (x - 1) 可清掉最低位的 1，常用於計算 set bits 或判斷 power of two；但 x=0 要另外處理。
  - C++20 <bit> 提供 popcount、has_single_bit、rotl、bit_width 等標準工具，能取代許多手寫技巧。
  - bit_cast 是按位元重新解讀且要求大小相同與 trivially copyable；它不是任意型別轉換。
  - flags 應使用每個 bit 代表一個獨立選項，例如 Read=1<<0、Write=1<<1、Exec=1<<2。
  - 設定旗標用 OR，清除旗標用 AND 搭配 NOT，檢查旗標用 AND 後和 0 比較。
  - enum class 搭配自訂 bitwise operator 可讓旗標更型別安全，避免不同旗標群組混用。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】實務 bit flag 設計
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 位遮罩旗標的標準寫法是什麼？為什麼一定要用無號型別？
//     答：enum Perm : unsigned { Read = 1u<<0, Write = 1u<<1, Exec = 1u<<2 };
//     設定 f |= Read; 清除 f &= ~Write; 反轉 f ^= Exec; 測試 f & Read;
//     測試多個 (f & (Read|Write)) == (Read|Write)。用無號的理由：~Write 在有號型別上
//     會產生負數，而且最高位（第 31 位）的旗標用 1 << 31 在 C++17 及以前就是 UB，
//     一律寫 1u << k 既跨標準安全又表達「這是位元不是數量」的意圖。
//     追問：enum class 可以直接做旗標嗎？（不行，它不會隱式轉成整數，必須自己重載
//     operator|、operator& 等，或明確用底層型別轉換）
//
// 🔥 Q2. bit flag 和 std::bitset 怎麼選？
//     答：手動位運算的優點是可以直接當成單一整數傳給 C API、與硬體暫存器對應、
//     可以放進結構化的二進位格式；bitset<N> 的優點是可讀性佳、有 count()／any()／
//     all()／test()，越界的 test() 會拋例外，但大小必須編譯期已知，而且要轉成整數得用
//     to_ulong()／to_ullong()（數值無法表示時會拋 std::overflow_error）。與 C API 或
//     硬體打交道用前者，純內部的旗標集合用後者。
//
// ⚠️ 陷阱. 用 bit-field（struct { unsigned a : 3; }）來對應二進位協定，可行嗎？
//     答：不可行。bit-field 的配置順序、跨越儲存單元的方式、padding 都是
//     implementation-defined，不同編譯器甚至不同旗標下的佈局都可能不同。它適合用來
//     節省記憶體，但絕不適合用於序列化、網路協定或硬體暫存器對應——那些場合必須用
//     明確的位移與遮罩自己組裝。
//     為什麼會錯：bit-field 的語法看起來就像是在「精確描述位元佈局」，但標準其實沒有
//     規定那個佈局。
//
// ⚠️ 陷阱. std::vector<bool> v; auto x = v[0]; 之後改 v[0]，x 會跟著變嗎？
//     答：會。vector<bool> 是空間最佳化特化，每個元素只佔 1 bit，operator[] 無法回傳
//     bool&（沒有「bit 的參考」這種東西），只能回傳一個 proxy 物件（libstdc++ 是
//     std::_Bit_reference，型別名稱屬實作細節）。於是 auto 推導出的是 proxy 而不是
//     bool，x 仍指向原本那個 bit。本機實測：decltype(v[0]) 不是 bool，v[0]=false 之後
//     x 變成 0。想要真正的複製要寫 bool x = v[0]; 或 auto x = bool(v[0]);。
//     為什麼會錯：假設所有容器的 operator[] 都回傳 T&。&v[0] 同樣編不過（taking
//     address of rvalue）——它不滿足 contiguous 容器的要求、也沒有 data()，因此餵不進
//     需要 bool* 的 C API。固定長度改用 std::bitset；動態長度又要正常容器語意，用
//     std::vector<char> 或 std::deque<bool>。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <type_traits>

// 前置宣告：附加範例
static void demo_lc_461_hamming_distance();
static void demo_count_active_flags();

// 教學註解：如果這樣寫
//
//      enum class Perm : unsigned {
//          Read  = 1u << 0,
//          Write = 1u << 1,
//          Exec  = 1u << 2,
//          All   = Read | Write | Exec,   // ❌ enum class 沒 operator|
//      };
//
// 會編譯失敗，因為運算子還沒 overload。常見解法：把 All 直接寫成
// 原始整數值 0b111，或在 enum 外另做 constexpr。
//
// 正確示範（下面用 Mode 演示）：

enum class Mode : unsigned {
    None     = 0,
    Read     = 1u << 0,
    Write    = 1u << 1,
    Append   = 1u << 2,
    Binary   = 1u << 3,
};

// ─── operator overloading：把 enum class 當作 bit flag 用 ───
constexpr Mode operator|(Mode a, Mode b) {
    using U = std::underlying_type_t<Mode>;
    return static_cast<Mode>(U(a) | U(b));
}
constexpr Mode operator&(Mode a, Mode b) {
    using U = std::underlying_type_t<Mode>;
    return static_cast<Mode>(U(a) & U(b));
}
constexpr Mode operator~(Mode a) {
    using U = std::underlying_type_t<Mode>;
    return static_cast<Mode>(~U(a));
}
constexpr Mode& operator|=(Mode& a, Mode b) { return a = a | b; }
constexpr Mode& operator&=(Mode& a, Mode b) { return a = a & b; }

// 高階 helper
constexpr bool has(Mode v, Mode flag) {
    return (v & flag) == flag;
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：組合與查詢
    // ─────────────────────────────────────────────────────────
    Mode m = Mode::Read | Mode::Binary;        // 讀取 + binary
    std::cout << "[Demo1] has Read?   "  << has(m, Mode::Read)   << '\n';
    std::cout << "[Demo1] has Write?  "  << has(m, Mode::Write)  << '\n';
    std::cout << "[Demo1] has Binary? "  << has(m, Mode::Binary) << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：開啟模式 + 動態加開關
    // ─────────────────────────────────────────────────────────
    Mode opt = Mode::Write;
    opt |= Mode::Append;                       // 加開 Append
    opt |= Mode::Binary;                       // 再加開 Binary
    std::cout << "[Demo2] flags raw = "
              << static_cast<unsigned>(opt) << '\n';

    // 清掉 Append
    opt &= ~Mode::Append;
    std::cout << "[Demo2] after clear Append, has Append? "
              << has(opt, Mode::Append) << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼不用普通 enum？
    //    A：普通 enum 名字會洩漏到外層 scope（enum Foo { Read }; 之後
    //       Read 直接可用，但和別的程式碼撞名）；enum class 強隔離 +
    //       強型別，更安全。代價是要自己 overload 運算子。
    //
    //  Q2：怎麼避免每個 enum 都 overload 一次？
    //    A：寫一個 trait + 條件 enable 版的 operator| 即可：
    //
    //         template <class E> struct enable_bitmask { static constexpr bool value = false; };
    //         template <class E, class = std::enable_if_t<enable_bitmask<E>::value>>
    //         constexpr E operator|(E a, E b) { ... }
    //
    //       對任意 enum 只要 specialize enable_bitmask<MyEnum> = true 即可。
    //
    //  Q3：std::bitset 不更好？
    //    A：std::bitset<N> 是「等大小的位元集合」，沒有命名 — 適合「fixed
    //       N 位元的 bitmap」場景。flag enum 的優勢在於「每位有名字」，
    //       讀程式時清楚對應某個語意。
    //
    demo_lc_461_hamming_distance();
    demo_count_active_flags();
    return 0;
}

// =============================================================================
//  附加 1：LeetCode 461. Hamming Distance
// =============================================================================
//  題意：給整數 x、y，回傳「二進位下不同位的個數」。
//  跟 flag enum 的連結：實作 diff(a, b) 想知道「兩個 flag set 差在哪幾個位」
//  時，用 a^b 就是差異 mask；popcount 後就是「差幾個 flag」。
// =============================================================================
static int hammingDistance(int x, int y) {
    unsigned diff = static_cast<unsigned>(x) ^ static_cast<unsigned>(y);
    int cnt = 0;
    while (diff) { diff &= diff - 1; ++cnt; }
    return cnt;
}
static void demo_lc_461_hamming_distance() {
    Mode a = Mode::Read | Mode::Binary;
    Mode b = Mode::Write | Mode::Binary;
    int d = hammingDistance(static_cast<int>(a), static_cast<int>(b));
    std::cout << "[LC461] flag diff(Read+Binary, Write+Binary) = " << d << " (= 2)\n";
}

// =============================================================================
//  附加 2：實用範例 — 計算 Mode 內「開了幾個 flag」
// =============================================================================
//  工作上很常需要：給一個 flag 組合，統計「開了幾個選項」（用於審計、報表）。
//  本質就是 popcount。下面同時 demo「列出名稱」這個 debug 用方法。
// =============================================================================
static int countActiveFlags(Mode m) {
    unsigned v = static_cast<unsigned>(m);
    int cnt = 0;
    while (v) { v &= v - 1; ++cnt; }
    return cnt;
}
static void demo_count_active_flags() {
    Mode m = Mode::Read | Mode::Append | Mode::Binary;
    std::cout << "[count_flags] active count = " << countActiveFlags(m) << " (= 3)\n";
    // 列出名稱（debug 時常用）
    struct NamedFlag { Mode flag; const char* name; };
    NamedFlag table[] = {
        {Mode::Read, "Read"}, {Mode::Write, "Write"},
        {Mode::Append, "Append"}, {Mode::Binary, "Binary"},
    };
    std::cout << "[count_flags] active flags: ";
    for (auto& f : table) if (has(m, f.flag)) std::cout << f.name << ' ';
    std::cout << '\n';
}
