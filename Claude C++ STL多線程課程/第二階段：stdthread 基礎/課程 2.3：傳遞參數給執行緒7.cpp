/*
# 第二階段：std::thread 基礎

## 課程 2.3：傳遞參數給執行緒

---

### 引言

當執行緒函式需要參數時，我們可以在建構 `std::thread` 時一併傳入。但這裡有一些重要的細節需要注意。

---

### 一、基本參數傳遞

參數直接跟在函式後面：

```cpp
#include <iostream>
#include <thread>

void printSum(int a, int b) {
    std::cout << a << " + " << b << " = " << (a + b) << std::endl;
}

int main() {
    std::thread t(printSum, 3, 5);
    t.join();
    return 0;
}
```

輸出：
```
3 + 5 = 8
```

---

### 二、預設行為：參數被複製

`std::thread` **總是複製**傳入的參數，即使函式期望引用：

```cpp
#include <iostream>
#include <thread>

void modify(int& x) {  // 期望引用
    x = 100;
}

int main() {
    int value = 1;
    
    std::thread t(modify, value);  // value 被複製！
    t.join();
    
    std::cout << "value = " << value << std::endl;  // 仍然是 1
    return 0;
}
```

輸出：
```
value = 1
```

`value` 沒有被修改，因為執行緒收到的是副本。

---

### 三、使用 std::ref 傳遞引用

要真正傳遞引用，必須使用 `std::ref`：

```cpp
#include <iostream>
#include <thread>

void modify(int& x) {
    x = 100;
}

int main() {
    int value = 1;
    
    std::thread t(modify, std::ref(value));  // 傳遞引用
    t.join();
    
    std::cout << "value = " << value << std::endl;  // 現在是 100
    return 0;
}
```

輸出：
```
value = 100
```

---

### 四、std::ref vs 直接傳遞

```
┌────────────────────────────────────────────────────┐
│              參數傳遞方式比較                       │
├────────────────────────────────────────────────────┤
│                                                    │
│  std::thread t(func, arg);                         │
│  → arg 被複製，執行緒操作副本                       │
│                                                    │
│  std::thread t(func, std::ref(arg));               │
│  → 傳遞引用，執行緒操作原始變數                     │
│                                                    │
│  std::thread t(func, std::cref(arg));              │
│  → 傳遞 const 引用，執行緒只能讀取                  │
│                                                    │
└────────────────────────────────────────────────────┘
```

---

### 五、傳遞指標

指標本身被複製，但指向同一位址：

```cpp
#include <iostream>
#include <thread>

void modifyPtr(int* p) {
    *p = 200;
}

int main() {
    int value = 1;
    
    std::thread t(modifyPtr, &value);
    t.join();
    
    std::cout << "value = " << value << std::endl;
    return 0;
}
```

輸出：
```
value = 200
```

---

### 六、傳遞字串的陷阱

傳遞 `const char*` 給期望 `std::string` 的函式時要小心：

```cpp
#include <iostream>
#include <thread>
#include <string>

void printStr(const std::string& s) {
    std::cout << s << std::endl;
}

void danger() {
    char buffer[] = "Hello";
    
    // 危險！buffer 可能在轉換前就被銷毀
    std::thread t(printStr, buffer);
    t.detach();
    
}  // buffer 在這裡被銷毀

void safe() {
    char buffer[] = "Hello";
    
    // 安全：明確轉換為 std::string
    std::thread t(printStr, std::string(buffer));
    t.detach();
}
```

**建議**：傳遞字串時，明確轉換為 `std::string`。

---

### 七、使用 std::move 傳遞

對於只能移動的物件（如 `std::unique_ptr`）：

```cpp
#include <iostream>
#include <thread>
#include <memory>

void process(std::unique_ptr<int> ptr) {
    std::cout << "Value: " << *ptr << std::endl;
}

int main() {
    auto ptr = std::make_unique<int>(42);
    
    std::thread t(process, std::move(ptr));  // 必須 move
    t.join();
    
    // ptr 現在是空的
    return 0;
}
```

---

### 八、參數傳遞總結表

| 情況 | 寫法 | 說明 |
|------|------|------|
| 傳值 | `thread(f, arg)` | arg 被複製 |
| 傳引用 | `thread(f, std::ref(arg))` | 傳遞原始變數的引用 |
| 傳 const 引用 | `thread(f, std::cref(arg))` | 只讀引用 |
| 傳指標 | `thread(f, &arg)` | 傳遞位址 |
| 移動語意 | `thread(f, std::move(arg))` | 轉移所有權 |

---

### 九、完整範例

```cpp
// 檔案：lesson_2_3_parameters.cpp

#include <iostream>
#include <thread>
#include <string>

void byValue(int x) {
    x = 999;  // 不影響原值
}

void byRef(int& x) {
    x = 999;  // 修改原值
}

void byString(const std::string& s) {
    std::cout << "String: " << s << std::endl;
}

int main() {
    int a = 1, b = 1;
    
    std::thread t1(byValue, a);
    std::thread t2(byRef, std::ref(b));
    std::thread t3(byString, std::string("Hello"));
    
    t1.join();
    t2.join();
    t3.join();
    
    std::cout << "a = " << a << " (unchanged)" << std::endl;
    std::cout << "b = " << b << " (modified)" << std::endl;
    
    return 0;
}
```

輸出：
```
String: Hello
a = 1 (unchanged)
b = 999 (modified)
```

---

### 十、本課重點回顧

1. `std::thread` 預設**複製**所有參數
2. 要傳遞引用，必須使用 `std::ref()` 或 `std::cref()`
3. 傳遞 C 字串時，建議先轉換為 `std::string`
4. 只能移動的物件需使用 `std::move()`
5. 使用引用時要確保變數的生命週期夠長

---

### 下一課預告

在 **課程 2.4：join() 與 detach()** 中，我們將深入探討：
- join() 的阻塞行為
- detach() 的風險與適用場景
- joinable() 狀態檢查

---

準備好繼續嗎？
*/

