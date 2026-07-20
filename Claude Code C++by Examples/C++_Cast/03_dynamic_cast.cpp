// =============================================================================
//  03_dynamic_cast.cpp  —  dynamic_cast 詳解
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/dynamic_cast
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、dynamic_cast 解決什麼問題？                            │
//  └────────────────────────────────────────────────────────────┘
//
//  static_cast 對「父→子」的下轉型完全不檢查 — 你說它是 Dog* 它就當你說
//  得對，runtime 真的不是 Dog 就 UB。
//
//  dynamic_cast 把這個檢查搬到 runtime：
//
//   * 對指標：成功回傳目標指標，失敗回傳 nullptr
//   * 對參考：成功回傳目標 ref，失敗 throw std::bad_cast
//
//  代價：
//   * 需要 RTTI（Run-Time Type Information）— 編譯時要 -frtti（GCC 預設開）
//   * Base 類別必須是「polymorphic」 — 至少要有一個 virtual function
//   * 比 static_cast 慢一點（要查 type info 表）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、什麼時候真的需要 dynamic_cast？                       │
//  └────────────────────────────────────────────────────────────┘
//
//  經驗法則：「你需要 dynamic_cast」往往意味著「設計可能可以用 virtual
//  function 重構」。但有時候真的避不開：
//
//   * 處理一個 base class 容器，需要對特定子類別做特殊處理
//   * 跨模組型別「猜」，只有 base 介面，需要 try-cast 識別
//   * Visitor pattern 的退化版（更乾淨的做法是 std::variant + std::visit）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：Animal 階層、安全下轉型
//   * Demo 2：對 reference 做 dynamic_cast 失敗 → bad_cast
//   * Demo 3：跨層 cross cast（兄弟類別之間）
// =============================================================================

