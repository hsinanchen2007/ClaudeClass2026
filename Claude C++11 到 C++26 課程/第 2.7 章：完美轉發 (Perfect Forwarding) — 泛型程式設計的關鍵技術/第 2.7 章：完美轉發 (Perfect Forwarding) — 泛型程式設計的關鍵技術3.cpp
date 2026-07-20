// =============================================================================
//  第 2.7 章 - 3  —  貪婪的轉發建構子：它會搶走複製建構（C++17 寫法）
// =============================================================================
//
// 【主題資訊 Information】
//   有問題的寫法（單參數轉發建構子，無約束）:
//       template<typename T> Widget(T&& name);
//   C++14/17 的約束寫法（本檔採用）:
//       template<typename T,
//                std::enable_if_t<!std::is_same_v<std::decay_t<T>, Widget>, int> = 0>
//       Widget(T&& name);
//
//   標準版本：std::enable_if / std::decay / std::is_same 是 C++11；
//             **std::enable_if_t / std::decay_t 是 C++14**；
//             **std::is_same_v 與 if constexpr 是 C++17**。
//             本檔用到 is_same_v 與 if constexpr，因此必須 C++17。
//             （實測 -std=c++11 -pedantic-errors 會報 enable_if_t has not been declared；
//               C++11 的等價寫法請見本章第 4 檔。）
//   標頭檔：<type_traits>（enable_if / decay / is_same）、<utility>（forward）
//
// 【詳細解釋 Explanation】
//
// 【1. 問題：轉發建構子是「貪婪重載」(greedy overload)】
//   單參數的轉發建構子 template<class T> C(T&&) 會吃下**任何**單一引數，
//   而且往往比編譯器產生的複製建構子更匹配。關鍵在於一次型別推導：
//
//     Widget w1("Hello");     T 推導為 const char(&)[6]  → 模板，沒爭議
//     Widget w3(w1);          w1 是**非 const** 的 Widget 左值
//                             → 模板推導 T = Widget&
//                             → 摺疊後參數型別是 Widget&
//                             → 候選是   Widget(Widget&)        ← 模板
//                                 vs     Widget(const Widget&)  ← 複製建構子
//
//   引數是 Widget&（非 const）。模板那個**完全不需要加 const**就能綁定，
//   複製建構子則需要一次 qualification conversion（加上 const）。
//   重載決議偏好「不需要轉換」的那個 → **模板勝出，複製建構子被搶走**。
//
//   這是完美轉發最有名的陷阱，因為它違反直覺：
//   「我明明寫了複製建構子，為什麼複製物件時沒被呼叫？」
//
// 【2. 為什麼 const 物件反而正常】
//     const Widget cw("X");
//     Widget w(cw);           // 引數是 const Widget&
//                             // 模板推導 T = const Widget& → Widget(const Widget&)
//                             // 複製建構子也是 Widget(const Widget&)
//                             // → 兩者「一樣好」→ 平手時**非模板勝出**
//   所以症狀是間歇性的：複製 const 物件正常、複製非 const 物件卻走模板。
//   這種「有時候對、有時候錯」的行為最難 debug。
//
// 【3. 沒有約束的話會發生什麼】
//   若模板建構子的初始化列表寫的是 name_(std::forward<T>(name))，
//   當 T = Widget& 時就等於要用一個 Widget 去建構 std::string —— 編譯失敗，
//   而且錯誤訊息會指向 std::string 的建構子，完全看不出真正的原因是
//   「複製建構被模板搶走了」。
//
//   本檔的 Gadget 刻意用 if constexpr 讓兩條路都能編譯，
//   目的是把「模板真的贏了」這件事**直接印出來**給你看，
//   而不是只丟一個難懂的編譯錯誤。這是本檔最重要的一段輸出。
//
// 【4. 解法：用 enable_if 把自己的型別排除掉】
//       template<typename T,
//                std::enable_if_t<!std::is_same_v<std::decay_t<T>, Widget>, int> = 0>
//       Widget(T&& name);
//
//   逐層拆解：
//     * std::decay_t<T>：把 T 的引用與 cv 修飾去掉。
//       T 可能被推導成 Widget&、const Widget&、Widget，
//       decay 之後統一變成 Widget，才能拿去比對。
//     * std::is_same_v<..., Widget>：判斷「這是不是在拿 Widget 建構 Widget」。
//     * ! + enable_if_t：是 Widget 就讓這個模板**推導失敗**。
//       SFINAE（Substitution Failure Is Not An Error）保證推導失敗只是
//       把這個候選從重載集合移除，不是編譯錯誤——於是複製建構子重新可見。
//
//   為什麼用 `int = 0` 這個匿名非型別參數，而不是 `typename = enable_if_t<...>`？
//   因為預設模板引數**不參與**函式簽名的區分。兩個只差在預設引數的模板
//   會被視為重複宣告。用非型別參數比較不會踩到這個問題；
//   兩種寫法在單一模板的情況下都能運作（第 4 檔用的是 typename 版）。
//
// 【5. C++20 之後：改用 concepts，可讀性好很多】
//       template<typename T>
//           requires (!std::same_as<std::decay_t<T>, Widget>)
//       Widget(T&& name);
//   或直接寫成簡寫函式模板。concepts 是 **C++20**（實測 -std=c++17
//   -pedantic-errors 編不過 <concepts>），語意與 enable_if 相同，
//   但錯誤訊息會直接說「不滿足哪個條件」，而不是吐出一整頁模板實例化堆疊。
//
// 【概念補充 Concept Deep Dive】
//
// ── 重載決議為什麼偏好模板：完整的比較過程 ──────────────────────────
//   對 Widget w3(w1)（w1 是非 const Widget 左值），候選函式是：
//
//     候選 A（模板實例）  Widget(Widget&)
//         引數 Widget 左值 → 參數 Widget&
//         需要的轉換：**identity（完全不用轉換）**
//
//     候選 B（複製建構子）Widget(const Widget&)
//         引數 Widget 左值 → 參數 const Widget&
//         需要的轉換：qualification adjustment（加 const）
//
//   標準規定先比「轉換序列的優劣」，identity 優於任何需要調整的序列，
//   所以 A 直接勝出——根本輪不到「非模板優先於模板」這條規則
//   （那條只在轉換序列**一樣好**的時候才拿出來用，也就是 const 物件那個情況）。
//
// ── 為什麼複製建構子不會被「隱藏」而是被「打敗」──────────────────
//   常見誤解是「模板把複製建構子蓋掉了」。實際上複製建構子一直都在候選集合裡，
//   只是每次比較都輸。這個區別很重要：因為它意味著只要讓模板**退出候選集合**
//   （SFINAE / concepts），複製建構子立刻就會被選中，不需要做別的事。
//
// ── decay_t 為什麼不可省 ────────────────────────────────────────────
//   若寫成 is_same_v<T, Widget>，當 T 推導為 Widget& 時
//   is_same_v<Widget&, Widget> 是 **false**，約束形同虛設，陷阱照樣發生。
//   非 const 左值正是最容易觸發問題的情況，也正是這種寫法擋不住的情況。
//   decay_t 會一併處理 Widget&、const Widget&、Widget 三種推導結果。
//
// ── 繼承會讓問題更隱蔽 ──────────────────────────────────────────────
//   decay_t + is_same_v 只擋 Widget 自己，不擋它的衍生類別。
//   若有 class Derived : public Widget，用 Derived 物件建構 Widget 時，
//   模板依然會贏過「切片複製」的 Widget(const Widget&)。
//   要一起擋掉衍生類別，得改用 std::is_base_of_v<Widget, std::decay_t<T>>。
//
// 【注意事項 Pay Attention】
//   1. 只要看到**單參數**的 template<class T> C(T&&) 建構子，就要立刻檢查
//      有沒有約束。這幾乎是 code review 的固定檢查項。
//   2. 症狀具有間歇性：const 物件正常、非 const 物件出錯。不要因為
//      「我測過複製沒問題」就放心——很可能你測的剛好是 const 版本。
//   3. is_same_v 一定要配 decay_t，否則約束會失效。
//   4. 有繼承關係時改用 is_base_of_v。
//   5. 完美轉發建構子同樣會搶走**移動**建構子（第 4 檔會把移動建構子也加進來
//      一起驗證）。
//   6. 版本：enable_if_t 是 C++14、is_same_v 與 if constexpr 是 C++17、
//      concepts 是 C++20。C++11 的等價寫法見第 4 檔。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】貪婪的轉發建構子
// ───────────────────────────────────────────────────────────────────────────
// ⚠️ 陷阱 Q1. 類別有 template<class T> C(T&&) 和 C(const C&)。
//             寫 C a("x"); C b(a); 時 b 走哪個建構子？
//     答：走**模板建構子**，不是複製建構子。a 是非 const 左值，
//         模板推導 T = C& 得到 C(C&)，綁定引數完全不需要轉換；
//         複製建構子 C(const C&) 則需要加上 const。
//         轉換序列較優者勝，模板贏。
//     為什麼會錯：多數人記得「非模板優先於模板」，就直接套用。
//         但那條規則只在兩者的**轉換序列一樣好**時才啟動；
//         這裡模板的轉換序列嚴格更好，根本輪不到那條規則。
//         把 a 改成 const C a; 就會走複製建構子——這正是它難抓的原因。
//
// 🔥 Q2. 怎麼修？enable_if 裡為什麼一定要 decay_t？
//     答：用 SFINAE 把自身型別排除：
//         template<class T, std::enable_if_t<
//             !std::is_same_v<std::decay_t<T>, C>, int> = 0> C(T&&);
//         decay_t 不可省，因為 T 會被推導成 C&（非 const 左值時），
//         而 is_same_v<C&, C> 是 false，約束就完全失效了——
//         偏偏非 const 左值正是最常觸發問題的情況。
//     追問：C++20 怎麼寫？→ requires (!std::same_as<std::decay_t<T>, C>)，
//           語意相同但錯誤訊息可讀得多。
//
// 🔥 Q3. 為什麼是「打敗」而不是「隱藏」複製建構子？這個區別重要在哪？
//     答：複製建構子始終在候選集合中，只是每次重載決議都輸給模板。
//         重要之處在於：只要讓模板透過 SFINAE / concepts **退出候選集合**，
//         複製建構子立刻自動被選中，不必額外做任何事。
//     追問：那 = delete 掉模板可以嗎？→ 不行。deleted function 仍會參與
//           重載決議，被選中後才報錯，等於把「靜默走錯路」變成「硬編譯錯誤」，
//           並沒有讓複製建構子勝出。
//
// ⚠️ 陷阱 Q4. 加了 enable_if 擋掉 Widget 自己，就一定安全了嗎？
//     答：不一定。decay_t + is_same_v 只擋 Widget 本身，擋不住衍生類別。
//         若 class Derived : public Widget，用 Derived 物件去建構 Widget 時，
//         模板（T = Derived&）依然贏過 Widget(const Widget&)。
//         要一併擋掉就得用 std::is_base_of_v<Widget, std::decay_t<T>>。
//     為什麼會錯：以為「排除自己」等於「排除所有同族型別」，
//         忽略了衍生類別到基底類別的轉換也是一種轉換，
//         模板的 identity 綁定依然更優。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

