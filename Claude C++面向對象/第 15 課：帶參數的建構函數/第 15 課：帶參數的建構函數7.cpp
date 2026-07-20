// =============================================================================
//  第 15 課：帶參數的建構函數 7  —  五種初始化語法的差別
// =============================================================================
//
// 【主題資訊 Information】
//   五種寫法：
//     (1) Point p1(3.0, 4.0);          直接初始化     direct-initialization
//     (2) Point p2{5.0, 6.0};          直接列表初始化 direct-list-init   （C++11）
//     (3) Point p3 = Point(7.0, 8.0);  複製初始化     copy-initialization
//     (4) Point p4 = {9.0, 10.0};      複製列表初始化 copy-list-init     （C++11）
//     (5) Point* p5 = new Point(1, 2); 動態配置
//   標準版本：(1)(3)(5) 為 C++98；(2)(4) 為 C++11 引入的統一初始化
//   複雜度：五種都是 O(1)；(5) 額外有一次 heap 配置
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 直接初始化 vs 複製初始化：差在「重載決議怎麼挑」】
//   ● 直接初始化 (1)(2)：把括號／大括號內的東西當成建構函數引數，
//     考慮**所有**建構函數，包含被標成 explicit 的。
//   ● 複製初始化 (3)(4)：形式上是「先做出右邊的東西，再用它初始化左邊」，
//     只考慮**非 explicit** 的建構函數（以及轉換函數）。
//   這就是 explicit 唯一真正的作用點：它讓 (3)(4) 這兩種寫法失效，
//   逼使用者改用 (1)(2) 明講「我就是要建這個型別」。本課 10.cpp 會專門示範。
//
// 【2. 小括號 vs 大括號：窄化轉換的防護】
//   大括號初始化禁止**窄化轉換**（narrowing conversion），例如 double → int、
//   long → short、或會遺失精度的整數轉換。小括號則允許（沿用 C 的規則）。
//     Holder h1(3.14);   // OK，截斷成 3
//     Holder h2{3.14};   // 標準規定為 ill-formed
//   注意 g++ 的實際行為：預設只給 -Wnarrowing **警告**（為了相容舊程式碼），
//   加上 -pedantic-errors 才會變成**錯誤**。本機 g++ 15.2 實測確認如此。
//   所以「大括號一定編不過」這句話要看編譯器設定，說「標準規定不合法」才精確。
//
// 【3. (3) Point p3 = Point(7.0, 8.0); 到底有沒有複製】
//   字面上看是「先建一個臨時 Point，再複製給 p3」，但實務上沒有複製：
//     ● C++17 以前：標準**允許**編譯器省略這次複製（copy elision），
//       GCC/Clang 預設都會省。但複製建構函數**仍然必須存在且可存取**——
//       這是關鍵差別。
//     ● C++17 起：這叫**保證的複製省略**（guaranteed copy elision）。
//       右邊的 prvalue 直接就地初始化 p3，語言層面根本沒有「那個臨時物件」，
//       所以複製建構函數可以是 deleted 也照樣合法。
//   本機以 g++ 15.2 實測驗證（把複製建構函數標成 = delete）：
//       -std=c++11 -pedantic-errors → error: use of deleted function
//       -std=c++14 -pedantic-errors → error: use of deleted function
//       -std=c++17 -pedantic-errors → 編譯成功
//   這是一個很好的「用 -pedantic-errors 驗標準版本」的實例。
//
// 【4. (5) new：唯一需要你自己收尾的寫法】
//   前四種都是自動儲存期，離開作用域自動解構。(5) 配置在 heap 上，
//   必須自己 delete，否則就是記憶體洩漏。現代 C++ 應改用智慧指標：
//       auto p5 = std::make_unique<Point>(1.0, 2.0);   // C++14
//   本檔為了對照仍寫原始的 new/delete，正式程式請勿如此。
//
// 【概念補充 Concept Deep Dive】
//
//   ● 為什麼 C++11 要引入大括號「統一初始化」
//     C++98 的初始化語法非常零碎：陣列用 {}、類別用 ()、聚合體又是另一套，
//     還有惡名昭彰的 most vexing parse（Point p(); 會被當成函數宣告）。
//     大括號的目標是「一種語法初始化所有東西」，並順便加上窄化防護。
//
//   ● most vexing parse：大括號真正解決的問題
//         Point p();          // 這是宣告一個「回傳 Point 的無參數函數」！
//         Point p{};          // 這才是建立一個值初始化的 Point
//     只要有可能被解析成函數宣告，編譯器就會選函數宣告。大括號沒有這個歧義。
//
//   ● initializer_list 的優先權陷阱（本檔沒有，但要知道）
//     如果類別同時有 initializer_list 建構函數與一般建構函數，
//     大括號會**優先**匹配 initializer_list 版本：
//         std::vector<int> a(3, 0);   // 三個 0
//         std::vector<int> b{3, 0};   // 兩個元素：3 和 0
//     本檔的 Point 沒有 initializer_list 建構函數，所以 (2)(4) 正常匹配
//     兩個 double 的版本。
//
//   ● 建構函數參數與成員同名
//     本檔 Point(double x, double y) : x(x), y(y) 又是一次同名示範。
//     注意函數體內的 cout << x 印的是**參數**（成員被遮蔽），
//     所幸兩者值相同，看不出差別；print() 裡沒有同名參數，印的才是成員。
//
// 【注意事項 Pay Attention】
//   1. 「大括號一定比較安全」要有但書：它防窄化，但也可能因為
//      initializer_list 優先權而挑到你沒預期的建構函數。
//   2. explicit 只擋得住 (3)(4)，擋不住 (1)(2)。
//   3. new 出來的物件一定要 delete；正式程式請用 std::unique_ptr。
//   4. C++17 之後 (3) 不再產生任何複製，但這是 C++17 才**保證**的；
//      寫要相容舊標準的程式碼時不能依賴它。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】初始化語法的差異
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Point p(1,2); 和 Point p{1,2}; 有什麼差別？
//     答：兩者都是直接初始化，會考慮所有建構函數（含 explicit）。差別在
//         大括號禁止窄化轉換，而且不會有 most vexing parse 的歧義；
//         但若類別有 initializer_list 建構函數，大括號會優先選它。
//     追問：Point p(); 是什麼意思？
//         → 那是宣告一個回傳 Point 的無參數函數（most vexing parse），
//           不是建立物件。要預設初始化請寫 Point p; 或 Point p{};。
//
// 🔥 Q2. Point p = Point(1,2); 會呼叫幾次建構函數？會不會有複製？
//     答：只呼叫一次帶參建構函數，沒有複製。C++17 以前是編譯器**允許**
//         省略（但複製建構函數仍須可存取）；C++17 起是**保證的複製省略**，
//         語言層面直接以 prvalue 初始化 p，複製建構函數可以是 deleted。
//     追問：怎麼證明這個版本差異？
//         → 把複製建構函數標成 = delete 再編。本機 g++ 15.2 實測：
//           C++11/14 加 -pedantic-errors 會報 use of deleted function，
//           C++17 則編譯成功。
//
// ⚠️ 陷阱. 大括號初始化禁止窄化，所以 Holder h{3.14}; 一定編不過，對嗎？
//     答：標準規定它是 ill-formed 沒錯，但 g++ 預設只給 -Wnarrowing **警告**
//         就放行；要加 -pedantic-errors（或 -Werror=narrowing）才會是錯誤。
//     為什麼會錯：把「標準說不合法」直接等同於「編譯器一定拒絕」。
//         標準只要求發出診斷訊息，警告也算診斷；編譯器為了相容既有程式碼
//         常選擇警告而非錯誤。驗標準版本要用 -pedantic-errors 才準。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class Point {
private:
    double x, y;

