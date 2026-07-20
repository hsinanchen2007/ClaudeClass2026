/*
================================================================================
【C++_Cast/summary.cpp】

本目錄主題：C++ 轉型（cast）與常見陷阱

你在 C++ 會遇到的轉型大致分四種（加上 C++20 的 bit_cast）：
  1) static_cast      ：「編譯期」可驗證、語意明確的轉型（最常用）
  2) const_cast       ：移除/添加 const/volatile（只改型別屬性，不改位元）
  3) dynamic_cast     ：多型（有 virtual）下的安全向下轉型（執行期檢查）
  4) reinterpret_cast ：位元層級的重新解讀（最危險、能不用就不用）
  5) std::bit_cast    ：C++20：以位元複製做安全轉換（類似 memcpy）

本 summary 原則：
  - 不加入 題庫 類範例
  - 以 C++17 可編譯為主；C++20 內容用條件編譯說明

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_Cast/C++_Cast summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Cast/C++_Cast summary 要先分清楚四種命名轉型：static_cast、dynamic_cast、const_cast、reinterpret_cast 各自解決不同問題。
  - static_cast 用於明確且語意合理的轉換，例如數值轉型、base/derived 已知方向；它不做執行期型別檢查。
  - dynamic_cast 用於 polymorphic base 上的安全向下轉型；失敗時 pointer 得到 nullptr，reference 會丟 std::bad_cast。
  - const_cast 只能調整 const/volatile 屬性；若原物件本來就是 const，移除 const 後修改是未定義行為。
  - reinterpret_cast 是低階重新解讀位元或位址，最容易違反 aliasing、alignment 和生命週期規則；能不用就不用。
  - C++ 風格轉型 (T)x 太模糊，可能偷偷做 const_cast 或 reinterpret_cast；教材應優先使用具名 cast 表達意圖。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Cast/C++_Cast summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++_Cast 總複習
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 面試官要你「用一分鐘說完四種 cast 該怎麼選」，你怎麼答？
//     答：先問自己在做什麼：改 const/volatile → const_cast（但底層物件若真的是
//         const，改它就是 UB）；有繼承關係的轉換 → static_cast（不檢查、你負責）
//         或 dynamic_cast（執行期檢查、要多型 base）；一般數值/enum/void* 還原
//         → static_cast；想看同一塊位元的另一種型別 → std::bit_cast（C++20）或
//         memcpy，【不是】 reinterpret_cast；真的要低階硬轉指標 → reinterpret_cast，
//         而且最好只走 char* / std::byte* 那條合法路徑。
//     追問：那 C-style cast 呢？（一律不用 — 它會依序嘗試多種轉型並靜默選中最
//           危險的那個）
//
// Q2. shared_ptr 要怎麼轉型？可以直接對它用 static_cast 嗎？
//     答：【不可以】。<memory> 提供專用的輔助函式：static_pointer_cast、
//         dynamic_pointer_cast、const_pointer_cast，以及 【C++17】 才加入的
//         reinterpret_pointer_cast。它們回傳的新 shared_ptr 會【共用同一個控制
//         區塊】（等同 aliasing），所以引用計數維持正確；dynamic_pointer_cast
//         失敗時回傳空的 shared_ptr。
//     追問：unique_ptr 有對應的嗎？（標準沒有 — 所有權轉移的語意太複雜，需自行
//           release() + cast + 重新包裝，且失敗時容易漏掉所有權）
//
// Q3. 你在 code review 看到滿滿的 dynamic_cast，會給什麼建議？
//     答：把它當成【設計訊號】而非單純的效能問題：需要一長串 dynamic_cast 做型別
//         分支，通常代表這個行為本來就該是 base 上的一個 virtual 函式，由物件自己
//         回答。若型別集合是封閉且已知的，改用 std::variant + std::visit 能讓編譯器
//         【保證所有型別都處理到】，而且沒有 RTTI 成本。真的避不開的場合（跨模組
//         只拿得到 base 介面、外掛系統）才保留 dynamic_cast，並讓它遠離 hot path。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【重點 1】static_cast：最常用、最安全的「一般轉型」
// -----------------------------------------------------------------------------
static void demo_static_cast() {
    std::cout << "\n[demo_static_cast]\n";

    // (1) 整數/浮點轉換（可能截斷，請自己負責）
    double d = 3.14;
    int i = static_cast<int>(d);
    std::cout << "  double->int: " << d << " -> " << i << "\n";

    // (2) upcast（Derived* -> Base*）安全
    struct Base { virtual ~Base() = default; };
    struct Derived : Base {};
    Derived der;
    Base* pb = static_cast<Base*>(&der);
    std::cout << "  upcast ok: pb=" << pb << "\n";

    // (3) void* <-> T*（常見於 C API，但務必確保原本就是同一型別）
    int x = 42;
    void* vp = static_cast<void*>(&x);
    int* px = static_cast<int*>(vp);
    std::cout << "  void* roundtrip: *px=" << *px << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 2】const_cast：只處理 const/volatile（非常容易踩 UB）
// -----------------------------------------------------------------------------
// 規則要記住一句話：
//   - 你可以用 const_cast「拿掉型別上的 const」，
//     但如果原物件本來就是真正的 const，修改它是 UB（未定義行為）。
static void demo_const_cast() {
    std::cout << "\n[demo_const_cast]\n";

    int a = 10;
    const int* p = &a;                    // 指向「非 const 物件」的 const 指標（可合法去 const）
    int* q = const_cast<int*>(p);
    *q = 99;                              // OK：底層物件 a 不是 const
    std::cout << "  modified a via const_cast: a=" << a << "\n";

    const int c = 7;
    const int* pc = &c;
    int* qc = const_cast<int*>(pc);
    (void)qc;
    std::cout << "  NOTE: 若你寫 *qc = 1; 會是 UB（因為 c 真的是 const）\n";
}

// -----------------------------------------------------------------------------
// 【重點 3】dynamic_cast：多型下的安全向下轉型（RTTI）
// -----------------------------------------------------------------------------
struct Animal {
    virtual ~Animal() = default;          // ★ 必須是多型（有 virtual），dynamic_cast 才有意義
    virtual const char* kind() const { return "Animal"; }
};
struct Dog : Animal {
    const char* kind() const override { return "Dog"; }
    void bark() const { std::cout << "  woof!\n"; }
};
struct Cat : Animal {
    const char* kind() const override { return "Cat"; }
};

static void demo_dynamic_cast() {
    std::cout << "\n[demo_dynamic_cast]\n";

    Animal* a1 = new Dog();
    Animal* a2 = new Cat();

    // (1) 指標版：失敗回 nullptr（最常用）
    if (auto d = dynamic_cast<Dog*>(a1)) d->bark();
    if (auto d = dynamic_cast<Dog*>(a2)) d->bark();
    else std::cout << "  a2 is not Dog (nullptr)\n";

    // (2) 參考版：失敗丟 std::bad_cast
    try {
        Dog& ref = dynamic_cast<Dog&>(*a1);
        std::cout << "  ref.kind=" << ref.kind() << "\n";
    } catch (...) {
        std::cout << "  bad_cast\n";
    }

    delete a1;
    delete a2;
}

// -----------------------------------------------------------------------------
// 【重點 4】reinterpret_cast：位元重解讀（危險區）
// -----------------------------------------------------------------------------
// 典型用途（且仍需非常小心）：
//   - 與硬體/OS API 互動的「位元等價」資料
//   - 指標與整數之間的轉換（uintptr_t）
//
// 大多數情況，想做「位元等價轉換」更適合用：
//   - memcpy（C++11 起可用，最保守）
//   - C++20 的 std::bit_cast（若可用）
static void demo_reinterpret_cast() {
    std::cout << "\n[demo_reinterpret_cast]\n";

    // (1) 指標 <-> 整數（用 uintptr_t 才是正規做法）
    int x = 123;
    std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(&x);
    int* px = reinterpret_cast<int*>(addr);
    std::cout << "  addr=0x" << std::hex << addr << std::dec << ", *px=" << *px << "\n";

    // (2) 不能用 reinterpret_cast 亂把一段位元當成另一種型別來讀（容易違反 strict aliasing）
    std::cout << "  NOTE: reinterpret_cast 不是『想轉就轉』，錯用很容易 UB。\n";
}

// -----------------------------------------------------------------------------
// 【重點 5】C++20 的 std::bit_cast（概念提示）
// -----------------------------------------------------------------------------
static void demo_bit_cast_note() {
    std::cout << "\n[demo_bit_cast_note]\n";
#if __cplusplus >= 202002L
    std::cout << "  C++20: std::bit_cast<T>(u) 會做位元複製，型別必須 trivially copyable。\n";
#else
    std::cout << "  (本 summary 以 C++17 為主：bit_cast 是 C++20 才有)\n";
#endif
}

int main() {
    demo_static_cast();
    demo_const_cast();
    demo_dynamic_cast();
    demo_reinterpret_cast();
    demo_bit_cast_note();

    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
//
// [demo_static_cast]
//   double->int: 3.14 -> 3
//   upcast ok: pb=0x7ffdfa6ea230
//   void* roundtrip: *px=42
//
// [demo_const_cast]
//   modified a via const_cast: a=99
//   NOTE: 若你寫 *qc = 1; 會是 UB（因為 c 真的是 const）
//
// [demo_dynamic_cast]
//   woof!
//   a2 is not Dog (nullptr)
//   ref.kind=Dog
//
// [demo_reinterpret_cast]
//   addr=0x7ffdfa6ea254, *px=123
//   NOTE: reinterpret_cast 不是『想轉就轉』，錯用很容易 UB。
//
// [demo_bit_cast_note]
//   C++20: std::bit_cast<T>(u) 會做位元複製，型別必須 trivially copyable。
//
// [done]
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
