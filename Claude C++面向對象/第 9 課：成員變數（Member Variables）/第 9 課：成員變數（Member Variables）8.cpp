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
