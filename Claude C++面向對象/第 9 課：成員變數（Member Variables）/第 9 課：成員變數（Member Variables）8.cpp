// =============================================================================
//  第 9 課：成員變數（Member Variables）8.cpp  —  綜合實作：學生成績管理
// =============================================================================
//
//  ⚠️ 檔案結構說明：本檔【前 584 行是一個 /* */ 註解區塊】（本課完整講義），
//     裡面有多個示範用的 class 與 int main()，那些程式碼【不會被編譯】。
//     真正會編譯的程式從第 592 行的 class Student 開始。
//     搜尋 Student / main 時請以【最後一次出現】為準。
//
// 【主題資訊 Information】
//   本檔整合第 9 課全部主題：
//       內建型別成員   int id = 0;
//       類別型別成員   string name = "未命名";
//       陣列成員       double scores[5] = {0, 0, 0, 0, 0};   ← 陣列也能類內初始化
//       輔助狀態成員   int scoreCount = 0;                    ← 與 scores 必須同步維護
//   標準版本：類內初始值為 C++11；其餘 C++98
//   複雜度：addScore O(1)；average / highest / lowest 各為 O(scoreCount)
//   標頭檔：<iostream>、<string>
//
// 【詳細解釋 Explanation】
//
// 【1. scores 與 scoreCount 是一組「必須同步維護」的成員】
//   這是本檔最值得學的設計重點。
//   scores[5] 是固定大小的陣列，但實際用了幾格由 scoreCount 決定。
//   兩者必須【永遠一致】：
//     • addScore 成功時，一定要同時寫入 scores 並遞增 scoreCount
//     • average / highest / lowest 一定要用 scoreCount 而非 5 當上界
//   若哪個函式不小心用了 5，就會把尚未填入的 0 也算進去 ——
//   平均分數立刻錯誤。
//   把這兩個成員綁在同一個類別、只透過成員函式修改，
//   「同步性」才成為類別自己的責任而非使用者的紀律。
//
// 【2. 三個檢查各自防的是什麼】
//       if (scoreCount >= 5)              防【緩衝區溢位】
//       if (score < 0 || score > 100)     防【不合理的資料】
//       if (scoreCount == 0) return 0.0;  防【除以零】與空陣列存取
//   第一個尤其重要：如果沒有它，addScore 第六次呼叫就會寫入 scores[5]，
//   那是陣列邊界之外 —— 未定義行為，而且會【安靜地】覆寫掉
//   緊鄰其後的 scoreCount 成員（依記憶體佈局而定）。
//   本檔的輸出中「未設定的學生」那一段，
//   平均/最高/最低全部回傳 0.0，正是第三個檢查在發揮作用。
//
// 【3. 陣列成員的類內初始化】
//       double scores[5] = {0, 0, 0, 0, 0};
//   陣列也可以有類內初始值（C++11）。
//   其實只要寫 double scores[5] = {}; 就會全部歸零
//   —— 大括號內不足的部分一律【值初始化】。
//   對照同課 4 號檔的 Matrix2x2（沒有初始值），
//   這一行就是「不可能讀到垃圾值」的保證。
//
// 【4. 這個設計的真實限制】
//   固定 5 科是硬性上限，改成 6 科要動到三個地方
//   （陣列宣告、addScore 的檢查、以及所有隱含 5 的假設）。
//   實務上應該用 std::vector<double>：
//     • 不需要 scoreCount（用 scores.size()）
//     • 沒有上限，也不可能溢位
//     • 兩個成員合而為一，同步問題自然消失
//   本檔用原生陣列是為了示範「陣列成員」這個主題本身。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼「兩個成員必須同步」是設計氣味
//   scores 與 scoreCount 之間存在【不變量】：
//   scoreCount 必須恰好等於已填入的元素數。
//   任何需要人為維護的不變量都是 bug 的溫床。
//   換成 std::vector 之後這個不變量由容器自己保證，
//   類別的程式碼反而變少了。
//   一般原則是：能讓標準庫維護的不變量，就不要自己維護。
//
// (B) 本機實測的物件大小
//   sizeof(Student) = 88（x86-64 / g++ 15.2.0，屬實作定義）：
//       string name    32
//       int    id       4 + 【填充 4】（double 陣列需 8 bytes 對齊）
//       double scores[5] 40
//       int    scoreCount 4 + 【尾端填充 4】
//   注意即使只輸入了 3 科成績，這 88 bytes 也一分不少 ——
//   固定陣列的空間是預先配置好的。
//   改用 vector 的話 sizeof 會固定為 32 + 4 + 24 + 填充，
//   但實際資料在堆積上、按需成長。
//
// (C) average() 的浮點累加順序
//   average() 依序累加再除以個數。
//   對 5 個 0~100 的數值而言精度完全足夠，
//   但若要累加數百萬個浮點數，順序會顯著影響誤差
//   （小數一直加進大數會被吃掉）。
//   數值計算上有 Kahan summation 這類補償演算法，
//   標準庫則可用 std::accumulate 搭配適當的初值型別。
//
// 【注意事項 Pay Attention】
//   1. 本檔前 584 行是註解區塊，其中的 class 與 main 都不會被編譯。
//   2. scores 與 scoreCount 必須同步維護 ——
//      所有走訪迴圈都要用 scoreCount 當上界，不能用 5。
//   3. addScore 的 scoreCount >= 5 檢查防的是陣列越界（UB），不可省略。
//   4. 陣列成員也能類內初始化（C++11）；double scores[5] = {}; 即可全部歸零。
//   5. sizeof(Student) 是固定的（本機 88），與實際輸入幾科無關。
//   6. 實務應改用 std::vector<double>，可同時消除上限與同步問題。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】多個成員的同步維護
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. average() 為什麼要用 scoreCount 當迴圈上界，而不是直接用 5？
//     答：因為 scores[5] 是【固定配置】的，但實際填入幾科由 scoreCount 決定。
//         若用 5 當上界，尚未填入的位置（值為 0）也會被算進去 ——
//         輸入 3 科各 90 分，平均會變成 (90+90+90+0+0)/5 = 54 而不是 90。
//         這正是 scores 與 scoreCount 這組成員必須同步維護的原因。
//     追問：怎麼從根本消除這個風險？
//           → 改用 std::vector<double>：
//             size() 永遠等於實際元素數，不可能不同步，
//             也不需要額外的 scoreCount 成員。
//             能讓標準庫維護的不變量就不要自己維護。
//
// 🔥 Q2. 如果拿掉 addScore 裡的 if (scoreCount >= 5) 檢查會怎樣？
//     答：第六次呼叫會寫入 scores[5]，那是陣列邊界之外 ——
//         【未定義行為】。而且它不會立刻崩潰，
//         而是安靜地覆寫緊鄰其後的記憶體
//         （依本機佈局很可能就是 scoreCount 本身），
//         造成「寫入一個成績卻把計數器改成奇怪的值」這種
//         看起來毫無關聯的症狀。
//     追問：原生陣列有邊界檢查嗎？
//           → 沒有。scores[100] 一樣編譯通過。
//             std::array 的 operator[] 同樣不檢查，
//             但它提供 at() 會丟 std::out_of_range；
//             vector 也是如此。要檢查就必須明確選擇 at()。
//
// ⚠️ 陷阱. 「這個類別的所有成員都有類內初始值，
//          所以任何 Student 物件都一定處於正確的狀態」—— 錯在哪？
//     答：類內初始值保證的是「不會讀到未初始化的垃圾值」，
//         這確實消除了 UB。但它【不保證狀態有意義】——
//         剛建立的 Student 叫「未命名」、ID 是 0、沒有任何成績，
//         那並不是一個真實存在的學生。
//         本檔輸出最後那段「未設定的學生」就是這個狀態，
//         平均、最高、最低全是 0，等級是 F。
//     為什麼會錯：把「已初始化」與「有效」混為一談。
//         這兩者是不同層次的保證：
//           已初始化 → 沒有 UB（類內初始值就能做到）
//           有效     → 滿足業務上的不變量（只有建構子能做到）
//         真正的封裝要求物件【一建立就有意義】，
//         做法是提供建構子並移除預設建構子：
//             Student(std::string n, int i) : name(std::move(n)), id(i) {
//                 if (i <= 0) throw std::invalid_argument("學號必須為正");
//             }
//         如此就不可能存在「未命名、ID 0」這種半成品物件。
//         類內初始值與建構子是互補的：
//         前者防 UB，後者防無意義的狀態。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第 9 課：成員變數（Member Variables）

