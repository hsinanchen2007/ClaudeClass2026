// =============================================================================
//  第 6 課：什麼是面向對象？核心思想介紹 — summary.cpp
//  OOP 四大支柱總整理：封裝 / 繼承 / 多型 / 抽象
// =============================================================================
//
// 【主題資訊 Information】
//   封裝 Encapsulation：private/protected/public + 存取器，維護不變量
//   繼承 Inheritance   ：class D : public B —— is-a 關係，重用結構
//   多型 Polymorphism  ：virtual + override —— 執行期依實際型別分派
//   抽象 Abstraction   ：純虛擬函式 = 0 —— 定義契約、隱藏實作
//   標準版本：四者的核心語法皆 C++98；override / final 為 C++11
//   標頭檔  ：語言核心特性；本檔另用 <iostream> <string> <vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 四大支柱其實只在回答一個問題：如何控制複雜度】
//   把它們理解成四種不同的「隔離手段」會比死背定義有用得多：
//     封裝 —— 隔離【資料】。外界碰不到內部狀態，
//              類別因此能保證自己永遠合法（本檔 Student 的 GPA 範圍檢查）。
//     繼承 —— 隔離【共通與差異】。共通部分寫在基類，只寫一次。
//     多型 —— 隔離【呼叫端與實作】。呼叫端不必知道對方是誰。
//     抽象 —— 隔離【介面與實作】。契約寫在基類，細節留給派生類。
//   四者常被一起使用，但解決的是不同層面的問題，
//   面試時能講清楚「各自隔離了什麼」比背四個名詞有價值。
//
// 【2. 封裝的重點不是「藏起來」，而是「維護不變量」】
//   本檔的 Student::setGPA 會拒絕 -1 這種非法值。
//   如果 gpa 是 public，任何一行程式碼都能讓物件進入非法狀態，
//   而且出問題時無從追查是誰改的。
//   把資料設為 private + 提供唯一入口之後，
//   「GPA 必定落在 0.0~4.0」就成為型別本身的保證 ——
//   這才是封裝的價值。private 只是手段，不變量才是目的。
//   （這也是本檔加入的 LeetCode 1603 要示範的重點。）
//
// 【3. 多型的代價與界線】
//   多型不是免費的：每個物件多一個 vptr（本機 8 bytes），
//   虛擬呼叫無法 inline，連帶失去後續最佳化。
//   更重要的是設計上的代價 —— 一旦公開了虛擬介面，
//   它就成了必須長期維護的契約，改動會波及所有派生類。
//   判準：只有當「同一個呼叫端要處理多種、且未來可能增加的型別」時，
//   多型才值得。只有兩種且永遠不會變的情況，if-else 反而更清楚。
//
// 【4. 本檔用裸指標與 new/delete，這是教學簡化】
//   多型示範段落用了 vector<Shape*> 與手動 delete。
//   真實程式碼應該用智慧指標：
//       std::vector<std::unique_ptr<Shape>> shapes;
//       shapes.push_back(std::make_unique<Circle>(5.0, "紅色"));
//   如此就不需要手動 delete，即使中途丟出例外也不會洩漏。
//   本檔保留裸指標是為了讓「多型」這個主題不被記憶體管理干擾，
//   但請不要把它當成資源管理的範本（智慧指標為 C++11 起）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 vector<Shape> 不能用來裝多型物件
//   vector<Shape> 存的是【值】，每個元素大小固定為 sizeof(Shape)。
//   把 Circle 放進去會發生【物件切片（object slicing）】——
//   只有基類子物件被複製，Circle 特有的成員與 vptr 全部遺失，
//   之後呼叫 area() 會執行基類版本。
//   這是編譯得過、執行不報錯、但行為完全錯誤的經典陷阱。
//   多型容器【必須】存指標或參考（實務上是 unique_ptr）。
//
// (B) 抽象類別為什麼可以有建構子
//   Vehicle 是抽象類別、不能實體化，但它【一定】會被建構 ——
//   每次建立 Car 物件時，Vehicle 子物件都是其中的一部分。
//   所以抽象基類需要建構子來初始化自己的資料成員（本檔的 brand），
//   慣例上宣告為 protected 表示「只給派生類用」。
//
// (C) 四大支柱在 C++ 中都有「非 OOP」的替代方案
//   C++ 是多典範語言，不必凡事都用類別階層：
//     多型 → template + concept（編譯期多型，零成本）
//     抽象 → std::function（型別抹除，不需要繼承）
//     繼承 → 組合 + 委派（多數情況更好維護）
//   「Prefer composition over inheritance」是被反覆驗證的經驗法則。
//   本課先建立 OOP 的完整圖像，之後才有能力判斷什麼時候【不要】用它。
//
// 【注意事項 Pay Attention】
//   1. 多型容器不能存值（vector<Shape>），否則會發生物件切片。
//      必須存指標或 unique_ptr。
//   2. 多型基類【必須】有 virtual 解構子，否則透過基類指標 delete 是 UB。
//   3. 本檔用裸 new/delete 是教學簡化，實務請用 unique_ptr（C++11）。
//   4. 抽象類別可以有建構子、資料成員與已實作的函式，
//      它不等於「純介面」。
//   5. 覆寫一律加 override，避免簽名寫錯變成新多載。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】OOP 四大支柱
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 封裝的目的是什麼？只是「把資料藏起來」嗎？
//     答：藏起來只是手段，目的是【維護不變量】。
//         本檔 Student 的 GPA 必須落在 0.0~4.0；
//         只要 gpa 是 private 且唯一入口是 setGPA，
//         這個規則就成為型別本身的保證，任何程式碼都無法違反。
//         若是 public，規則就只能靠「大家記得遵守」，
//         而且出錯時無從追查是誰改的。
//     追問：那寫一堆 getter/setter 把每個成員都包一層，算封裝嗎？
//           → 不算，那只是換個語法的 public。
//             真正的封裝是【對外暴露有意義的操作】而不是欄位存取 ——
//             例如 deposit(amount) 而不是 setBalance(x)，
//             前者能檢查金額為正、能記錄交易，後者什麼都保證不了。
//
// 🔥 Q2. 為什麼多型容器要用 vector<Shape*> 而不能用 vector<Shape>？
//     答：因為 vector<Shape> 存的是值，每個元素大小固定為 sizeof(Shape)。
//         放入 Circle 時會發生【物件切片】：
//         只有基類子物件被複製過去，派生類的成員與 vptr 全部遺失，
//         之後呼叫 area() 執行的是基類版本。
//         多型必須透過指標或參考才能運作。
//     追問：那實務上該用什麼？
//           → std::vector<std::unique_ptr<Shape>>（C++11）。
//             既保有多型，又不需要手動 delete，
//             即使中途丟出例外也不會洩漏。
//
// ⚠️ 陷阱. 「這兩個類別有很多重複的程式碼，讓其中一個繼承另一個
//          就能消除重複」—— 這個推理錯在哪？
//     答：錯在把繼承當成消除重複的工具。
//         public 繼承的意義是 is-a：承諾派生類能在【任何】用到基類的
//         地方頂替基類（Liskov 替換原則）。
//         「碰巧有重複程式碼」和「概念上是同一種東西」是兩回事。
//         強行繼承會讓基類的整個介面變成派生類的介面，
//         日後基類新增一個方法，所有派生類都被迫承擔它。
//     為什麼會錯：因為繼承【確實】能消除重複，所以看起來有效。
//         但它同時偷偷建立了一個型別上的承諾，
//         而這個承諾的代價要到很久以後才會顯現 ——
//         通常是在某個派生類必須「覆寫一個它根本不該有的方法」
//         並在裡面丟例外的時候。
//         要消除重複，正解是【組合】或把共通邏輯抽成自由函式；
//         繼承只保留給真正的 is-a 關係。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第 6 課：什麼是面向對象？核心思想介紹】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 面向對象程式設計（OOP）的誕生動機
 * 2. 封裝（Encapsulation）—— 隱藏資料、控制存取
 * 3. 繼承（Inheritance）—— 程式碼重用與擴展
 * 4. 多型（Polymorphism）—— 同一介面、不同行為
 * 5. 抽象（Abstraction）—— 只暴露必要細節
 * 6. OOP vs 程序式程式設計的比較
 * ================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