// =============================================================================
//  課程 2.3：傳遞參數給執行緒7.cpp  —  參數傳遞總整理:複製之後還剩什麼
// =============================================================================
//
// 【主題資訊 Information】
//   建構      : template<class F, class... Args> explicit thread(F&& f, Args&&... args);
//   標準版本  : C++11
//   標頭檔    : <thread>;std::ref/std::cref 在 <functional>;std::move 在 <utility>
//   核心規則  : 所有參數一律 decay-copy(先 std::decay 再複製/移動),
//               再以「右值」傳給目標函式
//   共享方式  : std::ref / std::cref / 傳指標 / 傳 shared_ptr
//
// 【詳細解釋 Explanation】
//
// 【1. 一條規則貫穿本課全部五個範例】
// std::thread 對參數只做一件事:decay-copy。理解這五個字,本課就通了。
// 剩下唯一要問的問題是:**「複製之後,得到的東西還依賴原本那個物件嗎?」**
//
//   傳入的東西              複製之後                    安全嗎
//   ─────────────────────  ─────────────────────────  ──────────────────
//   int / std::string      完全獨立的副本               ✅ 一定安全
//   int* / char*           另一個指標,仍指向原物件      ⚠️ 看原物件活多久
//   std::ref(x)            reference_wrapper(內含指標) ⚠️ 看原物件活多久
//   std::move(unique_ptr)  所有權轉移,原本的變 nullptr ✅ 執行緒完全擁有
//   shared_ptr             參考計數 +1                 ✅ 物件保證不死
//
// 本課的第 3 個範例(std::ref)、第 4 個(指標)、第 5 個(懸空 char*)、
// 第 6 個(move),全都是這張表的其中一列。
//
// 【2. 為什麼「值傳遞」改不到原變數】
//     void byValue(int x) { x = 999; }
//     int a = 1;
//     std::thread t(byValue, a);
//     t.join();
//     // a 仍然是 1
// 這裡發生了兩次複製:std::thread 先把 a 複製進執行緒的儲存空間,
// 呼叫時再把那份複製給參數 x。執行緒改的是 x,和 a 隔了兩層。
// 這個行為和一般函式的值傳遞一致,不是 thread 特有的怪癖 ——
// 只是很多人以為「開了執行緒就會共享」,那是錯的直覺。
//
// 【3. 為什麼參數是以「右值」傳給函式的】
// 標準規定 thread 在呼叫時,把內部儲存的參數副本以右值形式傳出去。
// 這帶來一個重要後果:
//     void f(int&  x);   // ✗ 不能直接用,右值無法綁定非 const 左值參考
//     void f(const int& x);   // ✅ 可以,const 參考能綁右值
//     void f(int x);          // ✅ 可以
// 這就是為什麼吃 int& 的函式一定要配 std::ref,
// 而吃 const std::string& 的函式卻可以直接傳 —— 後者能綁定右值。
// ⚠️ 但「能編譯」不等於「安全」:本課第 5 個範例的懸空 char* 正是
//    const 參考能綁右值,結果編譯順利通過,問題留到執行期才爆。
//
// 【4. 實務決策流程】
//   問題一:執行緒需要「讀」還是「改」呼叫端的資料?
//     只讀且資料小 → 直接傳值(最安全,不必想生命週期)
//     只讀但資料大 → std::cref(避免大物件複製,但要顧生命週期)
//     要改         → std::ref 或傳指標
//   問題二:會 detach 嗎?
//     會 → 絕對不要用 ref/指標指向區域變數;改成傳值或 shared_ptr
//     不會(會 join)→ ref/指標安全,因為 join 保證執行緒先結束
//   問題三:這份資源交出去之後呼叫端還要用嗎?
//     不要了 → std::move(unique_ptr),用型別杜絕誤用
//     還要用 → shared_ptr
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 join() 讓結果「看得見」
//   t.join() 不只是等待,它還建立同步關係:執行緒中的所有操作
//   happens-before join() 的返回。所以 join 之後讀 b 一定是 999,
//   這是記憶體模型的保證,不是「反正跑完了應該寫進去了」的直覺。
//   少了這個保證,一條執行緒寫的值可能還在該核心的 store buffer 裡,
//   別的核心讀不到。
//
// (B) decay 到底做了什麼
//   std::decay 會:陣列 → 指標、函式 → 函式指標、去掉 const/volatile 與參考。
//   關鍵是「decay 發生在複製之前」,所以 char[6] 在被複製的當下
//   已經只是個 char* 了 —— 這就是第 5 個範例那個 bug 的根源。
//
// (C) 為什麼不是所有東西都用 shared_ptr 就好
//   shared_ptr 確實最安全,但它有成本:控制區塊的配置、
//   每次複製與銷毀都要做原子的參考計數增減(在多核心下是真實開銷)。
//   準則仍是「能用值就用值,不能才用智慧指標」;
//   shared_ptr 是用來表達「真的有多個擁有者」,不是用來當萬用保險。
//
// 【注意事項 Pay Attention】
// 1. 一切都是 decay-copy;想共享必須明確寫 std::ref/cref 或傳指標。
// 2. 用了 ref/指標,生命週期就是你的責任,detach 時尤其危險。
// 3. 陣列會 decay 成指標,「複製的是指標不是內容」——見第 5 個範例。
// 4. move-only 型別必須 std::move;移動後 unique_ptr 保證為 nullptr。
// 5. std::ref 不提供任何同步;多執行緒同時讀寫仍需 mutex 或 atomic。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::thread 參數傳遞總整理
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 用一句話說明 std::thread 是怎麼處理參數的,以及由此衍生的所有規則。
//     答：它對每個參數做 decay-copy —— 先 std::decay(陣列變指標、
//         去掉參考與 cv),再複製或移動一份存進執行緒,呼叫時以右值傳出。
//         由此可推出全部規則:值傳遞改不到原變數、吃 int& 要配 std::ref、
//         char[] 只複製指標所以會懸空、move-only 型別必須 std::move。
//     追問：那怎麼判斷一種寫法安不安全?
//         → 問一個問題:「複製之後得到的東西,還依賴原本那個物件嗎?」
//           不依賴(值、move 進來的、shared_ptr)就安全;
//           依賴(指標、reference_wrapper)就要確保原物件活得比執行緒久。
//
// 🔥 Q2. 為什麼吃 int& 的函式需要 std::ref,吃 const std::string& 的卻不用?
//     答：因為 thread 是以「右值」把參數副本傳給函式的。
//         非 const 的左值參考不能綁定右值,所以編譯失敗,必須用
//         std::ref 包成 reference_wrapper;而 const 參考可以綁定右值,
//         所以直接傳就能編譯。
//     追問：那 const 參考的版本就一定安全嗎?
//         → 不一定。本課第 5 個範例正是反例:傳 char[] 給
//           const std::string& 可以編譯,但 std::string 是在新執行緒裡
//           才用那個 char* 建構的,原陣列可能已經消失 —— 執行期才爆的 UB。
//           「能編譯」和「安全」是兩件事。
//
// ⚠️ 陷阱. 「我用 std::ref 把變數傳給三條執行緒讓它們各自累加,
//         join 之後總和就會是正確的。」哪裡錯了?
//     答：std::ref 只解決了「能不能改到原變數」,完全沒有解決
//         「多條執行緒同時改同一個變數」。三條執行緒同時對同一個 int
//         做 ++,那是資料競爭 —— 未定義行為,實務上會漏掉更新,
//         總和小於預期。要正確就得用 std::atomic 或 mutex。
//     為什麼會錯：把 std::ref 當成某種「執行緒安全的共享機制」。
//         它其實只是「一個可以被複製的參考」,語意上等同傳指標,
//         不含任何同步。共享與同步是兩個獨立的問題:
//         std::ref 解決前者,mutex/atomic 解決後者,兩者都要做對。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// -----------------------------------------------------------------------------
// 原始示範:三種傳遞方式的對照
// -----------------------------------------------------------------------------
void byValue(int x) {
    x = 999;   // 只改到副本;印出來證明「執行緒內確實改了,但改的不是原變數」
    say("  [byValue] 執行緒內的副本已改成 " + std::to_string(x));
}

