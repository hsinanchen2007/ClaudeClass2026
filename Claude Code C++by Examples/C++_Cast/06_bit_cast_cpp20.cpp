// =============================================================================
//  06_bit_cast_cpp20.cpp  —  std::bit_cast (C++20)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/numeric/bit_cast
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 為什麼有 bit_cast？                                        │
//  └────────────────────────────────────────────────────────────┘
//
//  之前要做「把這塊 bit 重新解釋成另一型」有三條路：
//
//   A) reinterpret_cast<T*>(&src)         ❌ 違反 strict aliasing → UB
//   B) union { float f; uint32_t i; }      ❌ C++ 是 UB（C 允許）
//   C) std::memcpy(&dst, &src, n)          ✅ 標準允許但醜
//
//  C++20 的 std::bit_cast<T>(src) 是「memcpy 的型別安全版」：
//   * 強制檢查 sizeof(From) == sizeof(To)
//   * 強制檢查兩邊都是 trivially_copyable
//   * 編譯器把它優化到「零成本」（單一 mov）
//   * constexpr 友善（編譯期就能算）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 跟 reinterpret_cast 的差異                                 │
//  └────────────────────────────────────────────────────────────┘
//
//   reinterpret_cast：操作「指標 / ref 層級」 — 同一塊記憶體當另一型 lvalue
//                     讀，會踩 strict aliasing
//   bit_cast        ：操作「值層級」 — 拷貝 bit 出來成新物件，沒有 alias
//                     問題，且 constexpr
//
//  簡單記法：「值對值的 bit 拷貝」就用 bit_cast；「真的要寫一塊記憶體當
//  另一型操作」（很少需要）才考慮 reinterpret_cast + char* 路線。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：float → uint32_t（看 IEEE 754 bit pattern）
//   * Demo 2：uint64_t → double（從序列化資料還原 double）
//   * Demo 3：constexpr bit_cast — 編譯期就能算
// =============================================================================

/*
補充筆記：bit_cast_cpp20
  - bit_cast_cpp20 要先分清楚四種命名轉型：static_cast、dynamic_cast、const_cast、reinterpret_cast 各自解決不同問題。
  - static_cast 用於明確且語意合理的轉換，例如數值轉型、base/derived 已知方向；它不做執行期型別檢查。
  - dynamic_cast 用於 polymorphic base 上的安全向下轉型；失敗時 pointer 得到 nullptr，reference 會丟 std::bad_cast。
  - const_cast 只能調整 const/volatile 屬性；若原物件本來就是 const，移除 const 後修改是未定義行為。
  - reinterpret_cast 是低階重新解讀位元或位址，最容易違反 aliasing、alignment 和生命週期規則；能不用就不用。
  - C++ 風格轉型 (T)x 太模糊，可能偷偷做 const_cast 或 reinterpret_cast；教材應優先使用具名 cast 表達意圖。
  - std::bit_cast 適合在不違反 strict aliasing 的前提下查看位元表示。
  - bit_cast 會產生新的目標型別物件，不是讓同一塊記憶體同時具有兩種生命週期。
  - 大小不同的型別不能 bit_cast；需要序列化時應明確處理 byte array 和 endian。
*/
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>

#if __cplusplus >= 202002L
#  include <bit>
#endif

#if __cplusplus >= 202002L
// 前置宣告：附加範例
static void demo_float_hash_via_bit_cast();
static void demo_double_to_bytes();
#endif