using namespace std;

// ================================================================
// 重點一：OOP 的誕生動機
// ================================================================
// 程序式（C 風格）：資料與函數分離，無法保護資料
// C++ OOP 目標：把資料與操作綁在一起，並控制存取
//
// C 語言風格的問題：
//   struct Student { char name[50]; float gpa; };
//   void setGpa(Student* s, float g) { s->gpa = g; }  // 任何人都可亂改
//
// OOP 解決方案：將資料「封裝」在類別中，只透過受控的介面修改

// ================================================================
// 重點二：封裝（Encapsulation）
// ================================================================
// 核心：把資料（member variables）和操作（member functions）封裝在 class 中
// private 成員外界不能直接存取；public 成員是對外介面
// 好處：可在 setter 中加入驗證邏輯，防止非法資料

class Student {
private:
    string name;   // 私有：外界無法直接存取
    float gpa;     // 私有：必須透過 setGpa 修改

public:
    // 建構函數
    Student(const string& n, float g) : name(n), gpa(0.0f) {
        setGpa(g);  // 使用自己的 setter，進行驗證
    }

    // Setter 帶有驗證邏輯 —— 封裝的精髓
    void setGpa(float newGpa) {
        if (newGpa >= 0.0f && newGpa <= 4.0f) {
            gpa = newGpa;
        } else {
            cout << "[錯誤] GPA 必須在 0.0~4.0 之間，拒絕設定：" << newGpa << endl;
        }
    }