## 一、成員變數的本質

成員變數是定義在類別內部的變數，用來描述對象的**狀態（state）**。每個對象擁有自己獨立的一份成員變數副本，這點在上一課已經看過。

本課我們要深入探討：成員變數有哪些類型？如何正確初始化？未初始化會發生什麼？

---

## 二、成員變數的種類

### 2.1 基本型別作為成員變數

```cpp
#include <iostream>
using namespace std;

class Sensor {
public:
    int id;
    double temperature;
    bool isActive;
    char grade;

    void show() {
        cout << "ID: " << id << endl;
        cout << "溫度: " << temperature << " °C" << endl;
        cout << "啟用: " << (isActive ? "是" : "否") << endl;
        cout << "等級: " << grade << endl;
    }
};

int main() {
    Sensor s;
    s.id = 1001;
    s.temperature = 36.5;
    s.isActive = true;
    s.grade = 'A';

    s.show();

    return 0;
}
```

**預期輸出：**
```
ID: 1001
溫度: 36.5 °C
啟用: 是
等級: A
```

---

### 2.2 字串和容器作為成員變數

```cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Classroom {
public:
    string teacherName;
    string roomNumber;
    vector<string> students;  // 動態陣列作為成員

    void addStudent(const string& name) {
        students.push_back(name);
    }

    void show() {
        cout << "教室: " << roomNumber << endl;
        cout << "老師: " << teacherName << endl;
        cout << "學生 (" << students.size() << " 人):" << endl;
        for (int i = 0; i < students.size(); i++) {
            cout << "  " << (i + 1) << ". " << students[i] << endl;
        }
    }
};

int main() {
    Classroom room;
    room.teacherName = "王老師";
    room.roomNumber = "A-301";

    room.addStudent("小明");
    room.addStudent("小華");
    room.addStudent("小美");

    room.show();

    return 0;
}
```

