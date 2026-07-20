// =============================================================================
//  08_pitfalls.cpp  —  Cast 常見陷阱
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/language/cast
//    - https://en.cppreference.com/w/cpp/language/implicit_conversion (隱式轉換規則)
//    - https://en.cppreference.com/w/cpp/language/list_initialization (narrow 偵測)
//    - https://en.cppreference.com/w/cpp/language/object#Object_slicing (object slicing)
//
//  本檔列出最常見的 cast 陷阱，每個都附「為什麼會錯」「怎麼正確做」。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 1：narrow conversion（縮窄轉換）                      │
//  └────────────────────────────────────────────────────────────┘
//
//  把「大型別」塞進「小型別」可能截斷或溢位：
//
//      double d = 3.99;
//      int i = d;             // 隱式 — 變成 3，可能不符預期
//      int j = (int)d;        // C-style — 一樣，但看起來「比較認」
//      int k = static_cast<int>(d);  // 明寫意圖
//      int x{d};              // brace init → C++11 起編譯錯（保護你）
//
//  建議：「值要轉」用 brace init 或 static_cast；前者擋下大部分 narrow。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 2：signed / unsigned 混用                             │
//  └────────────────────────────────────────────────────────────┘
//
//      std::vector<int> v(5);
//      for (int i = 0; i < v.size(); ++i) { ... }
//      //         ^^^^^^^^^^^^^^^^ warning: comparison signed vs unsigned
//
//  v.size() 回傳 size_t (unsigned)，跟 int 比會把 int 提升成 unsigned。當
//  i 是負數時會被解釋為超大正數 → 邏輯錯誤。
//
//  解法：
//   * for 用 size_t i = 0
//   * 或用 std::ssize(v) (C++20) — 回傳有號版本
//   * 或 static_cast<int>(v.size())（如果你確定不會溢位）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 3：物件切片 (Object Slicing)                          │
//  └────────────────────────────────────────────────────────────┘
//
//  把子類別物件「按值」傳到接受父類別的位置 → 子類別獨有的成員會被截掉。
//
//      void use(Animal a) { ... }    // 參數 by value
//      Dog d;
//      use(d);                        // d 被切片成 Animal — 失去 Dog 部分
//
//  解法：「多型 ⇒ 用指標 / 參考」 — `void use(const Animal& a)`。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 4：函式指標 / member function pointer 互轉            │
//  └────────────────────────────────────────────────────────────┘
//
//      using FP = int(*)(int);
//      FP fp = reinterpret_cast<FP>(&someClass::method);   // ❌ UB
//
//  member function pointer 跟一般 function pointer ABI 不同（前者要帶
//  this）。混用會 UB。要綁 this 用 std::bind / lambda / std::mem_fn。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 5：dynamic_cast<Derived&> 沒做型別檢查就 use           │
//  └────────────────────────────────────────────────────────────┘
//
//      Animal& a = ...;
//      Dog& d = dynamic_cast<Dog&>(a);     // 失敗會 throw bad_cast
//      d.bark();
//
//  記得對 ref 版的 dynamic_cast 「失敗會 throw」 — 你必須 try/catch 或先用
//  pointer 版檢查 nullptr。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範可運行的部分                                       │
//  └────────────────────────────────────────────────────────────┘
// =============================================================================