    // Getter
    float getGpa() const { return gpa; }
    const string& getName() const { return name; }

    void print() const {
        cout << "學生：" << name << "，GPA：" << gpa << endl;
    }
};

// ================================================================
// 重點三：繼承（Inheritance）
// ================================================================
// 語法：class 子類 : public 父類 { ... };
// 子類「繼承」父類所有 public/protected 成員
// 目的：程式碼重用，避免重複撰寫相同邏輯
// 子類可以新增自己的屬性和方法

// 父類（基類）
class Animal {
public:
    string name;

    // 建構函數
    Animal(const string& n) : name(n) {}

    // 父類的通用行為
    void eat() {
        cout << name << " 正在吃東西" << endl;
    }

    void sleep() {
        cout << name << " 正在睡覺" << endl;
    }
};

// 子類：繼承 Animal 的一切，並新增自己的行為
class Dog : public Animal {
public:
    Dog(const string& n) : Animal(n) {}  // 呼叫父類建構函數

    // Dog 特有的行為
    void bark() {
        cout << name << " 汪汪叫！" << endl;
    }
};

class Cat : public Animal {
public:
    Cat(const string& n) : Animal(n) {}

    // Cat 特有的行為
    void meow() {
        cout << name << " 喵喵叫！" << endl;
    }
};

// ================================================================
// 重點四：多型（Polymorphism）
// ================================================================
// 「多型」= 同一個介面，根據實際物件類型呼叫不同實作
// 實現方式：virtual（虛函數）+ override（覆寫）
// 必須透過「指標或參考」才能觸發多型行為
//
// virtual 關鍵字的作用：
//   - 告訴編譯器「這個函數可能被子類覆寫」
//   - 呼叫時根據「實際物件類型」動態決定呼叫哪個版本（動態分派）
//
// override 關鍵字（C++11）：
//   - 明確標示「這個函數是覆寫父類的 virtual 函數」
//   - 若簽名不符合父類，編譯器會報錯（防止拼錯名稱）

