// =============================================================================
//  01_overview.cpp  —  五種 C++ cast 速覽 + 為何不要用 C-style cast
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/cast
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼 C++ 拆成五種 cast？                            │
//  └────────────────────────────────────────────────────────────┘
//
//  C 的 (T)x 一個運算符做所有事 — 但「事情」其實非常多種：
//
//   * 整數 ↔ 浮點：合理數值轉換
//   * 子類別 ↔ 父類別：物件指標的繼承轉型
//   * void* ↔ T*：類型擦除後重新指定型別
//   * const ↔ non-const：屬性增刪
//   * 完全不同的物件 bit pattern 互看（float ↔ uint32）
//
//  把所有這些都壓成 (T)x 之後，看程式時根本搞不清楚作者的意圖；而且編譯
//  器會「猜」最寬鬆的解釋，可能默默吃掉嚴重錯誤。
//
//  C++ 把它拆成 5 個關鍵字：
//
//      static_cast<T>(x)        合理「同類別」轉換 — 數值、上下轉型
//      dynamic_cast<T>(x)       多型階層中的「安全」向下轉型（會回 nullptr）
//      const_cast<T>(x)         加上 / 移除 const / volatile
//      reinterpret_cast<T>(x)   完全 bit-level 重新解釋（最危險）
//      bit_cast<T>(x) (C++20)   reinterpret_cast 的「安全 + constexpr」版
//
//  + 隱式轉換：T x = expr;（不寫 cast 也會發生）
//  + C-style cast：(T)x — 在 C++ 中等於「依次嘗試 const_cast、static_cast、
//    reinterpret_cast 直到能編過」 → 最危險，現代 C++ 風格指南都禁止使用。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、如何選 cast？                                          │
//  └────────────────────────────────────────────────────────────┘
//
//  選擇樹（從最安全往最危險走）：
//
//    需要轉換嗎？
//    ├─ 是「同型別」(int → long、derived* → base*) → static_cast
//    ├─ 是父→子，要安全檢查 → dynamic_cast
//    ├─ 只是脫掉 const 想呼叫舊 API → const_cast
//    ├─ 想看一塊 bit 當另一型 → bit_cast (C++20) 或 memcpy
//    └─ 真的需要硬轉指標 → reinterpret_cast（最後選擇）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//  本檔只「快速展示」每種 cast 一個小例，細節留給後面 02～07。
// =============================================================================