**預期輸出：**
```
教室: A-301
老師: 王老師
學生 (3 人):
  1. 小明
  2. 小華
  3. 小美
```

**重點**：`string` 和 `vector` 這種類別型別的成員變數，會自動呼叫自己的預設建構函數來初始化。`string` 預設是空字串 `""`，`vector` 預設是空容器。所以它們不需要你手動初始化就是安全的。

---

### 2.3 另一個類別作為成員變數（組合）

對象裡面可以包含另一個對象：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Engine {
public:
    int horsepower = 0;
    string fuelType = "汽油";

    void start() {
        cout << horsepower << " 匹馬力的" << fuelType << "引擎啟動！" << endl;
    }
};

class Car {
public:
    string brand;
    Engine engine;  // Car「擁有」一個 Engine —— 這就是組合（composition）

    void drive() {
        cout << brand << " 開始行駛" << endl;
        engine.start();
    }
};

int main() {
    Car car;
    car.brand = "Toyota";
    car.engine.horsepower = 150;    // 存取內層對象的成員
    car.engine.fuelType = "油電混合";

    car.drive();

    return 0;
}
```

**預期輸出：**
```
Toyota 開始行駛
150 匹馬力的油電混合引擎啟動！
```

**注意語法**：`car.engine.horsepower` — 先存取 `car` 的 `engine` 成員，再存取 `engine` 的 `horsepower` 成員。這是一層層地「進入」內部對象。

---

### 2.4 陣列作為成員變數

```cpp
#include <iostream>
using namespace std;