class Shape {
public:
    string color;

    Shape(const string& c = "白色") : color(c) {}

    // virtual 讓子類可以覆寫此函數
    virtual double area() const {
        return 0.0;
    }

    // virtual 函數：顯示資訊（可被覆寫）
    virtual void describe() const {
        cout << color << "的形狀，面積 = " << area() << endl;
    }

    // 虛解構函數：透過基類指標刪除子類物件時，確保子類解構函數被呼叫
    virtual ~Shape() {}
};

class Circle : public Shape {
private:
    double radius;

public:
    Circle(double r, const string& c = "紅色")
        : Shape(c), radius(r) {}

    // override 明確標示覆寫
    double area() const override {
        return 3.14159 * radius * radius;
    }

    void describe() const override {
        cout << color << "的圓形，半徑=" << radius
             << "，面積=" << area() << endl;
    }
};

class Rectangle : public Shape {
private:
    double width, height;

public:
    Rectangle(double w, double h, const string& c = "藍色")
        : Shape(c), width(w), height(h) {}

    double area() const override {
        return width * height;
    }

    void describe() const override {
        cout << color << "的矩形，" << width << "x" << height
             << "，面積=" << area() << endl;
    }
};

// 多型的核心展示：接受 Shape* 的函數，可以處理任何子類
void printShapeInfo(const Shape* s) {
    // 根據實際物件類型，動態呼叫正確的 describe() 和 area()
    s->describe();
}

// ================================================================
// 重點五：抽象（Abstraction）
// ================================================================
// 抽象 = 隱藏複雜的實現細節，只暴露「需要知道的」介面
// 純虛函數（pure virtual）：= 0，使類別成為「抽象類別」
// 抽象類別不能被實例化，只能作為基類使用

class Vehicle {  // 抽象類別（包含純虛函數）
public:
    string brand;
    Vehicle(const string& b) : brand(b) {}

    // 純虛函數 = 0：子類「必須」實作這個函數
    virtual void start() = 0;
    virtual void stop() = 0;

    // 非純虛函數：有預設實作，子類可以選擇覆寫
    virtual void honk() {
        cout << brand << "：嗶嗶！" << endl;
    }

    virtual ~Vehicle() {}
};

class Car : public Vehicle {
public:
    Car(const string& b) : Vehicle(b) {}

    void start() override {
        cout << brand << " 汽車：引擎發動，轟轟轟！" << endl;
    }

    void stop() override {
        cout << brand << " 汽車：踩煞車，停止。" << endl;
    }
};

class Bicycle : public Vehicle {
public:
    Bicycle(const string& b) : Vehicle(b) {}

    void start() override {
        cout << brand << " 自行車：開始踩踏板！" << endl;
    }

    void stop() override {
        cout << brand << " 自行車：手捏煞車，停止。" << endl;
    }

    void honk() override {
        cout << brand << "：鈴鈴！" << endl;
    }
};

// ================================================================
// 重點六：OOP 四大特性總結對照
// ================================================================
//
// ┌──────────┬─────────────────────────────────────────────────────┐
// │ 特性     │ 說明                                                 │
// ├──────────┼─────────────────────────────────────────────────────┤
// │ 封裝     │ 資料 + 函數綁在一起；private/public 控制存取         │
// │ 繼承     │ 子類繼承父類成員；實現程式碼重用                     │
// │ 多型     │ virtual + override；同一介面，不同行為               │
// │ 抽象     │ 純虛函數；隱藏細節，只暴露必要介面                   │
// └──────────┴─────────────────────────────────────────────────────┘


// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1603. Design Parking System
//   題目：設計停車系統。建構子給定大／中／小三種車位的數量；
//         addCar(carType) 嘗試停入一台車（1=大 2=中 3=小），成功回 true。
//   為什麼用到本主題：這題考的正是本課的【封裝】——
//         三個車位計數必須是 private，只能透過 addCar 這個唯一入口修改。
//         若把計數暴露成 public，外部就能把剩餘車位改成負數，
//         類別再也無法維護「車位數不得為負」這個不變量。
//   複雜度：建構 O(1)、addCar O(1)。
// -----------------------------------------------------------------------------
class ParkingSystem {
private:
    int slots_[4];              // 索引 1..3 對應大／中／小，索引 0 不用
public:
    ParkingSystem(int big, int medium, int small) : slots_{0, big, medium, small} {}

    bool addCar(int carType) {
        if (carType < 1 || carType > 3) return false;   // 防呆：型別不合法
        if (slots_[carType] <= 0) return false;         // 沒位子了
        --slots_[carType];
        return true;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】用抽象介面統一多種通知管道（Email / SMS）
//   情境：系統要在事件發生時通知使用者，管道可能隨時增加
//         （未來還會有 Slack、Line、Webhook…）。
//   為什麼用到本主題：把「通知」抽象成純虛擬介面之後，
//         業務邏輯 notifyAll() 只依賴介面，
//         新增管道完全不需要修改既有程式碼（開放封閉原則）。
// -----------------------------------------------------------------------------
class Notifier {
public:
    virtual void send(const string& msg) = 0;
    virtual ~Notifier() {}
};

class EmailNotifier : public Notifier {
    string addr_;
public:
    explicit EmailNotifier(string a) : addr_(std::move(a)) {}
    void send(const string& msg) override {
        cout << "  [Email -> " << addr_ << "] " << msg << endl;
    }
};

class SmsNotifier : public Notifier {
    string phone_;
public:
    explicit SmsNotifier(string p) : phone_(std::move(p)) {}
    void send(const string& msg) override {
        cout << "  [SMS   -> " << phone_ << "] " << msg << endl;
    }
};

// 業務邏輯只認識 Notifier 這個抽象，不知道有哪些具體管道
void notifyAll(const vector<Notifier*>& channels, const string& msg) {
    for (Notifier* c : channels) c->send(msg);
}

int main() {
    cout << "========================================" << endl;
    cout << "   第 6 課：面向對象核心思想展示" << endl;
    cout << "========================================" << endl;

    // --- 封裝示範 ---
    cout << "\n【封裝 Encapsulation】" << endl;
    Student s1("張三", 3.8f);
    s1.print();
    s1.setGpa(-1.0f);  // 非法值，會被攔截
    s1.setGpa(3.9f);   // 合法值
    s1.print();

    // --- 繼承示範 ---
    cout << "\n【繼承 Inheritance】" << endl;
    Dog dog("旺財");
    Cat cat("咪咪");

    dog.eat();   // 繼承自 Animal
    dog.bark();  // Dog 自己的行為

    cat.eat();   // 繼承自 Animal
    cat.meow();  // Cat 自己的行為

    // --- 多型示範 ---
    cout << "\n【多型 Polymorphism】" << endl;
    // 用父類指標儲存子類物件 —— 多型的關鍵
    vector<Shape*> shapes;
    shapes.push_back(new Circle(5.0, "紅色"));
    shapes.push_back(new Rectangle(4.0, 6.0, "藍色"));
    shapes.push_back(new Circle(3.0, "綠色"));

    for (const Shape* s : shapes) {
        printShapeInfo(s);  // 動態分派：根據實際類型呼叫正確的函數
    }

    // 釋放記憶體（虛解構函數確保正確刪除）
    for (Shape* s : shapes) {
        delete s;
    }

    // --- 抽象示範 ---
    cout << "\n【抽象 Abstraction】" << endl;
    // Vehicle v("X");  // 編譯錯誤！抽象類別不能實例化
    Car car("Toyota");
    Bicycle bike("Giant");

    // 透過基類指標使用（多型 + 抽象）
    vector<Vehicle*> vehicles = { &car, &bike };
    for (Vehicle* v : vehicles) {
        v->start();
        v->honk();
        v->stop();
        cout << endl;
    }


    // --- LeetCode 1603：封裝的實戰 ---
    cout << "\n【LeetCode 1603. Design Parking System】" << endl;
    ParkingSystem ps(1, 1, 0);          // 大 1 位、中 1 位、小 0 位
    cout << "  停大車(1) -> " << (ps.addCar(1) ? "true" : "false") << endl;
    cout << "  停中車(2) -> " << (ps.addCar(2) ? "true" : "false") << endl;
    cout << "  停小車(3) -> " << (ps.addCar(3) ? "true" : "false") << "  (小車位為 0)" << endl;
    cout << "  再停大車(1) -> " << (ps.addCar(1) ? "true" : "false") << "  (大車位已滿)" << endl;

    // --- 日常實務：抽象介面統一通知管道 ---
    cout << "\n【日常實務：多管道通知】" << endl;
    EmailNotifier mail("ops@example.com");
    SmsNotifier   sms("0912-345-678");
    vector<Notifier*> channels = { &mail, &sms };
    notifyAll(channels, "磁碟使用率已達 92%");

    cout << "========================================" << endl;
    cout << " OOP 四大支柱：封裝、繼承、多型、抽象 " << endl;
    cout << "========================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o oo_summary
//   本檔可執行部分只用到 C++98/C++11 的語法（override 為 C++11），
//   以 -std=c++17 編譯零警告通過。

// 註 1:本檔輸出是【完全確定的】—— 沒有輸入、亂數、執行緒或位址。
//      連跑 5 次位元組完全相同。

// 註 2:「面積=78.5397」「面積=28.2743」是 3.14159×r² 經
//      iostream 預設 6 位【有效數字】格式化的結果；
//      24 則是 4×6 的精確值（預設格式會去掉尾隨的零）。

// 註 3:封裝示範中「[錯誤] GPA 必須在 0.0~4.0 之間，拒絕設定：-1」
//      這一行正是封裝的價值 —— 非法值被擋在類別邊界之外，
//      物件始終維持在合法狀態。

// 註 4:本檔的多型示範用了裸 new/delete 與 vector<Shape*>，
//      這是為了聚焦在多型本身而做的教學簡化。
//      實務請用 std::vector<std::unique_ptr<Shape>>（C++11），
//      既保有多型又不必手動管理生命週期。

// === 預期輸出 ===
// ========================================
//    第 6 課：面向對象核心思想展示
// ========================================
//
// 【封裝 Encapsulation】
// 學生：張三，GPA：3.8
// [錯誤] GPA 必須在 0.0~4.0 之間，拒絕設定：-1
// 學生：張三，GPA：3.9
//
// 【繼承 Inheritance】
// 旺財 正在吃東西
// 旺財 汪汪叫！
// 咪咪 正在吃東西
// 咪咪 喵喵叫！
//
// 【多型 Polymorphism】
// 紅色的圓形，半徑=5，面積=78.5397
// 藍色的矩形，4x6，面積=24
// 綠色的圓形，半徑=3，面積=28.2743
//
// 【抽象 Abstraction】
// Toyota 汽車：引擎發動，轟轟轟！
// Toyota：嗶嗶！
// Toyota 汽車：踩煞車，停止。
//
// Giant 自行車：開始踩踏板！
// Giant：鈴鈴！
// Giant 自行車：手捏煞車，停止。
//
//
// 【LeetCode 1603. Design Parking System】
//   停大車(1) -> true
//   停中車(2) -> true
//   停小車(3) -> false  (小車位為 0)
//   再停大車(1) -> false  (大車位已滿)
//
// 【日常實務：多管道通知】
//   [Email -> ops@example.com] 磁碟使用率已達 92%
//   [SMS   -> 0912-345-678] 磁碟使用率已達 92%
// ========================================
//  OOP 四大支柱：封裝、繼承、多型、抽象
// ========================================
