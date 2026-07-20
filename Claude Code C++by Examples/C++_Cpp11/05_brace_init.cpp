// =============================================================================
//  05_brace_init.cpp  —  {} 統一初始化 (C++11)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/list_initialization
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼要 brace init？                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  C++98 有四五種初始化方式，意義微妙不同：
//
//      int x = 5;           // copy initialization
//      int y(5);            // direct initialization
//      int arr[3] = {1,2,3};// aggregate init for array
//      MyClass obj(1, 2);   // ctor call
//      MyClass obj2 = MyClass(1, 2);  // also ctor
//
//  常見問題：「Most Vexing Parse」 — Widget w(); 是宣告函式，不是 default
//  建構物件。
//
//  C++11 引入 {} 統一初始化（uniform initialization）：
//
//      int x{5};
//      int arr[3]{1, 2, 3};
//      MyClass obj{1, 2};
//      Widget w{};            // ✅ default 建構，不是函式宣告
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、防 narrow conversion                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  {} 會擋下「縮窄轉換」 — 隱式丟資訊的轉換編譯失敗：
//
//      int  a = 3.99;        // OK: 隱式截斷成 3
//      int  b{3.99};         // ❌ 編譯錯（C++11 brace init 規定）
//      int  c{3};            // OK
//      char d{2000};         // ❌ 縮窄
//
//  這個保護是 brace init 最大的好處之一。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、跟 initializer_list 的優先順序                        │
//  └────────────────────────────────────────────────────────────┘
//
//  類別有 initializer_list ctor 時，{} 會優先呼叫它：
//
//      std::vector<int> v1(3, 5);    // 三個 5：[5,5,5]
//      std::vector<int> v2{3, 5};    // 兩個元素：[3,5] — 走 initializer_list！
//
//  小心！這是 brace init 唯一容易踩雷的地方。已知容器有 initializer_list
//  ctor 的情況，要呼叫「不走 init list」的版本，用 () 而不是 {}。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：四種型別都用 {}
//   * Demo 2：narrow conversion 被擋下（用註解保留錯誤示範）
//   * Demo 3：vector{3, 5} vs vector(3, 5) 差異
// =============================================================================

