// =============================================================================
//  12_byte.cpp  —  std::byte (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/types/byte
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼有 std::byte？                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  C 跟 C++98 用 char / unsigned char 表達「一個 byte 的記憶體」 — 但 char
//  本質是「字元」，跟 byte 混淆語意。例如：
//
//      const char* buffer = readFile();   // 是字元還是二進位？
//
//  C++17 引入 std::byte：明確標示「這是位元組，不是字元」。
//
//      std::byte b{0x42};
//      std::byte data[] = {std::byte{0xCA}, std::byte{0xFE}};
//
//  特性：
//   * 是 enum class — 不會跟 int / char 隱式互轉
//   * 只支援位元運算（&、|、^、~、<<、>>）
//   * 不支援算術 +、-、*、/（不是「數字」而是「位元組」）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、用法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      std::byte b{0xFF};                        // 建構：要 brace init
//      // std::byte b = 0xFF;                    // ❌ enum class 不隱式
//      auto v = std::to_integer<int>(b);          // 取整數值
//      b |= std::byte{0x01};                      // 位元 OR-assign
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、典型場景                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * 二進位 buffer / 序列化：vector<std::byte>
//   * 跟 C API 互動的低階記憶體操作
//   * 取代「unsigned char* 用作 byte buffer」的傳統寫法
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
// =============================================================================

/*
補充筆記：byte
  - byte 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - std::byte 表示原始 byte，不是字元也不是整數，避免把 buffer 誤當數值運算。
  - std::byte 需要用 std::to_integer 轉回整數觀察值，這讓低階操作更明確。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::byte
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::byte 和 char / unsigned char 差在哪？
//     答：它是 enum class byte : unsigned char，表達「純記憶體位元組」，既不是數字
//         也不是字元。因為是 scoped enum，它不支援算術運算（+、* 都不行），
//         只支援位元運算（&、|、^、~、<<、>>）與 std::to_integer<T> 取值。
//         這在型別層面就擋掉了「把原始位元組誤當字元或整數用」。
//     追問：它享有 aliasing 豁免嗎？（有，std::byte 與 char、unsigned char 同為可
//           合法別名任意物件表示的型別）
//
// Q2. 怎麼建立一個 std::byte？為什麼 std::byte b = 0x42; 編不過？
//     答：scoped enum 沒有從整數的隱式轉換，必須寫 std::byte b{0x42};
//         取回數值要用 std::to_integer<int>(b)，也不能直接 << 到 ostream。
//         多寫的這幾個字正是它的價值：每次數值化都是明示的。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：建構 + 位元運算
    // ─────────────────────────────────────────────────────────
    std::byte a{0b1010'1010};
    std::byte b{0b0101'0101};

    std::byte c = a | b;          // 0xFF
    std::byte d = a & b;          // 0
    std::byte e = a ^ b;          // 0xFF
    std::byte f = ~a;             // 0x55

    std::cout << std::hex << "[Demo1] "
              << "a|b=0x" << std::to_integer<int>(c)
              << " a&b=0x" << std::to_integer<int>(d)
              << " a^b=0x" << std::to_integer<int>(e)
              << " ~a=0x"  << std::to_integer<int>(f)
              << std::dec << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：byte buffer 處理
    // ─────────────────────────────────────────────────────────
    std::vector<std::byte> buf;
    buf.reserve(4);
    buf.push_back(std::byte{0xDE});
    buf.push_back(std::byte{0xAD});
    buf.push_back(std::byte{0xBE});
    buf.push_back(std::byte{0xEF});

    std::cout << "[Demo2] buf:";
    for (auto byte : buf) {
        std::cout << ' ' << std::hex << std::setw(2) << std::setfill('0')
                  << std::to_integer<int>(byte);
    }
    std::cout << std::dec << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：跟 unsigned char* 互轉（C API 邊界）
    //   reinterpret_cast<std::byte*>(unsigned_char_ptr) 是合法的 — 兩者
    //   layout-compatible
    // ─────────────────────────────────────────────────────────
    std::uint8_t raw[] = {0x12, 0x34, 0x56, 0x78};
    auto* bytePtr = reinterpret_cast<std::byte*>(raw);
    std::cout << "[Demo3] first byte = 0x" << std::hex
              << std::to_integer<int>(bytePtr[0]) << std::dec << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::byte 跟 unsigned char 真的不同嗎？
    //    A：layout 上完全相同（都是 1 byte）。差別在「型別系統」 — std::byte
    //       是 enum class，不允許 +、-、字元字面量等操作。語意上更準確。
    //
    //  Q2：什麼情況還是用 unsigned char？
    //    A：跟舊 C API 完全相容 / 需要 char 算術 / 跨 C-C++ 邊界。「新寫
    //       的 byte buffer」優先用 std::byte。
    //
    //  Q3：std::byte 有 std::hash 嗎？
    //    A：標準沒提供。要當 hash key 自己 cast：
    //         std::hash<int>{}(std::to_integer<int>(b));
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：簡單 checksum — 對 byte buffer XOR fold
    //   工作上常見：協定中常見的 checksum 計算
    // ─────────────────────────────────────────────────────────
    auto xorChecksum = [](const std::vector<std::byte>& data) {
        std::byte sum{0};
        for (auto b : data) sum ^= b;
        return std::to_integer<int>(sum);
    };
    std::vector<std::byte> packet{
        std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40}};
    std::cout << "[Demo4] xor checksum = 0x" << std::hex << xorChecksum(packet)
              << std::dec << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：把整數打包成 little-endian byte 序列
    //   工作上常見：序列化 32-bit 欄位送上 socket
    // ─────────────────────────────────────────────────────────
    auto packLE32 = [](std::uint32_t v) {
        std::vector<std::byte> out(4);
        for (int i = 0; i < 4; ++i) {
            out[i] = std::byte{static_cast<std::uint8_t>((v >> (i * 8)) & 0xFFu)};
        }
        return out;
    };
    auto bytes = packLE32(0x12345678u);
    std::cout << "[Demo5] packLE32(0x12345678) =";
    for (auto b : bytes) {
        std::cout << ' ' << std::hex << std::setw(2) << std::setfill('0')
                  << std::to_integer<int>(b);
    }
    std::cout << std::dec << '\n';

    return 0;
}
