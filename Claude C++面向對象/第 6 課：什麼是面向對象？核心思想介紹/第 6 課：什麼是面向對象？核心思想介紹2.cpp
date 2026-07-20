// =============================================================================
//  第 6 課：什麼是面向對象？核心思想介紹2.cpp  —  多型（Polymorphism）與虛擬函式
// =============================================================================
//
// 【主題資訊 Information】
//   virtual 回傳型別 函式名(參數) [override] [final];
//   virtual ~Base();                       // 多型基類【必須】有虛擬解構子
//   override（C++11）：明確宣告「我在覆寫基類的虛擬函式」，寫錯簽名會編譯失敗
//   final   （C++11）：禁止再被覆寫
//   標準版本：virtual 為 C++98；override / final 為 C++11
//   執行期成本：一次間接跳躍（透過 vtable），通常無法被 inline
//   標頭檔：語言核心特性
//
// 【詳細解釋 Explanation】
//
// 【1. 多型解決的是什麼問題】
//   printArea(Shape*) 這個函式【只寫了一次】，
//   卻能正確處理 Circle 與 Rectangle，而且未來新增 Triangle 時
//   printArea 一行都不必改。
//   對照沒有多型的寫法：
//       if (type == CIRCLE)      area = ...;
//       else if (type == RECT)   area = ...;
//   每新增一種形狀，就要回頭修改每一個這樣的 if-else ——
//   而且很容易漏掉其中一處。
//   多型把「決定要執行哪段程式碼」的責任從【呼叫端】移到【物件自己】身上，
//   這正是開放封閉原則（對擴充開放、對修改封閉）的實作方式。
//
// 【2. virtual 在執行期是怎麼運作的】
//   編譯器為每個有虛擬函式的類別建立一張【虛擬函式表（vtable）】——
//   本質上是一個函式指標陣列，每個類別一份（不是每個物件一份）。
//   每個物件則多存一個指向該表的指標（vptr），通常放在物件最前面。
//       s->area() 實際上編譯成：
//           (*(s->vptr[area 的索引]))(s)
//   索引在【編譯期】就固定了，執行期只做一次查表 + 間接呼叫。
//   所以虛擬呼叫的成本是常數，不隨繼承深度增加 ——
//   但它通常無法被 inline，這才是效能上的主要代價
//   （失去 inline 之後，後續的常數傳播、迴圈最佳化也跟著失效）。
//
// 【3. 為什麼虛擬解構子是必須的，不是選配】
//   本檔寫了 virtual ~Shape() {}，這絕不是裝飾。
//   考慮：
//       Shape* s = new Circle(5.0);
//       delete s;
//   若 ~Shape 不是 virtual，delete 只會呼叫 ~Shape()，
//   Circle 自己的解構子【永遠不會執行】——
//   標準明訂這是【未定義行為】（[expr.delete]/3），
//   而不只是「會漏掉資源」。
//   本檔的 Circle/Rectangle 只有 double 成員，
//   即使真的發生也不容易看出症狀；
//   但一旦派生類持有 std::string、vector 或檔案 handle，就是實實在在的洩漏。
//   規則：【只要類別可能被當成多型基底使用（有任何 virtual 函式），
//   就給它 virtual 解構子】，或把解構子設為 protected 非虛擬
//   以禁止透過基類指標刪除。
//
// 【4. override 為什麼值得每次都寫】
//   override 不會改變任何執行期行為，它是【編譯期的檢查】。
//   假設不小心把參數寫錯：
//       double area(int) { ... }          // 沒寫 override
//   這不是覆寫，而是【多載了一個新函式】——
//   編譯完全通過，但透過 Shape* 呼叫時永遠執行基類版本，
//   而且症狀會在很遠的地方才顯現。
//   加上 override 之後，編譯器會直接報錯說找不到可覆寫的基類函式。
//   同理，const 寫錯（基類是 const 成員函式、派生類漏了 const）
//   也是靠 override 才抓得到。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 物件變大了多少
//   加入虛擬函式後，每個物件多一個 vptr（本機 x86-64 為 8 bytes）。
//   本機實測：Circle 有一個 double 成員，sizeof(Circle) = 16
//   （8 bytes vptr + 8 bytes double）；
//   Rectangle 有兩個 double，sizeof(Rectangle) = 24。
//   而 sizeof(Shape) = 8 —— 它沒有資料成員，就只有那個 vptr。
//   這些數值屬【實作定義】（vptr 大小與對齊隨平台而異）。
//   對照組：如果拿掉所有 virtual，Shape 會變成 1 byte（空類別的最小大小）。
//
// (B) 為什麼在建構子／解構子裡呼叫虛擬函式不會有多型
//   建構基類子物件時，派生類部分【還不存在】，
//   此時 vptr 指向的是基類的 vtable。
//   所以在 Shape 的建構子裡呼叫 area()，執行的是 Shape::area()
//   而不是 Circle::area()——這是標準規定的行為（不是 UB），
//   但幾乎總是違反寫程式者的意圖。
//   解構時同理，順序相反。
//   結論：不要在建構／解構過程中依賴虛擬分派。
//
// (C) 純虛擬函式與抽象類別
//   本檔的 Shape::area() 有實作（return 0），
//   所以 Shape 可以被實體化 —— 但「一個面積為 0 的通用形狀」是沒有意義的。
//   更好的設計是宣告成純虛擬：
//       virtual double area() const = 0;
//   如此 Shape 變成【抽象類別】，無法建立實體，
//   而且忘記實作 area() 的派生類會在編譯期就被擋下來。
//   同課 3 號檔的 Database 就是用這個做法。
//
// 【注意事項 Pay Attention】
//   1. 多型基類【必須】有 virtual 解構子。透過基類指標 delete 派生類物件
//      而基類解構子非虛擬，是【未定義行為】，不只是資源洩漏。
//   2. 覆寫時一律加 override（C++11）—— 它能抓出簽名寫錯、
//      漏 const 這類「編譯得過但行為錯誤」的 bug。
//   3. 建構子與解構子內【不會】發生虛擬分派，執行的是當前層級的版本。
//   4. 虛擬呼叫的成本主要不在查表，而在【無法 inline】。
//   5. 本檔的 area() 應該宣告成 const 成員函式（不修改物件狀態），
//      這是原始教材的簡化；實務上請寫 virtual double area() const。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】虛擬函式與多型
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼多型基類一定要有 virtual 解構子？不加會怎樣？
//     答：因為 delete 基類指標時，若解構子非虛擬，
//         只會呼叫基類的解構子，派生類的解構子【不會】執行。
//         標準明訂這是【未定義行為】（[expr.delete]/3），
//         不只是「漏掉清理」。派生類若持有 string、vector、
//         檔案 handle 等資源，就會真的洩漏。
//     追問：那是不是所有類別都該加 virtual 解構子？
//           → 不是。加了就會產生 vtable，讓物件多出一個 vptr、
//             也失去 trivially destructible 的性質。
//             判準是「這個類別會不會被當成多型基底、
//             會不會透過基類指標 delete」。
//             若只想禁止多型刪除，可把解構子設為 protected 且非虛擬。
//
// 🔥 Q2. virtual 函式呼叫的成本是什麼？是查表很慢嗎？
//     答：查表本身很便宜 —— 索引在編譯期就固定，
//         執行期只是一次記憶體載入加一次間接呼叫，成本是常數，
//         而且不隨繼承深度增加。
//         真正的代價是【無法 inline】：編譯器不知道會呼叫哪個版本，
//         於是連帶失去常數傳播、迴圈展開等後續最佳化。
//         在熱迴圈裡，這個間接損失通常遠大於查表本身。
//     追問：有辦法拿回效能嗎？
//           → 若型別在編譯期已知，可用 CRTP（靜態多型）；
//             或把 final 加在類別或函式上，讓編譯器有機會去虛擬化
//             （devirtualization）。
//
// ⚠️ 陷阱. 派生類寫了 double area(int n) { ... }，編譯完全通過，
//          但透過 Shape* 呼叫時永遠執行基類版本 —— 為什麼？
//     答：因為那根本不是覆寫。基類的是 area()（無參數），
//         派生類的是 area(int)——簽名不同，
//         這只是在派生類裡【多載】了一個新函式，
//         基類的虛擬 area() 從未被覆寫。
//         透過 Shape* 呼叫時自然執行 Shape::area()。
//     為什麼會錯：以為「同名 + 在派生類裡 + 基類有 virtual」就是覆寫。
//         覆寫的條件嚴格得多：函式名、參數列、const/volatile 限定、
//         參考限定必須【完全一致】（回傳型別則允許協變）。
//         最陰險的版本是漏寫 const —— 基類 area() const、
//         派生類 area()，看起來一模一樣卻不是覆寫。
//         解法只有一個：【每次覆寫都寫 override】，
//         讓編譯器在簽名對不上時直接報錯，而不是默默產生一個新多載。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class Shape {
public:
    virtual double area() {  // virtual = 虛函數，允許派生類覆寫
        return 0;
    }
    virtual ~Shape() {}  // 虛解構函數（後續課程會詳解）
};