// =============================================================================
// 【錯誤示範】Gadget：沒有任何約束的轉發建構子
//
// 這個類別可以編譯、可以執行——問題不在於編不過，而在於它會**默默地**
// 選錯建構子。這比編譯錯誤更危險。
//
// 為了讓「模板贏了」這件事能被印出來（而不是變成一個難懂的編譯錯誤），
// 這裡用 if constexpr（C++17）讓 T = Gadget& 那條路也能編譯。
// 真實程式碼裡通常只寫 name_(std::forward<T>(n))，於是 T = Gadget& 時
// 就會變成「拿 Gadget 建構 std::string」的編譯錯誤。
// =============================================================================
class Gadget {
    std::string name_;

public:
    template<typename T>
    Gadget(T&& n) {
        if constexpr (std::is_same_v<std::decay_t<T>, Gadget>) {
            // T = Gadget& 會走到這裡 —— 代表模板搶走了複製建構
            name_ = n.name_;
            std::cout << "  [Gadget] ⚠️ 模板建構子 (T = Gadget&) 勝出"
                         " —— 複製建構子被搶走了！\n";
        } else {
            name_ = std::string(std::forward<T>(n));
            std::cout << "  [Gadget] 模板建構子（字串）name=" << name_ << "\n";
        }
    }

