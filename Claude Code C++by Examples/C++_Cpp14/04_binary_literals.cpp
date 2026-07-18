// =============================================================================
//  04_binary_literals.cpp  —  二進位字面量 0b... (C++14)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/integer_literal
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、語法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      0b1010      = 10 (二進位)
//      0b1111'0000 = 240 (配合千位分隔符更好讀)
//
//  其他進制字面量（C++ 早就有）：
//      0           八進位前綴
//      0x          十六進位
//      0b          二進位 (C++14)
//
//      012  = 10 (八進位)
//      0xA  = 10 (十六進位)
//      0b1010 = 10 (二進位)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、好處                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * 表達 bitmask / hardware register 時直觀
//   * 對「位元邏輯」的 code review 更可讀
//
//  例：
//      const uint32_t MASK_PERMISSION =
//          0b0000'0111;        // 比 0x07 直觀 — 一眼看到「最低 3 bit」
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
// =============================================================================

/*
補充筆記：binary_literals
  - binary_literals 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - 0b 前綴讓 bit pattern 直接寫在程式中，適合 mask 和教學示範。
  - 二進位字面值仍是整數，型別和溢位規則與十進位字面值相同。
*/
#include <bitset>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：基本對照
    // ─────────────────────────────────────────────────────────
    int dec = 10;
    int oct = 012;
    int hex = 0xA;
    int bin = 0b1010;
    std::cout << "[Demo1] all == 10? "
              << (dec == 10 && oct == 10 && hex == 10 && bin == 10) << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：用 0b 表達 bitmask
    // ─────────────────────────────────────────────────────────
    constexpr std::uint32_t READ  = 0b0000'0001;
    constexpr std::uint32_t WRITE = 0b0000'0010;
    constexpr std::uint32_t EXEC  = 0b0000'0100;

    std::uint32_t perm = READ | EXEC;
    std::cout << "[Demo2] perm = " << std::bitset<8>(perm)
              << "  has READ? "  << ((perm & READ)  != 0)
              << "  has WRITE? " << ((perm & WRITE) != 0)
              << "  has EXEC? "  << ((perm & EXEC)  != 0) << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：硬體 register-like 對應 bit pattern
    //   假設：[7:6] mode、[5:0] address
    // ─────────────────────────────────────────────────────────
    constexpr std::uint8_t MODE_READ_ALL  = 0b11'000000;
    constexpr std::uint8_t addr_mask      = 0b00'111111;
    std::uint8_t cmd = MODE_READ_ALL | (0x2A & addr_mask);
    std::cout << "[Demo3] cmd = " << std::bitset<8>(cmd)
              << "  mode=" << ((cmd >> 6) & 0b11)
              << "  addr=" << (cmd & addr_mask) << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：八進位字面量 0 vs 二進位 0b 容易撞嗎？
    //    A：習慣很重要。寫 0123 是「八進位的 83」 — 多人看了沒注意到誤以
    //       為十進位 123。能避免就避免；改用十進位、十六進位、二進位三選
    //       一即可。八進位主要剩 chmod 之類舊習慣。
    //
    //  Q2：0b 跟 std::bitset 互轉？
    //    A：std::bitset<N>(value) 從整數建構；.to_ulong() / .to_string()
    //       回去。位元字面量配 std::bitset 印出來 debug 很直觀。
    //
    //  Q3：跟 C 相容嗎？
    //    A：0b 字面量是 C++14（與 C23）才標準化；之前 GCC / clang 提供
    //       extension。要寫絕對 portable 的 C99 代碼還是用 0x。
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 191. Number of 1 Bits — 計算二進位中 1 的個數
    //   題意：給一個 uint32_t，回傳「二進位表示中 1 的個數」(popcount)。
    //   為什麼放這？示範 0b 字面量配位元運算，邏輯一目瞭然。
    //   難度: easy
    // ─────────────────────────────────────────────────────────
    auto hammingWeight = [](std::uint32_t n) {
        int cnt = 0;
        while (n != 0) {
            cnt += static_cast<int>(n & 0b1u);   // 看最低位
            n >>= 1;
        }
        return cnt;
    };
    std::cout << "[LC191] popcount(0b00000000000000000000000000001011) = "
              << hammingWeight(0b00000000000000000000000000001011u) << '\n';  // 3
    std::cout << "[LC191] popcount(0b11111111111111111111111111111101) = "
              << hammingWeight(0b11111111111111111111111111111101u) << '\n';  // 31

    // ─────────────────────────────────────────────────────────
    // 實用範例：權限旗標 — 二進位字面量讓「位元欄位」一目瞭然
    //   工作上常見：linux 風格的 rwx 權限
    // ─────────────────────────────────────────────────────────
    constexpr std::uint8_t P_READ  = 0b100;
    constexpr std::uint8_t P_WRITE = 0b010;
    constexpr std::uint8_t P_EXEC  = 0b001;
    std::uint8_t user_perm = P_READ | P_WRITE;       // rw-
    std::uint8_t admin_perm = P_READ | P_WRITE | P_EXEC; // rwx
    auto toRwx = [](std::uint8_t p) {
        std::string s;
        s += (p & P_READ)  ? 'r' : '-';
        s += (p & P_WRITE) ? 'w' : '-';
        s += (p & P_EXEC)  ? 'x' : '-';
        return s;
    };
    std::cout << "[Demo4] user  = " << toRwx(user_perm)  << '\n';
    std::cout << "[Demo4] admin = " << toRwx(admin_perm) << '\n';

    return 0;
}