class Matrix2x2 {
public:
    double data[2][2];  // 固定大小的陣列作為成員

    void set(double a, double b, double c, double d) {
        data[0][0] = a;  data[0][1] = b;
        data[1][0] = c;  data[1][1] = d;
    }

    void print() {
        cout << "| " << data[0][0] << "  " << data[0][1] << " |" << endl;
        cout << "| " << data[1][0] << "  " << data[1][1] << " |" << endl;
    }

    double determinant() {
        return data[0][0] * data[1][1] - data[0][1] * data[1][0];
    }
};

int main() {
    Matrix2x2 m;
    m.set(3, 7, 1, 5);

    m.print();
    cout << "行列式 = " << m.determinant() << endl;

    return 0;
}
```

**預期輸出：**
```
| 3  7 |
| 1  5 |
行列式 = 8
```

---

## 三、未初始化的危險

這是本課最重要的部分。**基本型別的成員變數如果不初始化，其值是未定義的（garbage value）。**

```cpp
#include <iostream>
using namespace std;

class Dangerous {
public:
    int x;
    double y;
    bool flag;
};

int main() {
    Dangerous d;

    // 以下輸出是「垃圾值」—— 每次執行可能不同！
    cout << "x    = " << d.x << endl;
    cout << "y    = " << d.y << endl;
    cout << "flag = " << d.flag << endl;

    return 0;
}
```

**可能的輸出（每次可能不同）：**
```
x    = -858993460
y    = 6.95322e-310
flag = 204
```

這些值是棧記憶體上殘留的垃圾資料。使用未初始化的變數是 **未定義行為（Undefined Behavior, UB）**，可能導致程式崩潰、產生錯誤結果，或在不同編譯器/平台上表現不同。

### 哪些會自動初始化，哪些不會？

```cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class InitTest {
public:
    // === 不會自動初始化（危險！） ===
    int a;           // 垃圾值
    double b;        // 垃圾值
    bool c;          // 垃圾值
    char d;          // 垃圾值
    int* ptr;        // 野指標！

    // === 會自動初始化（安全） ===
    string name;            // 自動初始化為 ""
    vector<int> numbers;    // 自動初始化為空容器
};

int main() {
    InitTest t;

    cout << "--- 不安全的成員（垃圾值）---" << endl;
    cout << "a   = " << t.a << endl;
    cout << "b   = " << t.b << endl;
    cout << "c   = " << t.c << endl;
    // t.ptr 是野指標，千萬別 dereference！

    cout << "\n--- 安全的成員（自動初始化）---" << endl;
    cout << "name     = [" << t.name << "] (空字串)" << endl;
    cout << "numbers 大小 = " << t.numbers.size() << endl;

    return 0;
}
```

**規則總結**：

| 成員型別 | 是否自動初始化 | 預設值 |
|---------|--------------|--------|
| `int`, `double`, `char`, `bool` | ❌ 否 | 垃圾值 |
| 指標（`int*`, `char*` 等） | ❌ 否 | 野指標 |
| `string` | ✅ 是 | `""` |
| `vector`, `map` 等 STL 容器 | ✅ 是 | 空容器 |
| 其他 class 類型 | ✅ 是 | 呼叫其預設建構函數 |

**底層原因**：基本型別（int, double 等）來自 C 語言傳統，C 語言不做自動初始化以追求效能（零開銷原則）。而 `string`、`vector` 等是 C++ 類別，它們的建構函數會負責初始化自己。

---

## 四、初始化成員變數的方式

### 4.1 類內初始化（C++11 起）—— 推薦！

```cpp
#include <iostream>
#include <string>
using namespace std;