/*
補充筆記：overview
  - overview 要先分清楚四種命名轉型：static_cast、dynamic_cast、const_cast、reinterpret_cast 各自解決不同問題。
  - static_cast 用於明確且語意合理的轉換，例如數值轉型、base/derived 已知方向；它不做執行期型別檢查。
  - dynamic_cast 用於 polymorphic base 上的安全向下轉型；失敗時 pointer 得到 nullptr，reference 會丟 std::bad_cast。
  - const_cast 只能調整 const/volatile 屬性；若原物件本來就是 const，移除 const 後修改是未定義行為。
  - reinterpret_cast 是低階重新解讀位元或位址，最容易違反 aliasing、alignment 和生命週期規則；能不用就不用。
  - C++ 風格轉型 (T)x 太模糊，可能偷偷做 const_cast 或 reinterpret_cast；教材應優先使用具名 cast 表達意圖。
  - 轉型章的第一步是避免 C-style cast，因為它可能同時做多種危險轉換，讀者看不出你的真正意圖。
  - 具名 cast 的價值在於讓 code review 能直接看到風險等級：static_cast 較一般，reinterpret_cast 最低階。
  - 很多轉型需求其實是 API 設計問題；若呼叫端一直需要轉型，應回頭檢查型別關係是否設計清楚。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++ 轉型總覽（四種具名 cast + C-style cast）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ 四種具名轉型的用途與「檢查時機」分別是什麼？
//     答：static_cast — 編譯期檢查的一般轉換（數值、有繼承關係的上/下轉型、
//         void* 還原、呼叫轉換建構子）。dynamic_cast — 四種中唯一有【執行期】
//         檢查的，靠 RTTI 做多型型別的安全下轉型/側向轉型。const_cast — 唯一
//         能增刪 const/volatile 的。reinterpret_cast — 純粹重新解釋位元樣式，
//         編譯器幾乎不檢查，風險最高。
//     追問：那 C++20 的 std::bit_cast 屬於哪一類？（都不是 — 它是「值對值」的
//           位元複製，產生新物件，不像前四種在型別層級操作既有物件）
//
// 🔥 Q2. 為什麼現代 C++ 禁止用 C 風格轉型 (T)x？
//     答：三個理由：① 意圖不明確，讀 code 看不出作者想做數值轉換還是硬轉指標
//         ② 無法用文字搜尋出所有危險轉型 ③ 最關鍵：(T)x 會【依序嘗試】
//         const_cast、static_cast、static_cast+const_cast、reinterpret_cast、
//         reinterpret_cast+const_cast，取第一個編得過的 — 你以為在做安全轉換，
//         實際上可能靜默退化成 reinterpret_cast。
//     追問：怎麼在專案中強制執行？（-Wold-style-cast 編譯警告，或 clang-tidy 的
//           cppcoreguidelines-pro-type-cstyle-cast）
//
// Q3. typeid 與 dynamic_cast 的差別是什麼？
//     答：typeid(expr) 回傳 const std::type_info&；對【多型型別的 glvalue】會在
//         執行期取得動態型別，其他情況取編譯期靜態型別。關鍵語意差異：typeid 做
//         的是【精確型別相等】比較（Derived 的再衍生類別會是 false），
//         dynamic_cast 做的是【is-a 判斷】（子型別也會成功）。要「就是這個型別」
//         用 typeid，要「是這個型別或其衍生」用 dynamic_cast。
//     追問：type_info::name() 回傳什麼？（內容是 implementation-defined，標準未
//           規定；GCC/Clang 給的是 mangled name，需 abi::__cxa_demangle 還原）
//
// ⚠️ 陷阱. functional cast 寫法 T(x) 比 (T)x 安全嗎？
//     答：對 class type，T(x) 是呼叫建構子；但對 fundamental type，T(x) 的語意
//         【就等同於 (T)x】，一樣會依序嘗試各種轉型。所以它並沒有比較安全。
//     為什麼會錯：多數人看到「函式呼叫的樣子」就直覺認為它走的是建構子路徑，
//         因而以為避開了 C-style cast 的問題 — 實際上換湯不換藥。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <cstring>
#include <iostream>

struct Base { virtual ~Base() = default; };
struct Derived : Base { int x = 42; };

// 前置宣告：附加範例
static void demo_safe_numeric_narrow();
static void demo_polymorphic_visitor();

int main() {
    // ─────────────────────────────────────────────────────────
    // (1) static_cast：合理數值轉換
    // ─────────────────────────────────────────────────────────
    double d = 3.7;
    int i = static_cast<int>(d);          // 截斷到 3
    std::cout << "[static_cast] double->int = " << i << '\n';

    // ─────────────────────────────────────────────────────────
    // (2) dynamic_cast：多型階層的安全向下轉型
    //   要求 Base 有 virtual function
    // ─────────────────────────────────────────────────────────
    Base* b = new Derived;
    Derived* d2 = dynamic_cast<Derived*>(b);
    std::cout << "[dynamic_cast] derived* = " << (d2 ? "ok" : "null") << '\n';
    delete b;

    // ─────────────────────────────────────────────────────────
    // (3) const_cast：脫 const 呼叫舊 API
    // ─────────────────────────────────────────────────────────
    const char* msg = "hello";
    char* mut = const_cast<char*>(msg);
    // 注意：可以「拿掉 const」，但「真的去寫」就 UB（msg 來自 string literal）
    std::cout << "[const_cast] removed const, ptr=" << static_cast<const void*>(mut) << '\n';

    // ─────────────────────────────────────────────────────────
    // (4) reinterpret_cast：硬轉
    //   注意：嚴格說來 transparent 通過 char/byte 才安全，這個範例只示範語法
    // ─────────────────────────────────────────────────────────
    int n = 0x12345678;
    auto* p = reinterpret_cast<unsigned char*>(&n);
    std::cout << "[reinterpret_cast] first byte = 0x"
              << std::hex << static_cast<int>(p[0]) << std::dec
              << " (little-endian → 0x78)\n";

    // ─────────────────────────────────────────────────────────
    // (5) std::bit_cast (C++20) — 範例只在 C++20 可用，這裡用 memcpy 作 fallback
    // ─────────────────────────────────────────────────────────
    float f = 1.0f;
    std::uint32_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    std::cout << "[bit_cast/memcpy] float 1.0 → 0x"
              << std::hex << bits << std::dec
              << " (= 0x3f800000)\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：哪幾種 cast 在多執行緒會有問題？
    //    A：const_cast 加上實際寫入 → 通常會跟「const 物件被假設不變」
    //       的優化相衝突；reinterpret_cast 違反 strict aliasing 會被優化
    //       破壞語意。這兩個能不用就不用。
    //
    //  Q2：靜態分析工具會抓 C-style cast 嗎？
    //    A：會。clang-tidy 的 cppcoreguidelines-pro-type-cstyle-cast 就是
    //       專門抓這個。新專案應該開這個 check。
    //
    //  Q3：functional cast `T(x)` 跟 `(T)x` 一樣危險嗎？
    //    A：差不多。對 class type 它呼叫建構子，但對 fundamental type 它
    //       就是 (T)x。建議統一用 static_cast 等明寫意圖。
    //
    demo_safe_numeric_narrow();
    demo_polymorphic_visitor();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — 安全的窄化轉換偵測
// =============================================================================
//  工作上常見：要把 long long 塞回 int，但想偵測「會不會溢位」。標準做法是
//  做完 static_cast 後反推一次，比較是否一致。如果不一致代表有資訊丟失。
//  這個 idiom 在解析 protobuf / JSON int 時特別常用。
// =============================================================================
static bool safeNarrowToInt(long long src, int& dst) {
    int candidate = static_cast<int>(src);
    if (static_cast<long long>(candidate) != src) return false; // 失敗
    dst = candidate;
    return true;
}
static void demo_safe_numeric_narrow() {
    int out = 0;
    long long ok = 12345LL;
    long long bad = 5'000'000'000LL;
    std::cout << "[safe_narrow] 12345 ok? " << safeNarrowToInt(ok, out)
              << " -> " << out << '\n';
    std::cout << "[safe_narrow] 5e9   ok? " << safeNarrowToInt(bad, out)
              << " (溢位，回傳 false)\n";
}

// =============================================================================
//  附加 2：實用範例 — 多型物件型別識別
// =============================================================================
//  從 Base* 容器處理「混合型別」時，最常見的工作就是「判斷實際型別 + 取出
//  特定資料」。dynamic_cast 是標準工具。此範例展示 cast 在「資料探查」場景
//  的典型用途。
// =============================================================================
static void demo_polymorphic_visitor() {
    Base* objs[] = {new Derived, new Base, new Derived};
    int sumX = 0;
    for (Base* b : objs) {
        if (Derived* d = dynamic_cast<Derived*>(b)) {
            sumX += d->x;
        }
    }
    std::cout << "[visitor] sum of Derived::x = " << sumX << " (= 84)\n";
    for (Base* b : objs) delete b;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 01_overview.cpp -o 01_overview

// === 預期輸出 ===
// [static_cast] double->int = 3
// [dynamic_cast] derived* = ok
// [const_cast] removed const, ptr=0x600182816048
// [reinterpret_cast] first byte = 0x78 (little-endian → 0x78)
// [bit_cast/memcpy] float 1.0 → 0x3f800000 (= 0x3f800000)
// [safe_narrow] 12345 ok? 1 -> 12345
// [safe_narrow] 5e9   ok? 0 (溢位，回傳 false)
// [visitor] sum of Derived::x = 84 (= 84)
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
