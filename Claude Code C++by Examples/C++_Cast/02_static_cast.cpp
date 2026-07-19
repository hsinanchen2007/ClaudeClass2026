// =============================================================================
//  02_static_cast.cpp  —  static_cast 詳解
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/static_cast
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、static_cast 能做什麼？                                 │
//  └────────────────────────────────────────────────────────────┘
//
//   1. 數值轉換   double ↔ int、float ↔ long、enum class ↔ int...
//   2. 上轉型     Derived* → Base*（同 implicit）
//   3. 下轉型     Base* → Derived*（你保證 base 真指向 Derived，沒 RTTI 檢查）
//   4. void* 轉換 void* ↔ T*（搭配 malloc / 老 C API）
//   5. enum class ↔ underlying type
//   6. 顯式呼叫單參數建構子（function-style cast 寫法的等價）
//
//  static_cast 不能做的：
//   * 不能脫掉 const（要用 const_cast）
//   * 不能把不相干型別的指標互轉（要用 reinterpret_cast）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、編譯期檢查 — 跟 C-style cast 的關鍵差異                │
//  └────────────────────────────────────────────────────────────┘
//
//  static_cast 會做基本型別系統檢查 — 「明顯違反語意」的會被擋下。例如：
//
//      const int* p = ...;
//      int* q = static_cast<int*>(p);    // ❌ 編譯錯（不能脫 const）
//      int* q = (int*)p;                  // ⚠️ 通過（C-style 偷偷做了 const_cast）
//
//  C-style 把所有 cast 機制混在一起 → 沒注意到「脫 const」這件大事。
//  static_cast 會強制你明寫該用 const_cast，邏輯更明確。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：數值轉換 + narrow conversion 警告
//   * Demo 2：上轉型（safe）vs 下轉型（你要負責正確）
//   * Demo 3：enum class ↔ underlying
//   * Demo 4：void* 轉換（malloc 場景）
// =============================================================================

/*
補充筆記：static cast
  - static_cast 適合語言規則允許且你想明確標出的轉換，例如數值、enum、已知繼承方向。
  - 它不做 runtime 型別檢查，base 轉 derived 若真實物件不是 derived 會出事。
  - 數值窄化轉換可能截斷或超出範圍，cast 只表示你接受這個風險，不會讓值安全。
  - static cast 要先分清楚四種命名轉型：static_cast、dynamic_cast、const_cast、reinterpret_cast 各自解決不同問題。
  - static_cast 用於明確且語意合理的轉換，例如數值轉型、base/derived 已知方向；它不做執行期型別檢查。
  - dynamic_cast 用於 polymorphic base 上的安全向下轉型；失敗時 pointer 得到 nullptr，reference 會丟 std::bad_cast。
  - const_cast 只能調整 const/volatile 屬性；若原物件本來就是 const，移除 const 後修改是未定義行為。
  - reinterpret_cast 是低階重新解讀位元或位址，最容易違反 aliasing、alignment 和生命週期規則；能不用就不用。
  - C++ 風格轉型 (T)x 太模糊，可能偷偷做 const_cast 或 reinterpret_cast；教材應優先使用具名 cast 表達意圖。
*/
#include <cstdlib>
#include <iostream>
#include <type_traits>
#include <vector>

struct Animal { virtual ~Animal() = default; virtual void say() const = 0; };
struct Dog : Animal { void say() const override { std::cout << "woof\n"; } };
struct Cat : Animal { void say() const override { std::cout << "meow\n"; } };

enum class Color : unsigned { Red = 1, Green = 2, Blue = 4 };

// 前置宣告：附加範例
static void demo_enum_to_index_table();
static void demo_size_t_signed_compare();

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：數值轉換
    // ─────────────────────────────────────────────────────────
    double d = 3.9;
    int i = static_cast<int>(d);                     // 3 (向 0 截斷)
    std::cout << "[Demo1] double 3.9 -> int " << i << '\n';

    long long big = 5'000'000'000LL;
    // 術語更正（原版把三個層級混為一談）：
    //   超出目標型別範圍的【整數轉換】不是 undefined behavior。
    //   ・C++20 之前：implementation-defined（實作定義，編譯器要說明自己怎麼做）
    //   ・C++20 起  ：完全 well-defined —— 標準強制二補數，行為就是模數環繞
    //   本機實測 5'000'000'000 → 705032704，正好是 5e9 - 2^32，就是模數環繞。
    int chopped = static_cast<int>(big);
    std::cout << "[Demo1] truncate 5e9 -> int " << chopped
              << " (超出 int 範圍：C++20 起是定義良好的模數環繞，不是 UB；\n"
                 "          但『數值被無聲截斷』依然是 bug 溫床)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：上轉型 vs 下轉型
    // ─────────────────────────────────────────────────────────
    Dog dog;
    Animal* up = static_cast<Animal*>(&dog);         // 上轉型：永遠安全
    up->say();                                        // 動態派發到 Dog::say

    // 下轉型：你保證 up 真的指向 Dog，沒 runtime 檢查
    Dog* down_ok = static_cast<Dog*>(up);
    down_ok->say();

    // ⚠️ 把「實際上是 Dog」的物件強轉成 Cat*：編譯通過，但轉型本身就不滿足
    //    下轉型的前置條件 → undefined behavior（UBSan 會報 downcast of address ...
    //    which does not point to an object of type 'Cat'）。
    //    預設【不執行】，免得一般 ./example 就在跑 UB；要看就明確打開：
    //        g++ -std=c++17 -DDEMONSTRATE_UB -fsanitize=undefined 02_static_cast.cpp