public:
    // 同名參數 + 初始化列表（本課 6.cpp 的寫法）
    Point(double x, double y) : x(x), y(y) {
        // 注意：這裡的 x、y 是「參數」（成員被遮蔽），只是值剛好相同
        cout << "  建構 Point(" << x << ", " << y << ")" << endl;
    }

    void print() const {
        // 這裡沒有同名參數，印的是成員
        cout << "  (" << x << ", " << y << ")" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】幾何圖形的邊界框（bounding box）
//   情境：2D 繪圖、遊戲碰撞偵測、地圖圖磚裁切都需要「左下角 + 右上角」的框。
//         建立框的來源很多樣（設定檔、滑鼠拖曳、計算結果），因此不同的初始化
//         語法在真實程式裡其實都會遇到。
//   重點：示範同一個型別用不同語法建立，以及大括號的可讀性。
// -----------------------------------------------------------------------------
class BoundingBox {
private:
    double left_, bottom_, right_, top_;

public:
    BoundingBox(double left, double bottom, double right, double top)
        : left_(left), bottom_(bottom), right_(right), top_(top) { }

    double width()  const { return right_ - left_; }
    double height() const { return top_ - bottom_; }

    bool contains(double px, double py) const {
        return px >= left_ && px <= right_ && py >= bottom_ && py <= top_;
    }

    void print() const {
        cout << "  框 [(" << left_ << "," << bottom_ << ") - ("
             << right_ << "," << top_ << ")]"
             << " 寬=" << width() << " 高=" << height() << endl;
    }
};

int main() {
    cout << "=== 語法 1：直接初始化（小括號）===" << endl;
    Point p1(3.0, 4.0);
    p1.print();

    cout << "\n=== 語法 2：直接列表初始化（大括號，C++11）===" << endl;
    // 與語法 1 同樣考慮所有建構函數，但禁止窄化轉換
    Point p2{5.0, 6.0};
    p2.print();

    cout << "\n=== 語法 3：複製初始化（等號）===" << endl;
    // C++17 起為保證的複製省略：沒有臨時物件、沒有複製
    Point p3 = Point(7.0, 8.0);
    p3.print();

    cout << "\n=== 語法 4：等號 + 大括號（複製列表初始化，C++11）===" << endl;
    // 只考慮非 explicit 建構函數；若 Point 標了 explicit 這行就編不過
    Point p4 = {9.0, 10.0};
    p4.print();

    cout << "\n=== 語法 5：動態配置（需自行 delete）===" << endl;
    Point* p5 = new Point(1.0, 2.0);
    p5->print();
    delete p5;      // 正式程式建議改用 std::unique_ptr 自動管理

    cout << "\n=== 日常實務：邊界框與碰撞測試 ===" << endl;
    BoundingBox screen(0.0, 0.0, 1920.0, 1080.0);      // 語法 1
    BoundingBox sprite{100.0, 200.0, 164.0, 264.0};    // 語法 2
    screen.print();
    sprite.print();
    cout << "  點 (150,220) 在 sprite 內? "
         << (sprite.contains(150.0, 220.0) ? "是" : "否") << endl;
    cout << "  點 (900,500) 在 sprite 內? "
         << (sprite.contains(900.0, 500.0) ? "是" : "否") << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 15 課：帶參數的建構函數7.cpp" -o demo7
//
// ※ 想親自驗證【詳細解釋 3】的 C++17 保證複製省略，可另建一個小檔：
//     struct P { P(double,double){} P(const P&) = delete; };
//     int main(){ P p = P(1.0,2.0); (void)p; }
//   然後比較 -std=c++11 -pedantic-errors（報 use of deleted function）
//   與 -std=c++17 -pedantic-errors（編譯成功）。

// === 預期輸出 ===
// === 語法 1：直接初始化（小括號）===
//   建構 Point(3, 4)
//   (3, 4)
//
// === 語法 2：直接列表初始化（大括號，C++11）===
//   建構 Point(5, 6)
//   (5, 6)
//
// === 語法 3：複製初始化（等號）===
//   建構 Point(7, 8)
//   (7, 8)
//
// === 語法 4：等號 + 大括號（複製列表初始化，C++11）===
//   建構 Point(9, 10)
//   (9, 10)
//
// === 語法 5：動態配置（需自行 delete）===
//   建構 Point(1, 2)
//   (1, 2)
//
// === 日常實務：邊界框與碰撞測試 ===
//   框 [(0,0) - (1920,1080)] 寬=1920 高=1080
//   框 [(100,200) - (164,264)] 寬=64 高=64
//   點 (150,220) 在 sprite 內? 是
//   點 (900,500) 在 sprite 內? 否
