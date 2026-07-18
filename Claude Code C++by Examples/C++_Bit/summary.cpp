/*
================================================================================
【C++_Bit/summary.cpp】

本目錄主題：位元運算（bitwise）與實務 bit flag

從檔名可見本目錄涵蓋：
  - 基本運算子：& | ^ ~ << >>
  - 常見技巧：取最低位、清最低位、判斷 2 的冪、計數等
  - C++20 <bit> 與 bit_cast（本 summary 以條件編譯介紹，確保 C++17 也能編譯）
  - 實務案例：bit flags（權限/選項開關）

本 summary 原則：
  - 不額外加入題庫類範例，這裡只整理位元觀念與標準工具
  - 用最小可執行示例 + 大量繁中註釋把「為什麼」講清楚

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_Bit/C++_Bit summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Bit/C++_Bit summary 屬於位元操作；位元運算適合旗標、遮罩、集合狀態、低階資料格式和某些整數技巧。
  - 位移前要確認位數合法；位移負數或位移量大於等於型別寬度會造成未定義行為。
  - signed integer 的位元表示和右移行為有細節；做遮罩和位元技巧時通常使用 unsigned 型別較清楚。
  - x & (x - 1) 可清掉最低位的 1，常用於計算 set bits 或判斷 power of two；但 x=0 要另外處理。
  - C++20 <bit> 提供 popcount、has_single_bit、rotl、bit_width 等標準工具，能取代許多手寫技巧。
  - bit_cast 是按位元重新解讀且要求大小相同與 trivially copyable；它不是任意型別轉換。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Bit/C++_Bit summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/
#include <bitset>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

// 把 32-bit 印成二進位（除錯超好用）
static std::string bin32(std::uint32_t x) {
    return std::bitset<32>(x).to_string();
}

// -----------------------------------------------------------------------------
// 【重點 1】六個位元運算子與「優先級陷阱」
// -----------------------------------------------------------------------------
static void demo_basic_ops() {
    std::cout << "\n[demo_basic_ops]\n";

    std::uint32_t a = 0b11001010u; // 202
    std::uint32_t b = 0b10101100u; // 172

    std::cout << "  a     = " << bin32(a) << " (" << a << ")\n";
    std::cout << "  b     = " << bin32(b) << " (" << b << ")\n";
    std::cout << "  a & b = " << bin32(a & b) << "\n";
    std::cout << "  a | b = " << bin32(a | b) << "\n";
    std::cout << "  a ^ b = " << bin32(a ^ b) << "\n";
    std::cout << "  ~a    = " << bin32(~a) << "\n";
    std::cout << "  a<<2  = " << bin32(a << 2) << "\n";
    std::cout << "  a>>2  = " << bin32(a >> 2) << "\n";

    // ★ 優先級陷阱：`==` 比 `&` 高，所以下面會變成 n & (1==0)
    int n = 6;
    bool wrong = (n & (1 == 0));     // 永遠是 false
    bool right = ((n & 1) == 0);     // 判斷偶數的正確寫法
    std::cout << "  precedence: wrong=" << wrong << ", right=" << right << "\n";

    // 結論：位元判斷遇到比較運算，請「無腦加括號」。
}

// -----------------------------------------------------------------------------
// 【重點 2】常見 bit tricks（工作上真的常用）
// -----------------------------------------------------------------------------
static void demo_common_tricks() {
    std::cout << "\n[demo_common_tricks]\n";

    std::uint32_t x = 0b10110000u;
    std::cout << "  x=" << bin32(x) << "\n";

    // (1) 取最低位 1（LSB set bit）：x & -x（前提：用二補數，現代平台皆是）
    // 對 unsigned：-x 依定義是模 2^N 的補數，這個技巧仍然成立。
    std::uint32_t lsb = x & static_cast<std::uint32_t>(-static_cast<std::int32_t>(x));
    std::cout << "  lsb(x) = " << bin32(lsb) << " (" << lsb << ")\n";

    // (2) 清掉最低位 1：x & (x-1)
    std::uint32_t cleared = x & (x - 1);
    std::cout << "  x&(x-1)= " << bin32(cleared) << "\n";

    // (3) 判斷是否為 2 的冪：x != 0 且 (x&(x-1))==0
    auto is_pow2 = [](std::uint32_t v) {
        return v != 0 && (v & (v - 1)) == 0;
    };
    std::cout << "  is_pow2(16)=" << is_pow2(16) << ", is_pow2(18)=" << is_pow2(18) << "\n";

    // (4) 計算 set bits（popcount）：C++20 有 std::popcount；C++17 這裡示範手寫
    auto popcount = [](std::uint32_t v) {
        int cnt = 0;
        while (v) {
            v &= (v - 1); // 每次消掉一個 1
            ++cnt;
        }
        return cnt;
    };
    std::cout << "  popcount(x)=" << popcount(x) << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 3】bit flags（實務：權限/選項）
// -----------------------------------------------------------------------------
// 建議用 enum class + 位元運算（而不是散落一堆 bool）
enum class Permission : std::uint32_t {
    None  = 0,
    Read  = 1u << 0,
    Write = 1u << 1,
    Exec  = 1u << 2,
};

// 幫 enum class 做 bitwise（最小集合）
static constexpr Permission operator|(Permission a, Permission b) {
    return static_cast<Permission>(
        static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b)
    );
}
static constexpr Permission operator&(Permission a, Permission b) {
    return static_cast<Permission>(
        static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b)
    );
}
static bool has(Permission set, Permission p) {
    return (set & p) != Permission::None;
}

static void demo_flags() {
    std::cout << "\n[demo_flags]\n";

    Permission p = Permission::Read | Permission::Write;
    std::cout << "  has Read?  " << has(p, Permission::Read) << "\n";
    std::cout << "  has Exec?  " << has(p, Permission::Exec) << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 4】C++20 <bit> 與 bit_cast（概念介紹，C++17 下不啟用）
// -----------------------------------------------------------------------------
static void demo_cpp20_bit_header_note() {
    std::cout << "\n[demo_cpp20_bit_header_note]\n";
#if __cplusplus >= 202002L
    std::cout << "  C++20: 可使用 <bit> 的 std::popcount/std::rotl/std::rotr/endian...\n";
    std::cout << "  C++20: std::bit_cast 可做「位元層級」的安全轉換（類似 memcpy）\n";
#else
    std::cout << "  (此專案以 C++17 編譯時：<bit>/bit_cast 不可用，概念留待 C++20)\n";
#endif
}

int main() {
    demo_basic_ops();
    demo_common_tricks();
    demo_flags();
    demo_cpp20_bit_header_note();

    std::cout << "\n[done]\n";
    return 0;
}