#ifdef DEMONSTRATE_UB
    Animal* mystery = &dog;
    Cat* down_bad = static_cast<Cat*>(mystery);
    (void)down_bad;
    std::cout << "[Demo2] static_cast 對 mystery 強轉成 Cat — 編譯過、runtime UB\n";
#else
    std::cout << "[Demo2] (錯誤下轉型 Dog→Cat 是 UB，預設不執行；\n"
                 "        用 -DDEMONSTRATE_UB -fsanitize=undefined 可看 UBSan 抓到它)\n";
#endif
    // 真正安全要用 dynamic_cast（見 03 號檔）

    // ─────────────────────────────────────────────────────────
    // Demo 3：enum class ↔ underlying type
    //   enum class 不會自動轉成整數（這是它比普通 enum 的優勢），要用
    //   static_cast 顯式轉換。
    // ─────────────────────────────────────────────────────────
    Color c = Color::Green;
    auto raw = static_cast<std::underlying_type_t<Color>>(c);
    std::cout << "[Demo3] Color::Green raw = " << raw << '\n';

    // 反向也成立（要小心值是不是合法 enum 範圍）
    Color back = static_cast<Color>(4);
    std::cout << "[Demo3] from 4 -> "
              << (back == Color::Blue ? "Blue" : "?") << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 4：void* 轉換 — 跟 malloc 互動的標準姿勢
    // ─────────────────────────────────────────────────────────
    void* mem = std::malloc(sizeof(int) * 4);
    int* arr = static_cast<int*>(mem);              // void* -> int*
    for (int k = 0; k < 4; ++k) arr[k] = k * k;
    std::cout << "[Demo4] arr =";
    for (int k = 0; k < 4; ++k) std::cout << ' ' << arr[k];
    std::cout << '\n';
    std::free(mem);

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：narrow conversion 怎麼比較安全？
    //    A：
    //      (a) 用 static_cast 至少明寫意圖，方便 review
    //      (b) C++11 起 brace init 會擋下 narrow conversion：
    //            int x{3.9};   // ❌ 編譯錯（會丟資料）
    //          所以新程式碼建議盡量用 brace init 而不是 = 或 (
    //
    //  Q2：static_cast 會呼叫建構子嗎？
    //    A：會，如果目標是 class type 且只有單參數建構子（且該建構子未標
    //       explicit）。常見：static_cast<std::string>("hello")。
    //
    //  Q3：static_cast<bool>(ptr) 安全嗎？
    //    A：是。比較標準的寫法是 (ptr != nullptr)，但 static_cast<bool> 也
    //       明確不模糊。
    //
    demo_enum_to_index_table();
    demo_size_t_signed_compare();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — enum class 當索引查表
// =============================================================================
//  工作上常見：用 enum class 標記狀態 / 種類，並把它當 array 的 index。
//  enum class 不能隱式轉成整數，必須 static_cast 明寫。
//  好處：表格化 dispatch 比 switch 更密、查表 O(1)。
//  陷阱：enum class 值要連續、從 0 開始，否則需要做映射或 if 防呆。
// =============================================================================
enum class HttpMethod : unsigned { GET = 0, POST = 1, PUT = 2, DEL = 3 };
static void demo_enum_to_index_table() {
    const char* names[] = {"GET", "POST", "PUT", "DELETE"};
    HttpMethod m = HttpMethod::POST;
    auto idx = static_cast<unsigned>(m);
    std::cout << "[enum->index] HttpMethod " << names[idx] << '\n';
}

// =============================================================================
//  附加 2：實用範例 — 跟 size_t 做 signed/unsigned 比較
// =============================================================================
//  v.size() 是 size_t（unsigned）；跟 int 變數比較會把 int 隱式 promote 成
//  unsigned，負數會變成超大正數造成邏輯錯誤。static_cast 把 size 明寫成 int
//  能避開這個坑（前提是你確定不會超過 INT_MAX）。
//  C++20 起有 std::ssize 直接回傳 signed size。
// =============================================================================
static void demo_size_t_signed_compare() {
    int target = -1;
    std::vector<int> v{1, 2, 3, 4, 5};
    bool ok = static_cast<int>(v.size()) > target;
    std::cout << "[size_t_cmp] (signed) size > -1? " << ok << " (應為 1)\n";
    // ❌ 危險寫法：if (v.size() > target) → target 變超大正數，結果可能反直覺
}