/*
補充筆記：brace_init
  - brace_init 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - 大括號初始化可阻止 narrowing conversion，例如 int x{3.14} 會編譯失敗。
  - 若型別有 initializer_list 建構子，大括號可能優先選它，vector<int>{10} 和 vector<int>(10) 意義不同。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】brace init（統一初始化）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 統一初始化 `{}` 帶來哪些好處？
//     答：三點。(1) 語法統一，聚合、容器、成員初始化共用同一套寫法；(2) 禁止
//         narrowing conversion，`int b{3.99}` 直接編譯錯，而 `int a = 3.99`
//         只會靜默截斷成 3；(3) 解決 most vexing parse——`Widget w{};` 明確是
//         建構物件，`Widget w();` 卻會被解析成函數宣告。
//     追問：narrowing 只看型別還是也看值？（若來源是編譯期常數且該值能被目標型別
//         精確表示就放行，所以 `char c{65}` 合法、`char c{2000}` 不合法）
//
// Q2. `W c{};` 與 `W d({});` 差在哪？（W 有 initializer_list 建構子）
//     答：`W c{}` 呼叫的是「預設建構子」，不是傳一個空的 initializer_list；
//         要真的傳空 list 必須寫成 `W d({});`。空大括號在這裡是特例：它一律
//         優先解讀為預設建構，不會退化成長度 0 的 list。本機 g++ 已驗證兩者
//         走的是不同建構子。
//
// ⚠️ 陷阱. `std::vector<int> a{3, 5};` 與 `std::vector<int> b(3, 5);` 各是什麼？
//     答：a 是「兩個元素 [3, 5]」，b 是「三個元素 [5, 5, 5]」（見 Demo 3）。
//         只要類別有 initializer_list 建構子，`{}` 就強烈優先選它——強到即使
//         另一個建構子是精準匹配也照樣輸。本機驗證：類別同時有 W(int, int) 與
//         W(std::initializer_list<int>) 時，`W b{1, 2}` 選中的是 init_list 版。
//     為什麼會錯：多數人把 `{}` 當成「跟 () 一樣、只是換個括號」。實際上它多了
//         一條凌駕一切的優先規則。補充一個容易被反向記錯的邊界：若所有
//         initializer_list 建構子都「不可行」（型別根本轉不過去），才會退回考慮
//         一般建構子。實務折衷：預設用 {}，但「指定 size + 初值」這種語意一律用 ()。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <unordered_set>
#include <vector>

struct Point { int x, y; };

class Widget {
public:
    Widget() { std::cout << "  Widget()\n"; }
    Widget(int) { std::cout << "  Widget(int)\n"; }
};

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：各種型別都用 {}
    // ─────────────────────────────────────────────────────────
    int          x{42};
    double       d{3.14};
    int          arr[3]{10, 20, 30};
    Point        p{1, 2};            // aggregate init
    Widget       w{};                // ✅ default 建構，不是函式宣告
    std::vector<int> v{1, 2, 3, 4};  // 走 initializer_list ctor

    std::cout << "[Demo1] x=" << x
              << " d=" << d
              << " p=(" << p.x << "," << p.y << ")"
              << " v.size=" << v.size() << '\n';
    (void)arr; (void)w;

    // ─────────────────────────────────────────────────────────
    // Demo 2：narrow conversion
    //   下面這幾行如果取消註解會編譯失敗
    // ─────────────────────────────────────────────────────────
    // int    bad1{3.99};      // ❌ double → int (narrowing)
    // char   bad2{2000};      // ❌ int → char (narrowing)
    // int    bad3{(long long)1};  // 取決於平台是否窄化
    int safe1{3};
    int safe2 = static_cast<int>(3.99);   // 顯式轉換 OK
    std::cout << "[Demo2] safe1=" << safe1 << " safe2=" << safe2 << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：vector{3,5} vs vector(3,5)
    //   {} 走 initializer_list ctor，產出兩元素 [3,5]
    //   () 走 (count, value) ctor，產出三元素 [5,5,5]
    // ─────────────────────────────────────────────────────────
    std::vector<int> a{3, 5};
    std::vector<int> b(3, 5);
    std::cout << "[Demo3] a (brace) =";
    for (int v3 : a) std::cout << ' ' << v3;
    std::cout << "  size=" << a.size() << '\n';
    std::cout << "[Demo3] b (paren) =";
    for (int v3 : b) std::cout << ' ' << v3;
    std::cout << "  size=" << b.size() << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：要不要全面改用 {}？
    //    A：Effective Modern C++ Item 7：「優先 {}，但對 vector / 其他有
    //       initializer_list ctor 的類別要小心」。實務多數人折衷：能用 {}
    //       就用，但 vector 的「指定 size + value」一律用 ()。
    //
    //  Q2：「{}」對自訂 class 怎麼運作？
    //    A：先看是否有 initializer_list ctor → 有就走那條；
    //       沒有則找 best matching 的「普通 ctor」；
    //       都不行就 aggregate init（如 Point 那種 plain struct）。
    //
    //  Q3：「{}」對 array 也有特殊？
    //    A：陣列 {} 是「aggregate initialization」 — 不夠的元素會 zero-init：
    //         int arr[5]{1, 2};   // = {1, 2, 0, 0, 0}
    //         int arr[5]{};       // 全 0
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 217. Contains Duplicate
    //   題意：判斷 vector 內有沒有重複元素。
    //   為什麼放這？unordered_set 用 brace init 初始化 + insert 回傳
    //                pair<iterator, bool>，是 brace init 在 STL 的典型用法。
    // ─────────────────────────────────────────────────────────
    auto containsDup = [](const std::vector<int>& nums) {
        std::unordered_set<int> seen{};     // ✅ 空 set，用 {} 明寫「我故意空」
        for (int x : nums) {
            if (!seen.insert(x).second) return true;
        }
        return false;
    };
    std::cout << std::boolalpha;
    std::cout << "[LC217] {1,2,3,1} => " << containsDup({1, 2, 3, 1}) << '\n';  // true
    std::cout << "[LC217] {1,2,3,4} => " << containsDup({1, 2, 3, 4}) << '\n';  // false

    // ─────────────────────────────────────────────────────────
    // 實用範例：點/矩形這類 aggregate struct 用 brace init 最自然
    //   工作上常見：座標、顏色、設定結構直接 {} 構造
    // ─────────────────────────────────────────────────────────
    struct Rect { int x, y, w, h; };
    Rect r1{0, 0, 800, 600};                 // aggregate init
    Rect r2{};                                // 全 0 — zero init
    std::cout << "[Demo5] r1=(" << r1.x << "," << r1.y
              << "," << r1.w << "," << r1.h << ")\n";
    std::cout << "[Demo5] r2=(" << r2.x << "," << r2.y
              << "," << r2.w << "," << r2.h << ")  // 全 0\n";

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 05_brace_init.cpp -o 05_brace_init

// === 預期輸出 ===
//   Widget()
// [Demo1] x=42 d=3.14 p=(1,2) v.size=4
// [Demo2] safe1=3 safe2=3
// [Demo3] a (brace) = 3 5  size=2
// [Demo3] b (paren) = 5 5 5  size=3
// [LC217] {1,2,3,1} => true
// [LC217] {1,2,3,4} => false
// [Demo5] r1=(0,0,800,600)
// [Demo5] r2=(0,0,0,0)  // 全 0