class GameCharacter {
public:
    string name = "未命名";
    int hp = 100;
    int mp = 50;
    double speed = 1.0;
    bool isAlive = true;

    void show() {
        cout << name << " | HP:" << hp << " MP:" << mp
             << " 速度:" << speed
             << " 狀態:" << (isAlive ? "存活" : "陣亡") << endl;
    }
};

int main() {
    GameCharacter c1;           // 全部使用預設值
    c1.show();

    GameCharacter c2;
    c2.name = "戰士";           // 覆蓋部分預設值
    c2.hp = 150;
    c2.show();

    return 0;
}
```

**預期輸出：**
```
未命名 | HP:100 MP:50 速度:1 狀態:存活
戰士 | HP:150 MP:50 速度:1 狀態:存活
```

**這是最推薦的方式**：在宣告成員變數時直接給預設值，簡單明瞭，避免遺漏。

---

### 4.2 在建構函數中初始化（預告）

```cpp
class GameCharacter {
public:
    string name;
    int hp;
    int mp;

    // 建構函數 —— 第 13 課會詳細講
    GameCharacter() {
        name = "未命名";
        hp = 100;
        mp = 50;
    }
};
```

### 4.3 建構函數初始化列表（預告）

```cpp
class GameCharacter {
public:
    string name;
    int hp;
    int mp;

    // 初始化列表 —— 第 16 課會詳細講
    GameCharacter() : name("未命名"), hp(100), mp(50) {
    }
};
```

**目前階段**先掌握 4.1 的類內初始化，建構函數在第 13~16 課會完整講解。

---

## 五、成員變數的存取控制預告

目前我們所有成員變數都放在 `public` 下，外界可以隨意存取：

```cpp
class Player {
public:
    int hp = 100;
};

int main() {
    Player p;
    p.hp = -9999;  // 完全合法，但不合理！
}
```

這是不安全的。在第 11 課（存取修飾符）和第 20~21 課（封裝、getter/setter），我們會學到如何保護成員變數。現在先知道這個問題存在即可。

---

## 六、綜合實戰範例：學生成績管理

```cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
public:
    // 成員變數：各種型別
    string name = "未命名";
    int id = 0;
    double scores[5] = {0, 0, 0, 0, 0};  // 五科成績，陣列也能類內初始化
    int scoreCount = 0;                    // 已輸入的成績數

    // 新增成績
    void addScore(double score) {
        if (scoreCount >= 5) {
            cout << "錯誤：最多只能輸入 5 科成績" << endl;
            return;
        }
        if (score < 0 || score > 100) {
            cout << "錯誤：成績必須在 0~100 之間" << endl;
            return;
        }
        scores[scoreCount] = score;
        scoreCount++;
    }

    // 計算平均
    double average() {
        if (scoreCount == 0) return 0.0;
        double sum = 0;
        for (int i = 0; i < scoreCount; i++) {
            sum += scores[i];
        }
        return sum / scoreCount;
    }

    // 找最高分
    double highest() {
        if (scoreCount == 0) return 0.0;
        double max = scores[0];
        for (int i = 1; i < scoreCount; i++) {
            if (scores[i] > max) {
                max = scores[i];
            }
        }
        return max;
    }

    // 找最低分
    double lowest() {
        if (scoreCount == 0) return 0.0;
        double min = scores[0];
        for (int i = 1; i < scoreCount; i++) {
            if (scores[i] < min) {
                min = scores[i];
            }
        }
        return min;
    }

    // 顯示完整資訊
    void showReport() {
        cout << "================================" << endl;
        cout << "學生: " << name << " (ID: " << id << ")" << endl;
        cout << "成績: ";
        for (int i = 0; i < scoreCount; i++) {
            cout << scores[i];
            if (i < scoreCount - 1) cout << ", ";
        }
        cout << endl;
        cout << "平均: " << average() << endl;
        cout << "最高: " << highest() << endl;
        cout << "最低: " << lowest() << endl;

        // 判定等級
        double avg = average();
        cout << "等級: ";
        if (avg >= 90) cout << "A (優秀)";
        else if (avg >= 80) cout << "B (良好)";
        else if (avg >= 70) cout << "C (中等)";
        else if (avg >= 60) cout << "D (及格)";
        else cout << "F (不及格)";
        cout << endl;
        cout << "================================" << endl;
    }
};