class Circle : public Shape {
private:
    double radius;
public:
    Circle(double r) : radius(r) {}

    double area() override {  // 覆寫基類的 area()
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Shape {
private:
    double width, height;
public:
    Rectangle(double w, double h) : width(w), height(h) {}

    double area() override {
        return width * height;
    }
};

// 這個函數接受任何 Shape！
void printArea(Shape* s) {
    cout << "面積 = " << s->area() << endl;
}

int main() {
    Circle c(5.0);
    Rectangle r(4.0, 6.0);

    printArea(&c);  // 呼叫 Circle::area()
    printArea(&r);  // 呼叫 Rectangle::area()

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 6 課：什麼是面向對象？核心思想介紹2.cpp" -o oo2

// 註 1:本檔輸出是【完全確定的】—— 沒有輸入、亂數、執行緒或位址。
//
// 註 2:「面積 = 78.5397」是 3.14159 × 5² = 78.539750 經
//      iostream 預設的 6 位【有效數字】格式化後的結果（尾數四捨五入）。
//      而 24 是 4×6 的精確值，末尾沒有多餘的 .0 ——
//      因為預設格式會去掉尾隨的零。
//
// 註 3:同一個 printArea(Shape*) 印出兩種不同的面積，
//      這就是多型：呼叫端只寫一次，實際執行哪個 area()
//      由物件自己在執行期決定（透過 vtable）。
//
// 註 4:本機實測的物件大小（皆屬實作定義，x86-64 / g++ 15.2.0）：
//        sizeof(Shape)=8（只有 vptr）、sizeof(Circle)=16、sizeof(Rectangle)=24。
//      加入 virtual 讓每個物件多帶一個 8 bytes 的 vptr。
//
// 【LeetCode 實戰範例】從缺 —— 理由：
//      本檔主題是「虛擬函式與執行期分派」。可選清單中的設計類題目
//      （146、155、705、707、1603、1656）全部只需要單一類別，
//      沒有一題會用到繼承體系或虛擬函式；
//      強行加一題只會模糊本檔的焦點。

// === 預期輸出 ===
// 面積 = 78.5397
// 面積 = 24