void byRef(int& x) {
    x = 999;   // 修改原值
}

void byString(const std::string& s) {
    say("  [byString] String: " + s);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1116. Print Zero Even Odd
//   題目：ZeroEvenOdd 物件有 zero()、even()、odd() 三個方法,由三條執行緒
//         分別呼叫。zero() 印 n 個 0,odd() 印奇數,even() 印偶數,
//         要求輸出必須是 "0102030405..." 這樣的交錯序列。
//   為什麼用到本主題：這題把本課兩個重點同時用上了 ——
//         (1) 每個方法都接收一個 std::function<void(int)> 參數,
//             也就是「把可呼叫物件當參數傳進執行緒」;
//         (2) 三條執行緒必須操作「同一個」ZeroEvenOdd 物件,
//             所以建立執行緒時要傳 &obj 而不是 obj。若不小心傳成 obj,
//             三條執行緒各拿到一份副本、各自的 turn_ 互不相通,直接死結 ——
//             這正是本課反覆強調的「複製 vs 共享」。
//   作法：用 turn_ 表示現在輪到誰(0=zero,1=odd,2=even),
//         每個方法在條件變數上等自己的號碼,印完再把 turn_ 交棒出去。
//   複雜度：時間 O(n),空間 O(1)。
// -----------------------------------------------------------------------------
class ZeroEvenOdd {
    int                     n_;
    std::mutex              m_;
    std::condition_variable cv_;
    int                     turn_ = 0;    // 0=該印 0,1=該印奇數,2=該印偶數

public:
    explicit ZeroEvenOdd(int n) : n_(n) {}

    void zero(std::function<void(int)> printNumber) {
        for (int i = 1; i <= n_; ++i) {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this] { return turn_ == 0; });
            printNumber(0);
            turn_ = (i % 2 == 1) ? 1 : 2;   // 下一個該印的是奇數還偶數
            cv_.notify_all();
        }
    }

    void odd(std::function<void(int)> printNumber) {
        for (int i = 1; i <= n_; i += 2) {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this] { return turn_ == 1; });
            printNumber(i);
            turn_ = 0;
            cv_.notify_all();
        }
    }

    void even(std::function<void(int)> printNumber) {
        for (int i = 2; i <= n_; i += 2) {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [this] { return turn_ == 2; });
            printNumber(i);
            turn_ = 0;
            cv_.notify_all();
        }
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】示範「共享 ≠ 同步」:std::ref 解決不了資料競爭
//   情境: 這是【陷阱】那一題的可執行版本。多條執行緒透過 std::ref
//         共享同一個計數器並各自累加 —— 共享確實成功了,
//         但因為沒有同步,結果會少算。旁邊放一個 atomic 版本對照。
//   為什麼用本主題: 讓「std::ref 只負責共享、不負責同步」這件事
//                   從抽象的警告變成看得見的數字。
// -----------------------------------------------------------------------------
void unsafeIncrement(int& counter, int times) {
    for (int i = 0; i < times; ++i) {
        ++counter;              // ⚠️ 資料競爭:多執行緒同時讀寫同一個 int
    }
}