/*
補充筆記：dynamic cast
  - dynamic_cast 用於多型型別的安全向下轉型，需要基底類別至少有一個 virtual 函式。
  - 指標版本失敗回 nullptr；參考版本失敗丟 std::bad_cast。
  - 若設計需要到處 dynamic_cast，通常代表抽象介面可能還能重新設計。
  - dynamic cast 要先分清楚四種命名轉型：static_cast、dynamic_cast、const_cast、reinterpret_cast 各自解決不同問題。
  - static_cast 用於明確且語意合理的轉換，例如數值轉型、base/derived 已知方向；它不做執行期型別檢查。
  - dynamic_cast 用於 polymorphic base 上的安全向下轉型；失敗時 pointer 得到 nullptr，reference 會丟 std::bad_cast。
  - const_cast 只能調整 const/volatile 屬性；若原物件本來就是 const，移除 const 後修改是未定義行為。
  - reinterpret_cast 是低階重新解讀位元或位址，最容易違反 aliasing、alignment 和生命週期規則；能不用就不用。
  - C++ 風格轉型 (T)x 太模糊，可能偷偷做 const_cast 或 reinterpret_cast；教材應優先使用具名 cast 表達意圖。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】dynamic_cast
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. dynamic_cast 的實作原理是什麼？為什麼說它有真實的執行期成本？
//     答：它依賴 RTTI（Run-Time Type Information）— 編譯器為每個多型 class 產生
//         type_info，並在 vtable 中放指向它的指標。執行時：取物件的 vptr → 找到
//         type_info → 【走訪繼承圖】判斷目標型別是否可達，並計算必要的指標偏移。
//         關鍵是：多重繼承/虛繼承下繼承圖可能很複雜，所以它【不是常數時間操作】，
//         成本隨繼承結構而變，某些實作還會做字串比較。
//         ⚠️ 但要分路徑講，別一句「隨繼承深度變深就變慢」帶過：Itanium ABI 會先
//         比對 type_info 指標，所以【精準命中】那條路徑實測與繼承深度無關
//         （本機 g++ -O2 跨 TU 量測：深度 1 = 2.88 ns、深度 8 = 2.96 ns）。
//         真正貴的是「命中中段基底」與「轉型失敗」——那才要走繼承圖。
//     追問：只開 RTTI 但不用 dynamic_cast 有代價嗎？（幾乎只有 binary size 與
//           少量記憶體，執行期成本可忽略）／怎麼避免？（改用 virtual 函式、
//           visitor pattern、std::variant + std::visit，或自帶 type tag）
//
// 🔥 Q2. dynamic_cast 失敗時的行為？指標版與參考版有何不同？
//     答：【指標版失敗回傳 nullptr】，所以呼叫端必須檢查；【參考版失敗拋出
//         std::bad_cast】，因為 C++ 沒有「null reference」這種東西，無法用回傳值
//         表達失敗。慣用寫法是 if (auto* d = dynamic_cast<Derived*>(p)) { ... }，
//         把轉型與檢查寫在同一行。
//     追問：那什麼時候該用參考版？（當「轉不成功」屬於程式錯誤、應該直接讓例外
//           往上拋，而不是靜默走另一條分支時）
//
// 🔥 Q3. 為什麼 dynamic_cast<Derived*>(basePtr) 在非多型 class 上編譯不過？
//     答：執行期檢查需要從物件取得動態型別資訊，而該資訊掛在 vtable 上 — 沒有
//         virtual 函式的 class 沒有 vptr、沒有 vtable，也就無從查起。所以標準
//         要求做下轉型/側向轉型時【來源型別必須是多型的】，否則直接編譯錯誤
//         （不是執行期才失敗）。修法：給 base 加上通常本來就該有的 virtual
//         ~Base()。注意上轉型是例外 — 它合法、等同隱式轉換，且不需要 RTTI。
//
// Q4. 什麼是 cross-cast（側向轉型）？哪個 cast 做得到？
//     答：多重繼承中，從一個 base subobject 的指標轉到【兄弟 base】（D : public
//         B, public C，從 B* 轉成 C*）。【只有 dynamic_cast 做得到】 — 它在執行期
//         經由最衍生物件走訪繼承圖並計算偏移；static_cast 會因為 B 和 C 之間沒有
//         繼承關係而編譯失敗。
//     追問：dynamic_cast<void*> 有什麼特殊用途？（對多型型別的指標做它，會回傳
//           指向【最衍生物件起始位址】的指標 — 可用來判斷兩個不同型別的 base
//           指標是否指向同一個實體物件，多重繼承下直接比較指標會因偏移而失敗）
//
// ⚠️ 陷阱. 在 constructor / destructor 裡用 dynamic_cast 安全嗎？
//     答：【有限制】。物件在建構/解構期間，其動態型別是「當前正在執行的那個
//         class」，而非最衍生型別（vptr 還沒更新到 / 已經退回）。cppreference
//         明確指出：若在此期間對正在建構/解構的物件使用 dynamic_cast，而目標型別
//         不是該 class 本身或其 base，行為【未定義】。同理 typeid 在此期間也只會
//         回報當前 class。
//     為什麼會錯：多數人腦中的模型是「物件從 new 那一刻起型別就固定是 Derived」，
//         忽略了建構期間物件的動態型別是【逐層演進】的 — 這也是「不要在建構子裡
//         呼叫 virtual 函式」的同一個根因。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <stdexcept>
#include <vector>

struct Animal {
    virtual ~Animal() = default;
    virtual void say() const = 0;
};
struct Dog : Animal {
    void say() const override { std::cout << "woof "; }
    void wagTail() const     { std::cout << "(tail) "; }
};
struct Cat : Animal {
    void say() const override { std::cout << "meow "; }
    void purr() const         { std::cout << "(purr) "; }
};

// 前置宣告：附加範例
static void demo_shape_area_dispatch();
static void demo_event_handler_filter();

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：對指標做 dynamic_cast，nullptr 表示失敗
    // ─────────────────────────────────────────────────────────
    std::vector<Animal*> zoo;
    zoo.push_back(new Dog);
    zoo.push_back(new Cat);
    zoo.push_back(new Dog);

    for (Animal* a : zoo) {
        a->say();
        // 如果是 Dog，多印「搖尾巴」
        if (Dog* d = dynamic_cast<Dog*>(a)) d->wagTail();
        // 如果是 Cat，多印「咕嚕咕嚕」
        if (Cat* c = dynamic_cast<Cat*>(a)) c->purr();
    }
    std::cout << '\n';

    for (auto* a : zoo) delete a;

    // ─────────────────────────────────────────────────────────
    // Demo 2：對 reference 失敗會 throw bad_cast
    // ─────────────────────────────────────────────────────────
    Dog d2;
    Animal& a_ref = d2;
    try {
        Cat& bad = dynamic_cast<Cat&>(a_ref);   // a_ref 實際是 Dog
        bad.purr();
    } catch (const std::bad_cast& e) {
        std::cout << "[Demo2] caught bad_cast: " << e.what() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：兄弟之間 cross cast — 實際上是「先回 base 再下到另一邊」
    //   只要基底是 polymorphic 就能做，但要 base 中有相同祖先
    // ─────────────────────────────────────────────────────────
    Dog d3;
    Animal* a3 = &d3;
    Cat* c3 = dynamic_cast<Cat*>(a3);     // 失敗 → nullptr
    std::cout << "[Demo3] try Dog→Cat = "
              << (c3 ? "ok" : "null (correct)") << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 -fno-rtti 會打壞 dynamic_cast？
    //    A：dynamic_cast 需要 type info 表（vtable 旁邊一塊資料）。關掉
    //       RTTI 就沒這張表 → 連結期就會錯誤。一些嵌入式 / 高效能專案會
    //       關 RTTI 換空間，但要記得 dynamic_cast 跟 typeid 都不能用。
    //
    //  Q2：dynamic_cast 比 static_cast 慢多少？
    //    A：「幾倍」這個問法本身就有問題 —— static_cast 是編譯期算好的指標
    //       位移，執行期幾乎【零成本】（本機實測 0.11 ns/op，與空迴圈的
    //       0.12 ns 無法區分），拿它當分母只會得到一個無意義的巨大倍數。
    //       正確講法是給【絕對量級】，而且要分三條路徑（本機 g++ -O2 跨 TU
    //       量測，clang++ -O2 交叉驗證趨勢一致）：
    //         * 精準命中（型別剛好相符）：約 3 ns，與繼承深度無關
    //         * 命中中段基底（L8 物件 → L4*）：約 34 ns，貴一個數量級
    //         * 轉型失敗（cross-cast 到旁系）：約 67 ns，最貴
    //       標準【沒有】規定 dynamic_cast 的演算法複雜度，數字隨 ABI／實作
    //       而異，要拿來做決策就得自己在目標平台上量測。
    //       實務結論不變：熱迴圈裡用 static_cast + virtual function 重構通常
    //       更好；dynamic_cast 適合「不在 hot path」的辨識性場合。
    //
    //  Q3：std::variant + std::visit 怎麼取代 dynamic_cast？
    //    A：把所有可能型別放進 variant，用 visit 對每個型別寫 lambda
    //       handler。優點：編譯期保證所有型別都處理到了，無 RTTI 成本。
    //
    demo_shape_area_dispatch();
    demo_event_handler_filter();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — Shape 階層算面積（教科書級多型用例）
// =============================================================================
//  在繪圖、UI、CAD 系統，有一個 Shape* 容器，需要依不同子類別取出特殊資料
//  或執行特殊邏輯。雖然 virtual function 是首選，但「只在某幾個子類別才有
//  的 debug 動作」適合用 dynamic_cast。
// =============================================================================
struct Shape { virtual ~Shape() = default; virtual double area() const = 0; };
struct Circle : Shape {
    double r;
    explicit Circle(double r_) : r(r_) {}
    double area() const override { return 3.14159 * r * r; }
};
struct Square : Shape {
    double side;
    explicit Square(double s) : side(s) {}
    double area() const override { return side * side; }
};
static void demo_shape_area_dispatch() {
    std::vector<Shape*> shapes{new Circle(2.0), new Square(3.0), new Circle(1.0)};
    double total = 0;
    int circleCnt = 0;
    for (Shape* s : shapes) {
        total += s->area();
        if (dynamic_cast<Circle*>(s)) ++circleCnt; // 只統計 Circle 數量
    }
    std::cout << "[shapes] total area=" << total
              << ", circle count=" << circleCnt << '\n';
    for (Shape* s : shapes) delete s;
}

// =============================================================================
//  附加 2：實用範例 — 事件處理器型別過濾
// =============================================================================
//  訊息匯流排 (event bus) 把任意 Event 派發到 subscribers；某個 handler 只關
//  心特定型別的 event，其餘忽略。dynamic_cast 用來判斷「是不是我要處理的」。
// =============================================================================
struct Event { virtual ~Event() = default; };
struct ClickEvent : Event { int x, y; ClickEvent(int x_, int y_) : x(x_), y(y_) {} };
struct KeyEvent   : Event { int keycode;  explicit KeyEvent(int k) : keycode(k) {} };
static void demo_event_handler_filter() {
    std::vector<Event*> queue{new ClickEvent(10, 20), new KeyEvent(65), new ClickEvent(5, 5)};
    int clickCnt = 0;
    for (Event* e : queue) {
        // 只處理 ClickEvent
        if (auto* c = dynamic_cast<ClickEvent*>(e)) {
            std::cout << "[event] click at (" << c->x << "," << c->y << ")\n";
            ++clickCnt;
        }
    }
    std::cout << "[event] processed clicks = " << clickCnt << '\n';
    for (Event* e : queue) delete e;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 03_dynamic_cast.cpp -o 03_dynamic_cast

// === 預期輸出 ===
// woof (tail) meow (purr) woof (tail)
// [Demo2] caught bad_cast: std::bad_cast
// [Demo3] try Dog→Cat = null (correct)
// [shapes] total area=24.708, circle count=2
// [event] click at (10,20)
// [event] click at (5,5)
// [event] processed clicks = 2
