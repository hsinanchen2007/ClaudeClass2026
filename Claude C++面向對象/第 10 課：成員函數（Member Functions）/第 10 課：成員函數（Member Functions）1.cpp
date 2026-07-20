// =============================================================================
//  第 10 課：成員函數 1  —  類內定義 vs 類外定義（inline 的真正含義）
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  class X { ret f(args) { ... } };        // 類內定義 → 隱式 inline
//           class X { ret f(args); };  ret X::f(args) { ... }  // 類外定義
//   標準：  C++98 起即有；本檔語法無版本相依。
//   標頭檔：<iostream>
//   複雜度：成員函數呼叫本身 O(1)；本檔 area()/perimeter() 皆為 O(1) 算術。
//
// 【詳細解釋 Explanation】
//
// 【1. 兩種定義位置，語意完全相同】
//   成員函數寫在類內或類外，對「呼叫者」而言毫無差別 —— 都是同一個函式，
//   都能存取所有成員、都吃同一個隱含的 this。差別只在兩件事：
//     (a) 類內定義會自動帶上 inline 語意（見第 2 點）；
//     (b) 類外定義必須用 `Rectangle::` 限定，告訴編譯器「這是那個類的成員」，
//         否則它會被當成一個同名的自由函式，而自由函式看不到 width/height。
//
// 【2. 「隱式 inline」到底 inline 了什麼】
//   最常見的誤解是「inline = 編譯器會把函式展開，所以比較快」。
//   實際上現代編譯器的 inline expansion 決策完全由最佳化器自行判斷，
//   跟你有沒有寫 inline 幾乎無關（-O0 下寫了也不展開，-O2 下沒寫也可能展開）。
//   inline 關鍵字真正的、標準規定的意義是 **放寬 ODR（One Definition Rule）**：
//   允許同一個函式定義出現在多個 translation unit 中，連結器只留一份。
//   這正是為什麼類內定義必須是隱式 inline —— 類定義通常放在 header，
//   header 被多個 .cpp include，若不是 inline 就會 multiple definition 連結錯誤。
//
// 【3. 為什麼類內可以「先用後宣告」】
//   注意 print() 在類內定義時就呼叫了 perimeter()，而 perimeter() 的宣告
//   寫在 print() 之後 —— 這在自由函式是不合法的。類的成員函數本體被視為
//   在「完整的類定義之後」才解析（complete-class context），
//   所以整個類的成員彼此可見，不受書寫順序限制。
//   ★ 但這只適用於函式「本體」；成員的「型別」與「預設引數」仍依書寫順序解析。
//
// 【4. 成員函數呼叫成員函數】
//   print() 內寫 area() 等價於 this->area()。編譯器把成員函數編譯成一個
//   帶隱藏 this 參數的普通函式，呼叫時把當前物件位址傳下去，
//   所以「同一個物件」的狀態自然被共用。
//
// 【概念補充 Concept Deep Dive】
//   成員函數 **不佔物件大小**。本檔 Rectangle 只有兩個 double，
//   sizeof(Rectangle) 在本機 x86-64 Linux 實測為 16（此為實作定義值，
//   取決於 double 大小與對齊要求）。四個成員函數只有一份機器碼，
//   放在 text 段，所有 Rectangle 物件共用 —— 這與 virtual function
//   會塞一個 vptr 進物件（第 20 課以後）形成對比：本類沒有 virtual，
//   所以沒有任何隱藏欄位。
//
//   類外定義的 `double Rectangle::perimeter()` 中，回傳型別 double 寫在
//   `Rectangle::` **之前**，因此它在 class scope 之外解析；而參數列與函式本體
//   在 `Rectangle::` 之後，屬於 class scope。這個細節在回傳類內巢狀型別時會咬人：
//       Rectangle::Iterator Rectangle::begin() { ... }   // 回傳型別要寫全名
//   C++11 的 trailing return type（auto f() -> Iterator）可以繞開這個不對稱。
//
// 【注意事項 Pay Attention】
//   1. 類外定義若漏寫 `Rectangle::`，不會報「忘了寫」，而是變成自由函式，
//      錯誤訊息會是「width 未宣告」，離真正原因很遠。
//   2. 類外定義**不要**重複寫 inline 以外的說明符：virtual、static、
//      預設引數都只能寫在類內宣告處，寫在類外定義是編譯錯誤。
//   3. 類內定義的函式若很長，會讓 header 變肥、拖慢編譯，也讓實作細節外洩；
//      工程慣例是「一行 getter 放類內，其餘放 .cpp」。
//   4. 本檔 area()/perimeter()/print() 都沒有加 const —— 它們不修改成員，
//      理應宣告成 `double area() const`。沒加的後果是「const Rectangle 物件
//      無法呼叫它們」，這是第 10 課後段與 const 正確性的重點。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成員函數的定義位置與 inline
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 類內定義與類外定義的成員函數有什麼差別？
//     答：語意上沒有差別，都是同一個成員函式、都吃同一個 this。
//         唯一差別是類內定義帶隱式 inline（放寬 ODR），
//         以及類外定義必須用 Rectangle:: 限定名稱。
//     追問：那把類外定義搬進 header 會怎樣？
//         → 多個 .cpp include 就會 multiple definition 連結錯誤，
//           除非手動補上 inline。
//
// 🔥 Q2. inline 到底是做什麼的？寫了會不會比較快？
//     答：inline 的標準語意是放寬 ODR，允許多個 TU 有相同定義、連結器擇一。
//         「是否展開」是最佳化器的獨立決策，跟關鍵字幾乎無關。
//         把 inline 當效能開關是常見的錯誤認知。
//     追問：那要怎麼真的強制展開？
//         → 沒有可攜的方法；GCC/Clang 有 __attribute__((always_inline))，
//           但那是編譯器擴充，且強制展開常常反而讓 I-cache 變差。
//
// ⚠️ 陷阱. print() 在類內呼叫了寫在它後面的 perimeter()，為什麼能編譯過？
//     答：成員函式的**本體**在完整類定義之後才解析（complete-class context），
//         所以類內成員彼此可見，不受書寫順序影響。
//     為什麼會錯：多數人把 C 的「識別字必須先宣告後使用」直接套到類內，
//         於是預期這裡會報錯。但那條規則只約束 namespace scope，
//         類的成員本體是刻意被延後解析的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
using namespace std;

