// =============================================================================
//  第二課：泛型編程（Generic Programming）概念11.cpp
//   —  執行期多型（runtime polymorphism）：虛擬函式與 vtable
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     class Shape {
//     public:
//         virtual double area() const = 0;   // 純虛擬函式 → 抽象類別
//         virtual ~Shape() = default;        // 多型基底類別必須有虛擬解構子
//     };
//     class Circle : public Shape { double area() const override; };
//
//   標準版本：virtual C++98 起；override / = default C++11 起；
//             std::unique_ptr / make_unique 分別是 C++11 / C++14
//             （本檔用 -std=c++17 編譯）
//   標頭檔  ：<iostream>、<vector>、<memory>
//   分派成本：一次間接跳躍（本機 -O2 實測組語為 `jmp *%rax`），無法 inline
//   空間成本：每個物件多一個 vptr（本機 x86-64 實測 8 bytes）
//
//   本檔是概念12.cpp 的對照組：同一個問題，一個用繼承+virtual（執行期），
//   一個用 template（編譯期）。兩者的取捨是本課的核心結論。
//
// 【詳細解釋 Explanation】
//
// 【1. 明確介面（explicit interface）】
// 與模板的隱含介面相反，這裡的契約是**寫出來的**：
//     class Shape { virtual double area() const = 0; };
// 任何想被當成 Shape 用的型別，都必須 public 繼承 Shape 並實作 area()。
// 這是 is-a 關係，寫在型別宣告裡、一眼可見，IDE 也能直接列出所有實作者。
//
// 代價是**侵入式**（intrusive）：型別必須「事先知道」自己要被當成 Shape。
// 若 Circle 來自第三方函式庫、而該函式庫的作者沒讓它繼承你的 Shape，
// 你就無法把它放進這個 vector —— 除非另外寫一層 adapter 包起來。
//
// 【2. 為什麼一定要用指標或參考】
// 注意 vector 裡放的是 `std::unique_ptr<Shape>`，不是 `Shape`。原因有二：
//   * Shape 是抽象類別（有純虛擬函式），根本無法實體化。
//   * 更根本的原因：**多型只透過指標或參考運作**。若寫成
//     `std::vector<Shape>`（假設 Shape 可實體化），把 Circle 放進去會發生
//     **object slicing（物件切片）**：只有 Shape 的那部分被複製進容器，
//     Circle 特有的成員與 vptr 全部遺失，之後呼叫 area() 得到的是基底行為。
//     這是 C++ 最惡名昭彰的陷阱之一，而且**編譯器不會給任何警告**。
//
// 【3. vtable：執行期分派是怎麼做到的】
// 編譯 `shape->area()` 時，編譯器並不知道 shape 實際指向 Circle 還是 Rectangle
// （這要到執行期才知道）。它產生的程式碼大致是：
//     1) 從物件開頭讀出 vptr（指向該類別的虛擬函式表 vtable）
//     2) 從 vtable 的固定位移取出 area() 的實際位址
//     3) 間接呼叫該位址
//
// 本機 g++ 15.2 -O2 實測，這個呼叫編譯出來的關鍵指令是：
//     jmp *%rax            ← 間接跳躍，目標位址在執行期才確定
// 對照概念12.cpp 的模板版本，同樣的邏輯被完全 inline 成兩道 mulsd 乘法指令、
// 連一次函式呼叫都沒有。
//
// 這個間接性帶來三重成本：
//   * 一次額外的記憶體讀取（取 vptr、查表）
//   * **無法 inline** —— 這通常比呼叫本身更貴，因為它同時阻斷了常數傳播、
//     迴圈展開等一整串後續最佳化
//   * 分支預測器可能失準（呼叫目標不固定時）
//
// 空間上，本機 x86-64 實測：只含一個 double 的類別，
// 非虛擬版 sizeof = 8，加上 virtual 後 sizeof = 16 —— vptr 佔了 8 bytes，
// 對小物件而言等於體積翻倍。
// （以上為 g++ 15.2 / x86-64 的實測值；vtable 並非標準規定的機制，
//   標準只描述行為，實作方式由編譯器自行決定，只是主流實作都用 vtable。）
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼多型基底類別必須有 virtual 解構子
//   `std::unique_ptr<Shape>` 在銷毀時呼叫 `delete` 一個 Shape*。
//   若 ~Shape() 不是 virtual，就只會呼叫 ~Shape()，
//   Circle 的解構子**永遠不會被執行** —— 這是 UB，實務上通常表現為
//   Circle 自己持有的資源（記憶體、檔案控制代碼）洩漏。
//   規則：**任何打算被當成多型基底使用的類別，解構子必須 virtual**
//   （或設為 protected 非虛擬，以禁止透過基底指標 delete）。
//   本檔的 `virtual ~Shape() = default;` 正是這條規則。
//
// (B) override 關鍵字為什麼重要（C++11）
//   沒有 override 時，若你把簽名寫錯（例如漏了 const，寫成
//   `double area()`），編譯器會把它視為一個**新的、無關的函式**，
//   而不是覆寫。程式照樣編譯，但呼叫時走的是基底版本 —— 一個安靜的邏輯錯誤。
//   加上 override 後，編譯器會檢查「這確實覆寫了某個虛擬函式」，
//   簽名不符就直接報錯。**新程式碼一律該加**。
//
// (C) 執行期多型真正的價值：異質容器與晚期綁定
//   別因為有成本就否定它。本檔做到了一件模板做不到的事：
//   **把不同型別的物件放進同一個容器，統一走訪**。
//   模板版（概念12.cpp）的 Circle 與 Rectangle 是毫無關係的兩個型別，
//   沒有共同的靜態型別可以放進同一個 vector。
//   更關鍵的是「型別在執行期才決定」的場景：
//     * 依設定檔決定要建立哪種 Shape
//     * 外掛（plugin）系統：實作在 .so 裡，編譯主程式時根本不存在
//     * 需要跨越 ABI 邊界的介面
//   這些場景模板完全無能為力 —— 模板要求型別在**編譯期**就已知。
//
// (D) sizeof(std::unique_ptr<Shape>) 與所有權
//   unique_ptr 是零額外開銷的所有權包裝（本機實測與裸指標同為 8 bytes），
//   它保證離開作用域時自動 delete，且不可複製只可移動。
//   在 C++11 之前這裡得寫裸指標並手動 delete，是記憶體洩漏的常見來源。
//
// 【注意事項 Pay Attention】
// 1. 多型只透過**指標或參考**運作。按值傳遞或存入 vector<Shape> 會造成
//    object slicing，且編譯器不會警告。
// 2. 多型基底類別的解構子必須是 virtual，否則透過基底指標 delete 是 UB。
// 3. 覆寫時務必加 override。漏加而簽名寫錯會安靜地變成「新函式」而非覆寫。
// 4. 建構子與解構子中呼叫虛擬函式，不會分派到衍生類別 ——
//    此時衍生部分尚未建構完成（或已銷毀），vptr 指向的是當前正在建構的
//    那一層。這是另一個經典陷阱。
// 5. vptr 與 vtable 都是實作機制，不是標準規定。標準只規範行為；
//    本檔引用的 8 bytes、`jmp *%rax` 皆為本機 g++ 15.2 / x86-64 實測值。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行期多型
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼多型必須透過指標或參考？把物件直接存進 vector<Shape> 會怎樣？
//     答：會發生 **object slicing（物件切片）**：只有基底那部分被複製進去，
//         衍生類別特有的成員與 vptr 全部遺失，之後呼叫虛擬函式得到的是
//         基底的行為。編譯器**不會警告**，是個安靜的邏輯錯誤。
//         本檔用 vector<unique_ptr<Shape>> 正是為了避免這件事
//         （何況 Shape 是抽象類別，本來也無法實體化）。
//     追問：那怎麼安全地把多型物件放進容器？
//         → 存智慧指標（unique_ptr / shared_ptr）或 reference_wrapper；
//           C++17 起若型別集合已知且封閉，也可用 std::variant 走另一條路
//           （值語意、無 vtable、以 std::visit 分派）。
//
// 🔥 Q2. 虛擬函式的成本具體有哪些？
//     答：空間上每個物件多一個 vptr（本機 x86-64 實測：只含一個 double 的
//         類別從 8 bytes 變 16 bytes）。時間上是一次間接跳躍
//         （本機 -O2 實測組語為 jmp *%rax），但**真正的代價是無法 inline** ——
//         這同時阻斷了常數傳播、迴圈展開等後續最佳化。
//         對照組（概念12.cpp）的模板版被完全 inline 成兩道乘法指令。
//     追問：那是不是應該盡量避免 virtual？
//         → 不是。若型別要到執行期才知道（設定檔、外掛、跨 ABI 邊界），
//           模板根本做不到，virtual 是唯一解。成本要放在「它換到什麼能力」
//           的脈絡下評估，而不是無條件避免。
//
// ⚠️ 陷阱. Shape 的解構子如果不寫 virtual，會發生什麼？
//     答：透過 Shape* 刪除一個 Circle 是 **UB**。實務上通常表現為只呼叫
//         ~Shape() 而 ~Circle() 從不執行，於是 Circle 持有的資源洩漏。
//         注意這是 UB，標準不保證任何特定結果，也可能完全看不出異常。
//     為什麼會錯：以為「解構子會像建構子那樣自動一層層呼叫」。
//         由基底往衍生的正確銷毀鏈，是**靠虛擬分派**找到最衍生的解構子後
//         才開始的；沒有 virtual，編譯器只會依靜態型別呼叫 ~Shape()，
//         整條鏈根本沒被啟動。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <memory>

// 基底類別：明確介面 —— 契約寫在型別宣告裡
class Shape {
public:
    virtual double area() const = 0;   // 純虛擬 → Shape 是抽象類別
    virtual ~Shape() = default;        // 多型基底必須有虛擬解構子（見注意事項 2）
};

// 衍生類別
class Circle : public Shape {
    double radius;
public:
    Circle(double r) : radius(r) {}
    double area() const override {     // override：讓編譯器檢查確實覆寫了
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Shape {
    double width, height;
public:
    Rectangle(double w, double h) : width(w), height(h) {}
    double area() const override {
        return width * height;
    }
};

// 對照用：完全相同的資料，但沒有 virtual
class PlainCircle {
    double radius;
public:
    PlainCircle(double r) : radius(r) {}
    double area() const { return 3.14159 * radius * radius; }
};

int main() {
    std::cout << "=== 異質容器：不同型別放進同一個 vector ===" << std::endl;
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>(5.0));
    shapes.push_back(std::make_unique<Rectangle>(4.0, 3.0));

    for (const auto& shape : shapes) {
        std::cout << "Area: " << shape->area() << std::endl;  // 執行期決定
    }
    std::cout << "這是模板做不到的事 —— 見概念12.cpp 的對照。" << std::endl;

    std::cout << "\n=== vptr 的空間成本 ===" << std::endl;
    std::cout << "sizeof(double)      = " << sizeof(double) << std::endl;
    std::cout << "sizeof(PlainCircle) = " << sizeof(PlainCircle)
              << "   <- 只有一個 double，無 virtual" << std::endl;
    std::cout << "sizeof(Circle)      = " << sizeof(Circle)
              << "   <- 同樣一個 double，但多了 vptr" << std::endl;
    std::cout << "（本機 g++ 15.2 / x86-64 實測值；vtable 非標準規定的機制）"
              << std::endl;

    std::cout << "\n=== 分派方式 ===" << std::endl;
    std::cout << "shape->area() 編譯後是間接跳躍（本機 -O2 組語：jmp *%rax），"
              << std::endl;
    std::cout << "目標位址在執行期才確定，因此無法 inline。" << std::endl;
    std::cout << "概念12.cpp 的模板版則被完全 inline 成兩道 mulsd 乘法指令。"
              << std::endl;

    std::cout << "\n=== 執行期多型換到了什麼 ===" << std::endl;
    std::cout << "1) 異質容器：不同型別統一走訪" << std::endl;
    std::cout << "2) 晚期綁定：依設定檔／外掛決定實際型別" << std::endl;
    std::cout << "3) 跨 ABI 邊界：實作可以放在編譯時還不存在的 .so 裡"
              << std::endl;
    std::cout << "這三件事模板完全做不到 —— 模板要求型別在編譯期就已知。"
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第二課：泛型編程（Generic Programming）概念11.cpp -o concept11

// （本機 g++ 15.2 / x86-64 實測值；vtable 非標準規定的機制）

// === 預期輸出 ===
// === 異質容器：不同型別放進同一個 vector ===
// Area: 78.5397
// Area: 12
// 這是模板做不到的事 —— 見概念12.cpp 的對照。
//
// === vptr 的空間成本 ===
// sizeof(double)      = 8
// sizeof(PlainCircle) = 8   <- 只有一個 double，無 virtual
// sizeof(Circle)      = 16   <- 同樣一個 double，但多了 vptr
//
// === 分派方式 ===
// shape->area() 編譯後是間接跳躍（本機 -O2 組語：jmp *%rax），
// 目標位址在執行期才確定，因此無法 inline。
// 概念12.cpp 的模板版則被完全 inline 成兩道 mulsd 乘法指令。
//
// === 執行期多型換到了什麼 ===
// 1) 異質容器：不同型別統一走訪
// 2) 晚期綁定：依設定檔／外掛決定實際型別
// 3) 跨 ABI 邊界：實作可以放在編譯時還不存在的 .so 裡
// 這三件事模板完全做不到 —— 模板要求型別在編譯期就已知。