int main() {
    Student s1;
    s1.name = "陳信安";
    s1.id = 2001;
    s1.addScore(92);
    s1.addScore(88);
    s1.addScore(95);
    s1.addScore(78);
    s1.addScore(85);

    s1.showReport();

    cout << endl;

    Student s2;
    s2.name = "小明";
    s2.id = 2002;
    s2.addScore(65);
    s2.addScore(72);
    s2.addScore(58);

    s2.showReport();

    cout << endl;

    // 測試邊界情況
    Student s3;  // 完全使用預設值
    cout << "未設定的學生:" << endl;
    s3.showReport();

    return 0;
}
```

**預期輸出：**
```
================================
學生: 陳信安 (ID: 2001)
成績: 92, 88, 95, 78, 85
平均: 87.6
最高: 95
最低: 78
等級: B (良好)
================================

================================
學生: 小明 (ID: 2002)
成績: 65, 72, 58
平均: 65
最高: 72
最低: 58
等級: D (及格)
================================

未設定的學生:
================================
學生: 未命名 (ID: 0)
成績: 
平均: 0
最高: 0
最低: 0
等級: F (不及格)
================================
```

**這個範例展示了：**
- 多種型別的成員變數（`string`、`int`、`double[]`）
- 類內初始化確保預設值安全
- 成員函數直接存取成員變數進行運算
- 邊界條件處理（成績範圍檢查、空成績處理）
- 未設定任何值時，預設值讓程式依然正常運作

---

## 本課重點回顧

| 概念 | 說明 |
|------|------|
| 成員變數類型 | 基本型別、字串、容器、陣列、其他類別都可以 |
| 未初始化的危險 | 基本型別（int, double 等）不自動初始化，值是垃圾 |
| 自動初始化的型別 | `string`、`vector` 等類別型別會呼叫自己的建構函數 |
| 類內初始化 | C++11 起推薦：`int hp = 100;` 直接在宣告處給預設值 |
| 組合 | 類別可以包含另一個類別作為成員（`Car` 包含 `Engine`） |
| 存取語法 | 外層用 `.` 一層層存取：`car.engine.horsepower` |
| 安全準則 | **所有基本型別的成員變數都應該給預設值** |

---

下一課是 **第 10 課：成員函數（member functions）**，會深入講解成員函數的各種細節，包括函數重載、const 成員函數的預告、以及成員函數如何在背後運作。準備好就告訴我！
*/



#include <iostream>
#include <string>
using namespace std;

class Student {
public:
    // 成員變數：各種型別
    string name = "未命名";
    int id = 0;
    double scores[5] = {0, 0, 0, 0, 0};  // 五科成績，陣列也能類內初始化
    int scoreCount = 0;                    // 已輸入的成績數

    // 新增成績
    void addScore(double score) {
        if (scoreCount >= 5) {
            cout << "錯誤：最多只能輸入 5 科成績" << endl;
            return;
        }
        if (score < 0 || score > 100) {
            cout << "錯誤：成績必須在 0~100 之間" << endl;
            return;
        }
        scores[scoreCount] = score;
        scoreCount++;
    }

    // 計算平均
    double average() {
        if (scoreCount == 0) return 0.0;
        double sum = 0;
        for (int i = 0; i < scoreCount; i++) {
            sum += scores[i];
        }
        return sum / scoreCount;
    }

    // 找最高分
    double highest() {
        if (scoreCount == 0) return 0.0;
        double max = scores[0];
        for (int i = 1; i < scoreCount; i++) {
            if (scores[i] > max) {
                max = scores[i];
            }
        }
        return max;
    }

    // 找最低分
    double lowest() {
        if (scoreCount == 0) return 0.0;
        double min = scores[0];
        for (int i = 1; i < scoreCount; i++) {
            if (scores[i] < min) {
                min = scores[i];
            }
        }
        return min;
    }

    // 顯示完整資訊
    void showReport() {
        cout << "================================" << endl;
        cout << "學生: " << name << " (ID: " << id << ")" << endl;
        cout << "成績: ";
        for (int i = 0; i < scoreCount; i++) {
            cout << scores[i];
            if (i < scoreCount - 1) cout << ", ";
        }
        cout << endl;
        cout << "平均: " << average() << endl;
        cout << "最高: " << highest() << endl;
        cout << "最低: " << lowest() << endl;

        // 判定等級
        double avg = average();
        cout << "等級: ";
        if (avg >= 90) cout << "A (優秀)";
        else if (avg >= 80) cout << "B (良好)";
        else if (avg >= 70) cout << "C (中等)";
        else if (avg >= 60) cout << "D (及格)";
        else cout << "F (不及格)";
        cout << endl;
        cout << "================================" << endl;
    }
};

int main() {
    Student s1;
    s1.name = "陳信安";
    s1.id = 2001;
    s1.addScore(92);
    s1.addScore(88);
    s1.addScore(95);
    s1.addScore(78);
    s1.addScore(85);

    s1.showReport();

    cout << endl;

    Student s2;
    s2.name = "小明";
    s2.id = 2002;
    s2.addScore(65);
    s2.addScore(72);
    s2.addScore(58);

    s2.showReport();

    cout << endl;

    // 測試邊界情況
    Student s3;  // 完全使用預設值
    cout << "未設定的學生:" << endl;
    s3.showReport();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 9 課：成員變數（Member Variables）8.cpp" -o member8
//   類內初始值（含陣列的 double scores[5] = {...}）為 C++11 起的功能。

// 註 1:本檔前 584 行是 /* */ 註解區塊（本課完整講義），
//      其中的 class 與 int main() 都【不會被編譯】。

// 註 2:本檔輸出是【完全確定的】—— 沒有輸入、亂數、執行緒或位址。

// 註 3:第一位學生 5 科（92,88,95,78,85）平均 87.6 = 438/5，
//      第二位 3 科（65,72,58）平均 65 = 195/3，皆為精確的算術結果。
//      注意第二位只輸入 3 科，average() 用 scoreCount（3）而非 5 當除數 ——
//      若誤用 5 會得到 39，這正是 scores 與 scoreCount 必須同步維護的理由。

// 註 4:最後「未設定的學生」那一段展示了類內初始值的價值：
//      即使完全沒有設定過，物件仍處於明確且可預測的狀態
//      （未命名 / ID 0 / 平均 0），而不是同課 5、6 號檔那種未定義行為。
//      但請注意「已初始化」不等於「有意義」—— 這是個空白的學生記錄。

// 註 5:本機實測 sizeof(Student) = 88（實作定義）：
//      string 32 + int 4 +填充4 + double[5] 40 + int 4 +尾端填充4。
//      這個大小與實際輸入幾科【無關】—— 固定陣列的空間是預先配置好的。

// === 預期輸出 ===
// ================================
// 學生: 陳信安 (ID: 2001)
// 成績: 92, 88, 95, 78, 85
// 平均: 87.6
// 最高: 95
// 最低: 78
// 等級: B (良好)
// ================================
//
// ================================
// 學生: 小明 (ID: 2002)
// 成績: 65, 72, 58
// 平均: 65
// 最高: 72
// 最低: 58
// 等級: D (及格)
// ================================
//
// 未設定的學生:
// ================================
// 學生: 未命名 (ID: 0)
// 成績:
// 平均: 0
// 最高: 0
// 最低: 0
// 等級: F (不及格)
// ================================