class Rectangle {
public:
    double width = 0;
    double height = 0;

    // 類內定義（隱式 inline）
    double area() {
        return width * height;
    }

    // 類內宣告，類外定義
    double perimeter();
    void scale(double factor);
    void print();
};

// 類外定義
double Rectangle::perimeter() {
    return 2 * (width + height);
}

void Rectangle::scale(double factor) {
    width *= factor;
    height *= factor;
}

void Rectangle::print() {
    cout << "Rectangle(" << width << " x " << height << ")" << endl;
    cout << "  面積: " << area() << endl;            // 成員函數調用另一個成員函數
    cout << "  周長: " << perimeter() << endl;       // 同上
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 223. Rectangle Area
//   題目：給兩個軸對齊矩形（各以左下角、右上角座標表示），求兩者覆蓋的總面積
//         （重疊部分只算一次）。
//   為什麼用到本主題：這正是「把幾何計算封裝成成員函數」的典型場景 ——
//         總面積 = A面積 + B面積 - 重疊面積，其中每個面積都是一次成員函數呼叫。
//         這裡刻意用類外定義，示範它與類內定義可自由混用。
// -----------------------------------------------------------------------------
class AlignedRect {
public:
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;   // (x1,y1) 左下、(x2,y2) 右上

    long long area() const {              // 類內定義（隱式 inline）
        return 1LL * (x2 - x1) * (y2 - y1);
    }
    long long overlapWith(const AlignedRect& o) const;  // 類外定義
};

long long AlignedRect::overlapWith(const AlignedRect& o) const {
    // 重疊區同樣是個矩形：各軸取交集；沒交集時寬或高為負 → 夾成 0
    int w = min(x2, o.x2) - max(x1, o.x1);
    int h = min(y2, o.y2) - max(y1, o.y1);
    if (w <= 0 || h <= 0) return 0;
    return 1LL * w * h;
}

long long computeTotalArea(const AlignedRect& a, const AlignedRect& b) {
    return a.area() + b.area() - a.overlapWith(b);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】縮圖服務：等比例縮放到「不超過上限框」
//   情境：圖片上傳後要產生縮圖，規則是「保持長寬比，縮到剛好塞進 maxW x maxH」，
//         而且**不放大**原圖（比上限框還小的圖直接原樣輸出）。
//         這是所有相簿／CMS／電商後台都會寫的一段邏輯。
//   為什麼用到本主題：scale() 是會**修改自身狀態**的成員函數（mutator），
//         而 area()/perimeter() 是唯讀的 accessor —— 這組對比正是成員函數
//         設計的核心：誰改狀態、誰只讀狀態，要一眼看得出來。
// -----------------------------------------------------------------------------
void fitInto(Rectangle& img, double maxW, double maxH) {
    if (img.width <= 0 || img.height <= 0) return;        // 防呆：避免除以 0
    double ratio = min(maxW / img.width, maxH / img.height);
    if (ratio >= 1.0) return;                             // 已經夠小 → 不放大
    img.scale(ratio);                                     // 複用既有的 mutator
}

int main() {
    cout << "=== 基本：類內/類外定義的成員函數 ===" << endl;
    Rectangle r;
    r.width = 5.0;
    r.height = 3.0;

    r.print();

    r.scale(2.0);
    cout << "\n放大 2 倍後:" << endl;
    r.print();

    cout << "\n=== LeetCode 223. Rectangle Area ===" << endl;
    AlignedRect a{-3, 0, 3, 4};
    AlignedRect b{0, -1, 9, 2};
    cout << "總覆蓋面積: " << computeTotalArea(a, b) << endl;   // 45
    AlignedRect c{-2, -2, 2, 2};
    AlignedRect d{3, 3, 5, 5};                                  // 完全不重疊
    cout << "不重疊時: " << computeTotalArea(c, d) << endl;      // 20

    cout << "\n=== 日常實務：縮圖等比例塞進上限框 ===" << endl;
    Rectangle photo;
    photo.width = 4000; photo.height = 3000;
    fitInto(photo, 800, 800);
    cout << "4000x3000 塞進 800x800 -> ";
    photo.print();

    Rectangle small;
    small.width = 320; small.height = 240;
    fitInto(small, 800, 800);
    cout << "320x240 塞進 800x800（不放大）-> ";
    small.print();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：成員函數（Member Functions）1.cpp" -o member1

// === 預期輸出 ===
// === 基本：類內/類外定義的成員函數 ===
// Rectangle(5 x 3)
//   面積: 15
//   周長: 16
//
// 放大 2 倍後:
// Rectangle(10 x 6)
//   面積: 60
//   周長: 32
//
// === LeetCode 223. Rectangle Area ===
// 總覆蓋面積: 45
// 不重疊時: 20
//
// === 日常實務：縮圖等比例塞進上限框 ===
// 4000x3000 塞進 800x800 -> Rectangle(800 x 600)
//   面積: 480000
//   周長: 2800
// 320x240 塞進 800x800（不放大）-> Rectangle(320 x 240)
//   面積: 76800
//   周長: 1120