int main() {
#if __cplusplus < 202002L
    std::cout << "[skipped] need C++20 std::bit_cast\n";
    return 0;
#else
    // ─────────────────────────────────────────────────────────
    // Demo 1：float → uint32_t
    // ─────────────────────────────────────────────────────────
    float f = 1.0f;
    auto bits = std::bit_cast<std::uint32_t>(f);
    std::cout << "[Demo1] float 1.0 → 0x" << std::hex << bits << std::dec
              << " (= 0x3f800000)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：從 raw byte 還原 double
    // ─────────────────────────────────────────────────────────
    std::uint64_t pi_bits = 0x400921FB54442D18ULL;   // π 的 IEEE 754 表示
    double pi = std::bit_cast<double>(pi_bits);
    std::cout << "[Demo2] uint64 0x400921FB54442D18 → double "
              << pi << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：constexpr — 編譯期算 1.5f 的 bit pattern
    // ─────────────────────────────────────────────────────────
    constexpr std::uint32_t b = std::bit_cast<std::uint32_t>(1.5f);
    static_assert(b == 0x3FC00000u, "constexpr bit_cast at compile time!");
    std::cout << "[Demo3] constexpr bit_cast: 1.5f → 0x"
              << std::hex << b << std::dec << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 reinterpret_cast 不能做 constexpr？
    //    A：reinterpret_cast 操作的是「指標」(記憶體位址) — 編譯期沒有真實
    //       記憶體，無法做。bit_cast 操作的是「值」 — 純資料拷貝，編譯期能算。
    //
    //  Q2：bit_cast 慢嗎？
    //    A：不慢。編譯器把它降為單一 mov / 不做動作。它跟 memcpy 的 -O2
    //       輸出基本相同。優勢純粹在「型別系統檢查 + 可讀性 + constexpr」。
    //
    //  Q3：兩邊大小不一樣怎麼辦？
    //    A：bit_cast 會編譯失敗。標準不提供「截斷或補 0」的版本 — 那種要自
    //       己用 memcpy + zero buffer 模擬。
    demo_float_hash_via_bit_cast();
    demo_double_to_bytes();
#endif
    return 0;
}

#if __cplusplus >= 202002L
// =============================================================================
//  附加 1：實用範例 — float 的 hash key
// =============================================================================
//  unordered_map<float, V> 預設有 hash，但若想自己控制 hash policy（例如
//  把 -0.0 和 +0.0 視為相同 key、或處理 NaN），最簡單的做法就是 bit_cast
//  到 uint32_t 再對它做 hash。
//  注意：直接 bit_cast 後 -0.0 (0x80000000) 跟 +0.0 (0x00000000) 不同 hash，
//        需要先 normalize。本範例 demo 基本做法。
// =============================================================================
static std::size_t hashFloat(float f) {
    if (f == 0.0f) f = 0.0f; // 把 -0.0 統一成 +0.0
    auto bits = std::bit_cast<std::uint32_t>(f);
    return std::hash<std::uint32_t>{}(bits);
}
static void demo_float_hash_via_bit_cast() {
    std::cout << "[hash_float] hash(+0.0) == hash(-0.0) ? "
              << (hashFloat(0.0f) == hashFloat(-0.0f) ? "yes" : "no") << '\n';
    std::cout << "[hash_float] hash(3.14) = " << hashFloat(3.14f) << '\n';
}

// =============================================================================
//  附加 2：實用範例 — 把 double 變成 byte array（網路傳輸）
// =============================================================================
//  跨機器傳浮點數時，通常先轉成 bytes，再 byte-swap 處理 endian。bit_cast
//  比 memcpy 多一個型別檢查，且 constexpr 友善。
//  注意：跨平台還要處理浮點數格式不同（罕見，但嵌入式可能遇到）。
// =============================================================================
struct DoubleBytes { unsigned char b[8]; };
static void demo_double_to_bytes() {
    double pi = 3.14159265358979;
    auto bytes = std::bit_cast<DoubleBytes>(pi);
    std::cout << "[double->bytes] pi raw:";
    for (auto x : bytes.b) std::cout << ' ' << std::hex << static_cast<int>(x);
    std::cout << std::dec << '\n';

    // 還原
    double back = std::bit_cast<double>(bytes);
    std::cout << "[double->bytes] round-trip = " << back << '\n';
}
#endif
