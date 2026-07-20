// =============================================================================
//  第 10 課：成員函數 6  —  隱含的 this 指標：成員函數怎麼知道「是誰」
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  類內寫 name 等價於 this->name；
//           d1.bark() 概念上等價於 Dog::bark(&d1)。
//   標準：  C++98 起即有；C++11 起 this 在 lambda 捕獲清單中可寫 [this]/[*this]。
//   標頭檔：<iostream>、<string>
//   this 的型別：非 const 成員函式中是 Dog*；const 成員函式中是 const Dog*。
//
// 【詳細解釋 Explanation】
//
// 【1. 成員函數其實是「多吃一個參數的普通函數」】
//   所有 Dog 物件**共用同一份 bark() 機器碼**（放在 text 段），
//   那它怎麼區分該印 "旺財" 還是 "小黑"？答案是編譯器偷偷加了一個參數：
//       void bark()               →  概念上變成  void Dog_bark(Dog* this)
//       d1.bark()                 →  概念上變成  Dog_bark(&d1)
//   函式內每個未限定的成員名稱，都被補成 this->名稱。
//   所以 name 讀的是「呼叫時傳進來的那個物件」的 name。
//   ★ 這也解釋了為什麼成員函數不佔物件空間 —— 程式碼只有一份，
//     差別只在每次呼叫傳入不同的 this。
//
// 【2. 什麼時候一定要顯式寫 this->】
//   多數時候可省略，但有四個情境必須寫：
//     (a) 參數名遮蔽成員名：void setName(string name) { this->name = name; }
//     (b) 要回傳自身：return *this;（鏈式調用的基礎，見本課第 2 個範例）
//     (c) 要把自己交給別人：registry.add(this);
//     (d) class template 的衍生類別中存取依賴基底的成員 —— two-phase lookup
//         不會搜尋 dependent base，必須寫 this->f() 或 Base<T>::f()。
//
// 【3. this 是指標，而且不能被修改】
//   this 的型別是 Dog* const（指標本身是 const），
//   所以不能寫 this = &other。C++ 沒有讓你「換一個自己」的機制。
//   另外 this **永遠不會是 nullptr**（在定義良好的程式中）——
//   透過空指標呼叫成員函式是 UB，即使函式體沒碰任何成員也一樣。
//
// 【概念補充 Concept Deep Dive】
//   this 的傳遞方式是 ABI 的一部分。在 x86-64 System V ABI（Linux）上，
//   this 作為隱含的第一個參數放進 rdi 暫存器，其餘參數依序往後推 ——
//   這正是「成員函數就是多一個參數的普通函數」在機器層級的實證。
//   （MSVC 的 __thiscall 則放 ecx，細節依平台而異，屬實作定義。）
//
//   static 成員函式**沒有** this：它不綁定任何物件，
//   所以不能存取非 static 成員，也不能寫 this->。
//   這是「這個函式需不需要某個具體物件」的分界線。
//
// 【注意事項 Pay Attention】
//   1. this 是 Dog* const，不可重新賦值。
//   2. const 成員函式中 this 是 const Dog*，因此不能修改成員。
//   3. 不要回傳 this 指向的區域內容的指標／引用給比物件更長命的持有者，
//      物件銷毀後那些指標就懸空了。
//   4. lambda 捕獲 [this] 只抓指標，**不延長物件生命週期**；
//      若 lambda 比物件活得久就是懸空存取（非同步回呼的經典 bug）。
//      C++17 起可用 [*this] 複製一份物件進去避開。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】this 指標
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 所有物件共用同一份成員函數程式碼，那它怎麼知道該操作哪一個物件？
//     答：靠編譯器隱含加上的 this 參數。d1.bark() 實際上是把 &d1 當
//         第一個引數傳進去，函式內未限定的 name 都被補成 this->name。
//         成員函數本質上就是「多吃一個 this 參數的普通函數」。
//     追問：那 static 成員函式呢？
//         → 沒有 this，不綁定物件，因此無法存取非 static 成員。
//
// 🔥 Q2. 哪些情況必須顯式寫 this->？
//     答：(1) 參數名遮蔽成員名時；(2) return *this 做鏈式調用；
//         (3) 要把自身位址交給別的物件；
//         (4) class template 的衍生類別存取依賴基底類別的成員
//             （two-phase lookup 不搜尋 dependent base）。
//     追問：第 4 點不寫會怎樣？
//         → 編譯期就報「找不到該名稱」，而且錯誤訊息通常很難懂，
//           因為它發生在 template 的第一階段名稱查找。
//
// ⚠️ 陷阱. 非同步回呼裡寫 [this]{ use(name); }，物件先被銷毀了會怎樣？
//     答：這是 use-after-free，屬於未定義行為 —— 可能讀到舊資料、
//         可能崩潰、也可能「看起來正常」，沒有任何保證。
//         [this] 只複製了一個裸指標，完全不影響物件生命週期。
//     為什麼會錯：多數人把捕獲想成「lambda 會幫我保住這個物件」，
//         把它類比成 shared_ptr 的所有權語意。
//         正解是用 [*this]（C++17，複製物件）或捕獲 shared_from_this()。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Dog {
public:
    string name;

    void bark() {
        // 你寫的：
        cout << name << " 汪汪！" << endl;

        // 編譯器實際看到的（等價的寫法）：
        // cout << this->name << " 汪汪！" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】感測器自我註冊到監控中心（this 的三種必要用法一次到齊）
//   情境：每個感測器物件在建構時要把自己登記進中央的 Registry，
//         之後 Registry 就能統一輪詢所有感測器。這是 plugin／observer
//         架構最常見的起手式。
//   為什麼用到本主題：
//     (a) setLabel(string label) 的參數名遮蔽成員名 → 必須 this->label
//     (b) 建構時把自己交出去          → registry.add(this)
//     (c) 設定函式回傳 *this          → 支援鏈式設定
//   ★ 生命週期警告：Registry 存的是裸指標，並**不擁有**這些物件。
//     若感測器先被銷毀而 Registry 還在輪詢，就是懸空指標存取（UB）。
//     真實系統應改用 weak_ptr 或在解構時反註冊；這裡刻意保持簡單，
//     並在 main 中確保 Registry 的生命週期短於感測器。
// -----------------------------------------------------------------------------
class Sensor;   // 前向宣告

class Registry {
    vector<Sensor*> m_sensors;   // 只觀察、不擁有
public:
    void add(Sensor* s) { m_sensors.push_back(s); }
    void pollAll() const;        // 定義在 Sensor 完整定義之後
    size_t count() const { return m_sensors.size(); }
};

class Sensor {
    string label;
    double value = 0.0;
public:
    Sensor(Registry& reg, const string& initialLabel) : label(initialLabel) {
        reg.add(this);           // (b) 把自己的位址交給別人
    }

    // (a) 參數名與成員名同名 → 一定要用 this-> 區分
    Sensor& setLabel(string label) {
        this->label = label;     // 左邊是成員、右邊是參數
        return *this;            // (c) 回傳自身引用 → 可鏈式
    }

    Sensor& setValue(double value) {
        this->value = value;
        return *this;
    }

    void report() const {
        cout << "  [" << label << "] = " << value << endl;
    }
};

void Registry::pollAll() const {
    cout << "  共 " << m_sensors.size() << " 個感測器：" << endl;
    for (const Sensor* s : m_sensors) s->report();
}

int main() {
    cout << "=== 基本：同一份 bark() 程式碼，靠 this 分辨物件 ===" << endl;
    Dog d1;
    d1.name = "旺財";

    Dog d2;
    d2.name = "小黑";

    d1.bark();   // 編譯器轉換為：Dog::bark(&d1)  → this = &d1, 所以在 bark 函數內部，this->name 就是 d1.name
    d2.bark();   // 編譯器轉換為：Dog::bark(&d2)  → this = &d2. 所以在 bark 函數內部，this->name 就是 d2.name

    cout << "\n=== 日常實務：感測器自我註冊 ===" << endl;
    {
        Registry reg;                      // reg 先建立
        Sensor cpu(reg, "cpu_temp");       // 建構時自動註冊
        Sensor gpu(reg, "gpu_temp");
        Sensor nvme(reg, "nvme_temp");

        // 鏈式設定：每個 setter 都 return *this
        cpu.setValue(62.5);
        gpu.setLabel("gpu0_temp").setValue(71.0);   // 同時改名與設值
        nvme.setValue(44.0);

        reg.pollAll();
        // 此處三個 Sensor 都還活著，所以 Registry 裡的指標全部有效。
        // 區塊結束時銷毀順序與宣告順序相反：nvme → gpu → cpu → reg，
        // 也就是 Sensor 先走、reg 最後走。reg 解構時雖然還留著已失效的指標，
        // 但不再解參考它們，所以沒有問題 —— 真實系統仍應改用
        // weak_ptr 或在 ~Sensor 反註冊，不要依賴這種宣告順序上的巧合。
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：成員函數（Member Functions）6.cpp" -o member6

// === 預期輸出 ===
// === 基本：同一份 bark() 程式碼，靠 this 分辨物件 ===
// 旺財 汪汪！
// 小黑 汪汪！
//
// === 日常實務：感測器自我註冊 ===
//   共 3 個感測器：
//   [cpu_temp] = 62.5
//   [gpu0_temp] = 71
//   [nvme_temp] = 44
