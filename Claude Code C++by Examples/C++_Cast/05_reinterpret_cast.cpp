// =============================================================================
//  05_reinterpret_cast.cpp  —  reinterpret_cast 詳解
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/reinterpret_cast
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、reinterpret_cast 是什麼？                              │
//  └────────────────────────────────────────────────────────────┘
//
//  「強制把這塊記憶體當成另一型別看」 — 不做任何轉換、不檢查相容性、不
//  call 建構子。基本上就是「相信我，別問」的暴力 cast。
//
//  常見用法：
//   * 任意指標 ↔ uintptr_t（保存指標數值、做 hash）
//   * 不相干指標互轉（例如 char* ↔ unsigned char* — 拿來逐 byte 處理）
//   * 函式指標互轉（不同簽名 — 危險，要保證 ABI 相容）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、危險：strict aliasing 與 alignment                     │
//  └────────────────────────────────────────────────────────────┘
//
//  Strict aliasing rule（C++ 標準）：
//
//   只能透過「相容的型別」或「char/unsigned char/std::byte」的 lvalue 存取
//   一個物件。違反 → UB，編譯器會做出乍看莫名其妙的優化。
//
//  例：
//      int x = 0x12345678;
//      float* fp = reinterpret_cast<float*>(&x);
//      float y = *fp;                 // ❌ UB（透過 float lvalue 存取 int）
//
//  唯二「真的安全」用 reinterpret_cast 的情境：
//   1) 透過 char* / unsigned char* / std::byte* 逐 byte 看
//   2) 指標 ↔ uintptr_t（標準保證可雙向互轉，再轉回原型別也安全）
//
//  Alignment 也是地雷：
//      char buf[16];
//      double* dp = reinterpret_cast<double*>(buf);  // 可能 misaligned → UB
//      *dp = 3.14;
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、想做「型別重新解釋」的安全替代                         │
//  └────────────────────────────────────────────────────────────┘
//
//      A) std::memcpy(&dst, &src, sizeof(dst))  — 標準允許、編譯器會優化
//      B) std::bit_cast<T>(src)                  — C++20，constexpr 級安全
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：透過 unsigned char* 看 int 的 byte（合法）
//   * Demo 2：指標 ↔ uintptr_t
//   * Demo 3：危險範例（用 memcpy 替代）
// =============================================================================

/*
補充筆記：reinterpret cast
  - reinterpret_cast 是低階重新解讀位元或指標的工具，通常牽涉 alignment、aliasing 與 object lifetime。
  - 它不保證你用新型別讀寫就合法；能轉型不代表能安全解參考。
  - 處理位元表示時優先考慮 std::bit_cast、memcpy 或明確序列化格式。
  - reinterpret cast 要先分清楚四種命名轉型：static_cast、dynamic_cast、const_cast、reinterpret_cast 各自解決不同問題。
  - static_cast 用於明確且語意合理的轉換，例如數值轉型、base/derived 已知方向；它不做執行期型別檢查。
  - dynamic_cast 用於 polymorphic base 上的安全向下轉型；失敗時 pointer 得到 nullptr，reference 會丟 std::bad_cast。
  - const_cast 只能調整 const/volatile 屬性；若原物件本來就是 const，移除 const 後修改是未定義行為。
  - reinterpret_cast 是低階重新解讀位元或位址，最容易違反 aliasing、alignment 和生命週期規則；能不用就不用。
  - C++ 風格轉型 (T)x 太模糊，可能偷偷做 const_cast 或 reinterpret_cast；教材應優先使用具名 cast 表達意圖。
*/
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