void safeIncrement(std::atomic<int>& counter, int times) {
    for (int i = 0; i < times; ++i) {
        ++counter;              // ✅ 原子操作,不可分割
    }
}

int main() {
    std::cout << "=== 原始示範:值 / 引用 / 字串 三種傳遞 ===" << std::endl;
    {
        int a = 1, b = 1;

        std::thread t1(byValue, a);                      // 複製,改不到 a
        std::thread t2(byRef, std::ref(b));              // 共享,改得到 b
        std::thread t3(byString, std::string("Hello"));  // 複製完整字串

        t1.join();
        t2.join();
        t3.join();

        std::cout << "  a = " << a << " (unchanged)" << std::endl;
        std::cout << "  b = " << b << " (modified)" << std::endl;
    }

    std::cout << "\n=== LeetCode 1116. Print Zero Even Odd (n=5) ===" << std::endl;
    {
        ZeroEvenOdd zeo(5);
        auto print = [](int x) { std::cout << x; };

        // 注意傳的是 &zeo —— 三條執行緒必須共享同一個物件
        std::thread tz(&ZeroEvenOdd::zero, &zeo, print);
        std::thread to(&ZeroEvenOdd::odd,  &zeo, print);
        std::thread te(&ZeroEvenOdd::even, &zeo, print);
        tz.join();
        to.join();
        te.join();

        std::cout << std::endl;
        std::cout << "  ↑ 每次執行都固定是 0102030405" << std::endl;
    }

    std::cout << "\n=== 共享 ≠ 同步:std::ref 擋不住資料競爭 ===" << std::endl;
    {
        const int kThreads = 8, kTimes = 50000;

        int plain = 0;
        std::vector<std::thread> pool;
        for (int i = 0; i < kThreads; ++i)
            pool.emplace_back(unsafeIncrement, std::ref(plain), kTimes);
        for (std::thread& t : pool) t.join();

        std::atomic<int> atomicCounter{0};
        std::vector<std::thread> pool2;
        for (int i = 0; i < kThreads; ++i)
            pool2.emplace_back(safeIncrement, std::ref(atomicCounter), kTimes);
        for (std::thread& t : pool2) t.join();

        std::cout << "  期望值               = " << kThreads * kTimes << std::endl;
        std::cout << "  std::ref + 普通 int  = " << plain
                  << "  ← 這個數字每次執行都不同,而且幾乎都小於期望值" << std::endl;
        std::cout << "  std::ref + atomic    = " << atomicCounter.load()
                  << "  ← 永遠正確" << std::endl;
        std::cout << "  ↑ 兩者都用了 std::ref,差別只在有沒有同步" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.3：傳遞參數給執行緒7.cpp" -o pass_params

// 注意:以下為某一次實際執行的結果。
//   * 「std::ref + 普通 int」那一行的數字**每次執行都不同**
//     (那正是資料競爭的證據;理論上它甚至可能剛好等於期望值,
//      但那不代表程式是對的)。該行為屬於未定義行為,
//      這裡只是呈現「它會出錯」,不是保證會出現某個特定數字。
//   * 第一段三條執行緒的行序每次執行都可能不同。
//   * LeetCode 1116 那段每次都固定是 0102030405,
//     因為那是條件變數保證出來的。
//   * a=1 / b=999 / atomic=400000 也都是語意保證,每次相同。

// === 預期輸出 ===
// === 原始示範:值 / 引用 / 字串 三種傳遞 ===
//   [byValue] 執行緒內的副本已改成 999
//   [byString] String: Hello
//   a = 1 (unchanged)
//   b = 999 (modified)
//
// === LeetCode 1116. Print Zero Even Odd (n=5) ===
// 0102030405
//   ↑ 每次執行都固定是 0102030405
//
// === 共享 ≠ 同步:std::ref 擋不住資料競爭 ===
//   期望值               = 400000
//   std::ref + 普通 int  = 72536  ← 這個數字每次執行都不同,而且幾乎都小於期望值
//   std::ref + atomic    = 400000  ← 永遠正確
//   ↑ 兩者都用了 std::ref,差別只在有沒有同步