    Gadget(const Gadget& o) : name_(o.name_) {
        std::cout << "  [Gadget] 複製建構子 name=" << name_ << "\n";
    }
};

// =============================================================================
// 【正確示範】Widget：用 enable_if 把 Widget 自己排除在模板之外
// =============================================================================
class Widget {
    std::string name_;

public:
    // 使用 enable_if 排除 Widget 本身，避免模板搶走複製/移動建構子
    template<typename T,
             std::enable_if_t<!std::is_same_v<std::decay_t<T>, Widget>, int> = 0>
    Widget(T&& name) : name_(std::forward<T>(name)) {
        std::cout << "  [Widget] 模板建構子 name=" << name_ << "\n";
    }

    // 有了上面的約束，這個複製建構子才真的會被選中
    Widget(const Widget& other) : name_(other.name_) {
        std::cout << "  [Widget] 複製建構子 name=" << name_ << "\n";
    }
};

int main() {
    std::cout << "=== 錯誤示範：Gadget 沒有約束 ===\n";
    Gadget g1("Hello");             // T = const char(&)[6] → 模板，正常
    const Gadget cg("Const");       // 同上

    std::cout << "\n  -- Gadget g2(cg)：cg 是 const 左值 --\n";
    Gadget g2(cg);                  // 轉換序列平手 → 非模板勝出 → 複製建構子 ✅

    std::cout << "\n  -- Gadget g3(g1)：g1 是非 const 左值（陷阱在此）--\n";
    Gadget g3(g1);                  // 模板 T = Gadget& 轉換序列更優 → 模板勝出 ❌

    std::cout << "\n=== 正確示範：Widget 用 enable_if 約束 ===\n";
    Widget w1("Hello");             // T = const char(&)[6]，不是 Widget → 模板 ✅

    const Widget w2(w1);            // 複製建構子 ✅
    std::cout << "  -- Widget w3(w1)：同樣是非 const 左值 --\n";
    Widget w3(w1);                  // enable_if 把模板踢出候選 → 複製建構子 ✅

    // 避免未使用變數警告
    (void)g2; (void)g3; (void)w2; (void)w3;

    std::cout << "\n=== 結論 ===\n";
    std::cout << "  Gadget: 非 const 左值 → 模板搶走複製建構（靜默走錯路）\n";
    std::cout << "  Widget: 非 const 左值 → 複製建構子正確被呼叫\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.7 章：完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術3.cpp" -o pf3

// === 預期輸出 ===
// === 錯誤示範：Gadget 沒有約束 ===
//   [Gadget] 模板建構子（字串）name=Hello
//   [Gadget] 模板建構子（字串）name=Const
//
//   -- Gadget g2(cg)：cg 是 const 左值 --
//   [Gadget] 複製建構子 name=Const
//
//   -- Gadget g3(g1)：g1 是非 const 左值（陷阱在此）--
//   [Gadget] ⚠️ 模板建構子 (T = Gadget&) 勝出 —— 複製建構子被搶走了！
//
// === 正確示範：Widget 用 enable_if 約束 ===
//   [Widget] 模板建構子 name=Hello
//   [Widget] 複製建構子 name=Hello
//   -- Widget w3(w1)：同樣是非 const 左值 --
//   [Widget] 複製建構子 name=Hello
//
// === 結論 ===
//   Gadget: 非 const 左值 → 模板搶走複製建構（靜默走錯路）
//   Widget: 非 const 左值 → 複製建構子正確被呼叫