// 前置宣告：附加範例
static void demo_serialize_struct_to_bytes();
static void demo_pointer_tagging();

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：透過 unsigned char* 看 int 的記憶體佈局
    //   這是合法用法 — char/unsigned char/std::byte 對任何物件都「相容」
    // ─────────────────────────────────────────────────────────
    std::uint32_t n = 0x12345678;
    auto* p = reinterpret_cast<unsigned char*>(&n);
    std::cout << "[Demo1] bytes of 0x12345678:";
    for (size_t i = 0; i < sizeof(n); ++i) {
        std::cout << " 0x" << std::hex << static_cast<int>(p[i]);
    }
    std::cout << std::dec << " (little-endian → 78 56 34 12)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：指標 ↔ uintptr_t — 標準保證安全往返
    //   常用於：把指標當作 hash key、序列化暫存
    // ─────────────────────────────────────────────────────────
    int x = 42;
    int* px = &x;
    auto raw = reinterpret_cast<std::uintptr_t>(px);
    int* px_back = reinterpret_cast<int*>(raw);
    std::cout << "[Demo2] *px_back = " << *px_back
              << " (round-trip ok)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 3：危險範例 — 不要做
    //
    //     int v = 0x40490FDB;
    //     float* fp = reinterpret_cast<float*>(&v);
    //     std::cout << *fp;     // ❌ strict aliasing UB
    //
    //   想看 int 的 bit pattern 對應的 float，請用 memcpy（或 C++20 bit_cast）：
    // ─────────────────────────────────────────────────────────
    {
        std::uint32_t bits = 0x40490FDBu;     // ≈ pi (IEEE 754)
        float f;
        std::memcpy(&f, &bits, sizeof(f));    // ✅ 標準保證安全
        std::cout << "[Demo3] memcpy bits 0x40490FDB -> float "
                  << f << " (≈ 3.14159...)\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 std::byte / char / unsigned char 是例外？
    //    A：標準明確列出這些型別「可以對任意物件做 lvalue 存取」 —
    //       它們是「raw memory view」的官方型別。其他型別都受 strict
    //       aliasing 規則限制。
    //
    //  Q2：reinterpret_cast 跟 static_cast 同時都能做的轉換結果一樣嗎？
    //    A：兩者語法都能寫例如 (T*)(void*) 的轉換，但語意不同：static_cast
    //       會檢查相容性、reinterpret_cast 不檢查。能用 static_cast 就用，
    //       它更窄、更安全。
    //
    //  Q3：什麼時候 reinterpret_cast 是真的必要？
    //    A：底層 buffer 解析、SIMD intrinsic 互轉、低階 C ABI 相容。日常
    //       application code 應該幾乎用不到 — 看到就先警覺。
    //
    demo_serialize_struct_to_bytes();
    demo_pointer_tagging();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — 用 unsigned char* 把 trivially copyable struct 序列化
// =============================================================================
//  網路 / 檔案 I/O 常需要把一個 POD struct「以原始 byte 形式」寫入 / 讀出。
//  reinterpret_cast<unsigned char*> 是合法的（char/unsigned char/byte 例外），
//  逐 byte 拷貝是標準允許的型別擦除做法。
//  注意：跨機器需處理 endian 與 padding；只在「同 ABI 內部」做才安全。
// =============================================================================
struct PacketHeader {
    std::uint32_t magic;
    std::uint16_t version;
    std::uint16_t length;
};
static void demo_serialize_struct_to_bytes() {
    PacketHeader h{0xDEADBEEFu, 1, 256};
    std::vector<unsigned char> bytes(sizeof(h));
    // ✅ 合法：透過 unsigned char* 視同 raw byte view
    auto* src = reinterpret_cast<const unsigned char*>(&h);
    std::memcpy(bytes.data(), src, sizeof(h));

    // 反向還原
    PacketHeader back{};
    std::memcpy(&back, bytes.data(), sizeof(back));
    std::cout << "[serialize] magic=0x" << std::hex << back.magic
              << std::dec << " version=" << back.version
              << " length=" << back.length << '\n';
}

// =============================================================================
//  附加 2：實用範例 — 指標 tagging（把指標低位拿來存標記）
// =============================================================================
//  在資料結構（RB-tree colored pointer、GC mark bit）裡，指標的最低幾個 bit
//  因為對齊要求一定為 0 — 可以用來偷藏 flag。reinterpret_cast 指標 ↔ uintptr_t
//  是標準保證的安全用法。
//  注意：自訂結構若沒 alignment 要求，可能就沒空位可用；本範例假設 4-byte 對齊。
// =============================================================================
static void demo_pointer_tagging() {
    alignas(4) int value = 42;
    int* ptr = &value;

    auto raw = reinterpret_cast<std::uintptr_t>(ptr);
    raw |= 0x1u;                                          // 在最低位 set 一個 flag
    bool flag = (raw & 0x1u) != 0;
    auto recovered = reinterpret_cast<int*>(raw & ~std::uintptr_t{0x3}); // 清掉低 2 bits

    std::cout << "[tagging] flag=" << flag
              << " *ptr=" << *recovered << " (= 42)\n";
}