/*
補充筆記：pitfalls
  - pitfalls 要先分清楚四種命名轉型：static_cast、dynamic_cast、const_cast、reinterpret_cast 各自解決不同問題。
  - static_cast 用於明確且語意合理的轉換，例如數值轉型、base/derived 已知方向；它不做執行期型別檢查。
  - dynamic_cast 用於 polymorphic base 上的安全向下轉型；失敗時 pointer 得到 nullptr，reference 會丟 std::bad_cast。
  - const_cast 只能調整 const/volatile 屬性；若原物件本來就是 const，移除 const 後修改是未定義行為。
  - reinterpret_cast 是低階重新解讀位元或位址，最容易違反 aliasing、alignment 和生命週期規則；能不用就不用。
  - C++ 風格轉型 (T)x 太模糊，可能偷偷做 const_cast 或 reinterpret_cast；教材應優先使用具名 cast 表達意圖。
  - 轉型陷阱通常不是語法錯，而是把物件真實型別、const 屬性、alignment 或生命週期假設錯。
  - downcast 若沒有 polymorphic base 和 dynamic_cast 檢查，很容易把 base 指標當成錯誤 derived 使用。
  - 能用 virtual function、variant、visitor 或 overload 解決的問題，通常比到處 cast 更安全。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Cast 常見陷阱
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 C 風格轉型 (T)x 被視為不良實踐？
//     答：因為它是【一個語法、多種語意】。C++ 標準規定 (T)x 會依序嘗試：
//         const_cast → static_cast → static_cast + const_cast →
//         reinterpret_cast → reinterpret_cast + const_cast，取【第一個能成立的】。
//         也就是說，你以為自己在寫一個安全的數值或繼承轉換，當它編不過時，
//         編譯器不會報錯，而是【靜默往下退化】，最後可能真的幫你做了
//         reinterpret_cast。加上它語法上難以搜尋、意圖不明確，所以現代風格指南
//         一律禁用。四種具名 cast 各自限縮了能力範圍，錯用會直接編譯失敗。
//     追問：怎麼在既有專案中找出它們？（-Wold-style-cast，或 clang-tidy 的
//           cppcoreguidelines-pro-type-cstyle-cast）
//
// 🔥 Q2. 什麼是 object slicing（物件切片）？怎麼避免？
//     答：把 derived 物件【以值（by value）】指派或傳遞給 base 型別時，只有 base
//         的部分被複製，derived 的成員與多型行為被「切掉」 — 得到的是一個純粹的
//         base 物件，vptr 指向 Base 的 vtable，不再有多型。避免方式：base 一律用
//         pointer 或 reference 傳遞；或把 base 的 copy constructor / copy
//         assignment 設成 protected 或 delete；或讓 base 是 abstract class。
//     追問：std::vector<Base> 塞 Derived 會怎樣？（每個元素都被 slice；正確做法是
//           std::vector<std::unique_ptr<Base>>）
//
// Q3. 把成員函式指標 reinterpret_cast 成一般函式指標可以嗎？
//     答：【不行，是 UB】。成員函式指標與一般函式指標的 ABI 根本不同 — 前者必須
//         帶著 this、而且在有 virtual / 多重繼承時它甚至不是單一位址（實作上常是
//         含偏移量的結構）。想把「物件 + 成員函式」變成可呼叫物件，正解是 lambda
//         捕獲物件、std::bind，或 std::mem_fn。
//
// ⚠️ 陷阱. for (int i = 0; i < v.size(); ++i) 有什麼問題？
//     答：v.size() 回傳 size_t（unsigned），比較時 int 會被【提升成 unsigned】。
//         單看這個迴圈通常還跑得對，真正的坑是同類寫法中的 unsigned 減法：
//         v.size() - 5 在 size() < 5 時不會得到負數，而是【環繞成一個超大正數】，
//         於是 「< 0」 的判斷永遠為假。解法：迴圈用 size_t，或 C++20 的
//         std::ssize(v) 取得有號長度，或明確 static_cast 成有號型別後再運算。
//     為什麼會錯：多數人腦中的模型是「反正都是整數，比大小不會有問題」，忽略了
//         【usual arithmetic conversions 會把有號的一方轉成無號】，而不是相反 —
//         負數因此變成巨大正數，而且這一切【沒有任何執行期錯誤】，只有開了
//         -Wsign-compare / -Wconversion 才看得到警告。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>

struct Animal { virtual ~Animal() = default; virtual std::string kind() const { return "Animal"; } };
struct Dog : Animal { std::string kind() const override { return "Dog"; } int bones = 5; };

// 接受「by value」的版本 — 會切片
static void useByValue(Animal a) {
    std::cout << "[byValue] kind=" << a.kind() << '\n';
}
// 接受 const ref — 不切片
static void useByRef(const Animal& a) {
    std::cout << "[byRef]   kind=" << a.kind() << '\n';
}

// 前置宣告：附加範例
static void demo_safe_iterate_polymorphic_container();
static void demo_unsigned_subtraction_wrap();

int main() {
    // ─────────────────────────────────────────────────────────
    // 陷阱 1：narrow conversion 演示
    // ─────────────────────────────────────────────────────────
    double d = 3.99;
    int i = static_cast<int>(d);
    std::cout << "[陷阱1] (int)3.99 = " << i << " (截斷)\n";

    // ─────────────────────────────────────────────────────────
    // 陷阱 2：signed/unsigned 比較
    // ─────────────────────────────────────────────────────────
    std::vector<int> v{1, 2, 3};
    int target = -1;
    if (static_cast<int>(v.size()) > target) {
        std::cout << "[陷阱2] 用 static_cast 後比較正確\n";
    }
    // 危險寫法（註解，自己看）：
    //   if (v.size() > target)  → target 被 promote 成 unsigned，邏輯錯誤

    // ─────────────────────────────────────────────────────────
    // 陷阱 3：Object Slicing
    // ─────────────────────────────────────────────────────────
    Dog dog;
    useByValue(dog);   // ❌ 印出 "Animal" — 切片發生
    useByRef(dog);     // ✅ 印出 "Dog" — 不切片

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：怎麼一眼判斷某 cast 該用哪種？
    //    A：「我在做什麼」自問清楚：
    //        * 改 const?     → const_cast
    //        * 父子轉換?     → static_cast 或 dynamic_cast（要 runtime 檢查就 dynamic）
    //        * 數值轉換?     → static_cast
    //        * 看 bit?       → bit_cast (C++20) 或 memcpy
    //        * 真的要硬轉?   → reinterpret_cast（且只透過 char*）
    //
    //  Q2：警告開到什麼等級才能抓住這些陷阱？
    //    A：建議：
    //        -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    //        -Wold-style-cast 抓 C-style cast
    //
    //  Q3：clang-tidy 哪些規則最有用？
    //    A：cppcoreguidelines-pro-type-cstyle-cast、
    //       cppcoreguidelines-pro-type-reinterpret-cast、
    //       cppcoreguidelines-narrowing-conversions、
    //       google-explicit-constructor。
    //
    demo_safe_iterate_polymorphic_container();
    demo_unsigned_subtraction_wrap();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — 多型容器避免切片
// =============================================================================
//  vector<Animal> 會切片：push_back(Dog{}) 會把 Dog 部分丟掉。
//  正確做法是 vector<unique_ptr<Animal>>，這也是工程上儲存「多型物件」的標準姿勢。
//  注意：使用 unique_ptr 時不要再用 dynamic_cast 隨意「拿走」 — 用 get() 取 raw ptr。
// =============================================================================
#include <memory>
static void demo_safe_iterate_polymorphic_container() {
    std::vector<std::unique_ptr<Animal>> zoo;
    zoo.push_back(std::make_unique<Dog>());
    zoo.push_back(std::make_unique<Animal>()); // 普通 Animal
    for (auto& a : zoo) {
        // 用 raw pointer 配 dynamic_cast 判斷是不是 Dog
        if (auto* d = dynamic_cast<Dog*>(a.get())) {
            std::cout << "[zoo] Dog with " << d->bones << " bones\n";
        } else {
            std::cout << "[zoo] plain " << a->kind() << '\n';
        }
    }
}

// =============================================================================
//  附加 2：實用範例 — unsigned 相減的「環繞」陷阱
// =============================================================================
//  size_t 相減若被減數較小，結果會「環繞」成超大正數（unsigned underflow）。
//  這是 vector 索引運算最常見的 bug。修正：把至少一邊用 static_cast 轉成
//  ptrdiff_t (signed)，比較和相減都安全。
// =============================================================================
static void demo_unsigned_subtraction_wrap() {
    std::vector<int> v{10, 20, 30};
    std::size_t i = 1;
    // 「壞寫法」：v.size() - 5 是 unsigned 大正數，比較容易誤判
    if (static_cast<std::ptrdiff_t>(v.size()) - 5 < 0) {
        std::cout << "[unsigned_wrap] signed 相減正確判斷為負\n";
    }
    // 「對位寫法」：直接用 ssize / 加法避免相減
    std::cout << "[unsigned_wrap] v[" << i << "] = " << v[i] << '\n';
}
