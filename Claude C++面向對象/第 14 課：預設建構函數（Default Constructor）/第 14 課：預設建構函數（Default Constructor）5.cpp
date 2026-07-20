// =============================================================================
//  第 14 課：預設建構函數 5  —  A() = default 與 B() { } 的關鍵差異
// =============================================================================
//
// 【主題資訊 Information】
//   主題      : default-initialization vs value-initialization；user-provided 的份量
//   標準版本  : = default 是 C++11；value-initialization 的規則在 C++11／C++14 有調整
//   標頭檔    : <iostream>、<type_traits>
//   一句話    : 差別不在 constructor 做了什麼，而在**呼叫端寫不寫那兩個大括號時，
//               語言願不願意先幫你把記憶體清成 0**
//
// 【詳細解釋 Explanation】
//
// 【1. 先把兩個名詞分清楚】
//       T obj;      → default-initialization（預設初始化）
//       T obj{};    → value-initialization（值初始化）
//   兩者是**不同的初始化形式**，不是「同一件事的兩種寫法」。
//
// 【2. value-initialization 的實際規則（本檔的核心）】
//   對一個 class type T，T obj{} 的行為取決於 T 的 default constructor：
//     (a) 若 T 有 **user-provided**（你寫了 { } 本體）的 default constructor
//         → **只呼叫那個 constructor**，不做任何 zero-initialization
//     (b) 若 T 的 default constructor 不是 user-provided（= default 或隱式生成）
//         → **先把整個物件 zero-initialize**，然後若該 constructor 非 trivial 才呼叫它
//   於是就出現了本檔要示範的現象：
//       A a2{};   // A() = default  → 先歸零 → a2.value == 0（標準保證）
//       B b2{};   // B() { }        → 只呼叫空的 constructor → b2.value 不確定
//   **同樣是空的、同樣什麼都沒做的 constructor，結果完全相反。**
//
// 【3. 為什麼標準要這樣設計】
//   邏輯是「尊重使用者的意圖」：
//   你既然親手寫了 constructor 本體，語言就假設**你已經完全掌控了初始化**，
//   不該再自作主張替你把記憶體清零（那可能是你刻意避免的成本）。
//   反之 = default 表示「我沒有特別想法，照標準來」，
//   於是 value-initialization 就能安全地先歸零。
//
// 【4. 注意：原始教材的說法要修正】
//   常見的錯誤解釋是「因為 A() = default 生成的建構函數會對基本型別進行值初始化」。
//   這句話把因果搞反了：**歸零的動作不是那個 constructor 做的**，
//   而是 value-initialization 這個「初始化形式」在呼叫 constructor **之前**做的。
//   證據很直接：A a1;（default-init，同一個 constructor）就不會歸零。
//   同一個 constructor、不同的初始化形式 → 不同結果，可見關鍵不在 constructor。
//
// 【5. 實務結論：不要把安全性押在呼叫端身上】
//   靠 T obj{} 來保證歸零有兩個弱點：
//     * 呼叫端可能忘了寫那兩個大括號
//     * 只要有人日後把 = default 改成 { }，保證就無聲消失
//   類別作者能提供的可靠保證只有一個：**NSDMI**（int value = 0;）。
//   它不管呼叫端怎麼寫、也不管未來誰改了 constructor，值都是確定的。
//
// 【概念補充 Concept Deep Dive】
//   ▍C++11 → C++14 的規則修訂
//     value-initialization 的措辭在標準演進中被修過（CWG 1301 等）。
//     現行規則已如上述第 2 點。實務上請以「有沒有 user-provided」為判準，
//     不要用「有沒有寫 constructor」——= default 也是「寫了」，但不是 user-provided。
//
//   ▍在類別外 = default 會變成 user-provided
//     class A { public: A(); };  A::A() = default;
//     這樣寫的 A 就變成 user-provided，A a{} 不再歸零。
//     所以 = default 一定要寫在類別內的第一次宣告處。
//
//   ▍new T 與 new T() 也有同樣的差別
//     new A   → default-initialization → value 不確定
//     new A() → value-initialization   → value 為 0
//     這是同一套規則在動態配置上的體現，本檔一併實測。
//
// 【注意事項 Pay Attention】
//   1. b1.value 與 b2.value 是不確定值，讀取即 UB；本檔輸出僅為某次執行的觀察。
//   2. 「空的 constructor」和「= default」在語言規則上是不同的東西，儘管本體都空。
//   3. 依賴 T obj{} 來歸零很脆弱——類別作者請改用 NSDMI。
//   4. = default 必須寫在類別內第一次宣告，否則會退化成 user-provided。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】default-init vs value-init
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. A() = default; 和 B() { } 都是空的，為什麼 A a{}; 是 0 而 B b{}; 不是？
//     答：因為 B 的是 **user-provided**。value-initialization 的規則是：
//         若 default constructor 是 user-provided，就只呼叫它、不做 zero-init；
//         否則先把整個物件 zero-initialize 再視需要呼叫。
//         A 的 = default 不算 user-provided，所以 A a{} 先歸零 → 0。
//     追問：那 A a; 呢？→ 不確定值。同一個 constructor，
//         但 default-initialization 不會歸零——可見關鍵在初始化形式，不在 constructor。
//
// 🔥 Q2. new T 和 new T() 有差別嗎？
//     答：有。new T 是 default-initialization，內建型別成員不確定；
//         new T() 是 value-initialization，遵循上題的規則。
//         對沒有 user-provided default constructor 的型別，new T() 會歸零。
//         這也是 new int 與 new int() 行為不同的原因。
//     追問：new T{} 呢？→ 與 new T() 同為 value-initialization（若無
//         initializer_list constructor 介入），行為一致。
//
// ⚠️ 陷阱. 「我把 = default 改成 { }，反正本體都是空的，行為應該一樣」
//     答：不一樣，而且會無聲地改壞。改完之後：
//         (1) T obj{}; 不再歸零 → 原本正確的程式碼開始讀到不確定值；
//         (2) 型別不再 trivial → 依賴 memcpy 或 trivially copyable 的地方失效；
//         (3) C++17 下還會失去 aggregate 資格。
//         這三件事都不會有編譯警告。
//     為什麼會錯：以為「函式本體一樣 ⇒ 語意一樣」。
//         在特殊成員函式上，**怎麼宣告的**比**本體寫什麼**更重要。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <type_traits>
using namespace std;

class A {
public:
    int value;
    A() = default;   // 不是 user-provided → value-initialization 會先歸零
};

class B {
public:
    int value;
    B() { }          // user-provided（本體是空的也算）→ 不會歸零
};

// 唯一可靠的作法：NSDMI，不管呼叫端怎麼寫都有確定的值
class C {
public:
    int value = 0;
    C() = default;
};

// 編譯期事實查核（本機 GCC 15.2 實測）
static_assert(std::is_trivially_default_constructible<A>::value,
              "A() = default 且無 NSDMI → trivial");
static_assert(!std::is_trivially_default_constructible<B>::value,
              "B() { } 是 user-provided → 必定不是 trivial");
static_assert(!std::is_trivially_default_constructible<C>::value,
              "C 有 NSDMI → 需要執行初始化，不是 trivial");

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：本檔區分的是 default-initialization 與 value-initialization 這兩種
//         初始化形式，以及 user-provided 的判定——純粹是 C++ 的初始化語意。
//         LeetCode 不會因為你用 {} 或不用 {} 而給出不同結果，沒有題目對應。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】環形緩衝區（RingBuffer）的索引：一個大括號的代價
//   情境：實作一個固定容量的環形緩衝區，內部有 head / tail / count 三個索引。
//         若這三個索引沒有被歸零就開始使用，push 會寫到緩衝區外、
//         count 會從一個天文數字開始遞減——而且因為是不確定值，
//         同樣的程式碼在開發機上可能一路正常，到了正式環境才崩。
//   對照：Fragile 靠呼叫端寫 {} 才安全；Robust 用 NSDMI，怎麼建構都安全。
// -----------------------------------------------------------------------------
class RingBufferFragile {
public:
    static const int CAP = 4;
    int data[CAP];
    int head;
    int count;
    RingBufferFragile() = default;   // 靠呼叫端寫 {} 才會歸零 —— 脆弱

    bool push(int v) {
        if (count >= CAP) return false;
        data[(head + count) % CAP] = v;
        ++count;
        return true;
    }
    int size() const { return count; }
};

class RingBufferRobust {
public:
    static const int CAP = 4;
    int data[CAP] = {};      // NSDMI：陣列也歸零
    int head  = 0;           // NSDMI：類別作者的保證
    int count = 0;

    bool push(int v) {
        if (count >= CAP) return false;
        data[(head + count) % CAP] = v;
        ++count;
        return true;
    }
    int size() const { return count; }
    void dump() const {
        cout << "  [";
        for (int i = 0; i < count; ++i)
            cout << data[(head + i) % CAP] << (i + 1 < count ? ", " : "");
        cout << "]  size=" << count << endl;
    }
};

int main() {
    cout << "=== 情境 1：default-initialization（不寫大括號）===" << endl;
    A a1;    // 不確定值（= default 不做初始化）
    B b1;    // 不確定值（空的 constructor 也不做初始化）
    cout << "  a1.value = " << a1.value << "  (不確定值)" << endl;
    cout << "  b1.value = " << b1.value << "  (不確定值)" << endl;
    cout << "  ↑ 這一組兩者行為相同：都沒有初始化" << endl;

    cout << "\n=== 情境 2：value-initialization（寫了大括號）===" << endl;
    A a2{};  // A() 不是 user-provided → 先 zero-initialize → 標準保證為 0
    B b2{};  // B() 是 user-provided   → 只呼叫空 constructor → 仍不確定
    cout << "  a2.value = " << a2.value << "  (標準保證是 0)" << endl;
    cout << "  b2.value = " << b2.value << "  (不確定值，可能剛好是 0)" << endl;
    cout << "  ↑ 關鍵差異就在這裡：同樣是空 constructor，結果不同" << endl;

    cout << "\n=== 為什麼？看 user-provided 這個性質 ===" << endl;
    cout << boolalpha;
    cout << "  A trivially_default_constructible = "
         << std::is_trivially_default_constructible<A>::value
         << "  → A() 不是 user-provided" << endl;
    cout << "  B trivially_default_constructible = "
         << std::is_trivially_default_constructible<B>::value
         << "  → B() 是 user-provided" << endl;
    cout << "  歸零不是 constructor 做的，是 value-initialization 在呼叫" << endl;
    cout << "  constructor「之前」做的——所以 A a1; 才不會歸零。" << endl;

    cout << "\n=== 情境 3：new T vs new T() 也是同一套規則 ===" << endl;
    A* pa1 = new A;      // default-init
    A* pa2 = new A();    // value-init → 歸零
    cout << "  new A  → value = " << pa1->value << "  (不確定值)" << endl;
    cout << "  new A() → value = " << pa2->value << "  (標準保證是 0)" << endl;
    delete pa1;
    delete pa2;

    cout << "\n=== 情境 4：NSDMI —— 唯一不依賴呼叫端的保證 ===" << endl;
    C c1;
    C c2{};
    cout << "  C c1;   value = " << c1.value << endl;
    cout << "  C c2{}; value = " << c2.value << endl;
    cout << "  ↑ 兩種寫法都是 0，呼叫端不必知道任何規則" << endl;

    cout << "\n=== 日常實務：RingBuffer 的索引 ===" << endl;
    RingBufferRobust rb;          // 沒寫 {}，但 NSDMI 保證安全
    rb.push(10); rb.push(20); rb.push(30);
    cout << "  Robust（NSDMI）: ";
    rb.dump();

    RingBufferFragile fragileOk{};   // 寫了 {} 才安全
    cout << "  Fragile 有寫 {} : size=" << fragileOk.size()
         << "（歸零成功）" << endl;
    cout << "  Fragile 若寫成 RingBufferFragile rb; （少了大括號），" << endl;
    cout << "  count 會從不確定值開始，push 可能立刻越界——" << endl;
    cout << "  故本檔不實際示範，因為那是 UB，示範它的「結果」沒有意義。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：預設建構函數（Default Constructor）5.cpp" -o def5

// === 預期輸出 ===
// ⚠️ a1.value、b1.value、b2.value 與 new A 的 value 皆為不確定值（讀取即 UB），
//    下方僅為某一次執行的觀察結果，每次執行都可能不同。
//    標記為「標準保證」的 a2{}、new A()、C 與 NSDMI 結果才是可依賴的。
// === 情境 1：default-initialization（不寫大括號）===
//   a1.value = 562115968  (不確定值)
//   b1.value = 32039  (不確定值)
//   ↑ 這一組兩者行為相同：都沒有初始化
//
// === 情境 2：value-initialization（寫了大括號）===
//   a2.value = 0  (標準保證是 0)
//   b2.value = 32767  (不確定值，可能剛好是 0)
//   ↑ 關鍵差異就在這裡：同樣是空 constructor，結果不同
//
// === 為什麼？看 user-provided 這個性質 ===
//   A trivially_default_constructible = true  → A() 不是 user-provided
//   B trivially_default_constructible = false  → B() 是 user-provided
//   歸零不是 constructor 做的，是 value-initialization 在呼叫
//   constructor「之前」做的——所以 A a1; 才不會歸零。
//
// === 情境 3：new T vs new T() 也是同一套規則 ===
//   new A  → value = 1581108088  (不確定值)
//   new A() → value = 0  (標準保證是 0)
//
// === 情境 4：NSDMI —— 唯一不依賴呼叫端的保證 ===
//   C c1;   value = 0
//   C c2{}; value = 0
//   ↑ 兩種寫法都是 0，呼叫端不必知道任何規則
//
// === 日常實務：RingBuffer 的索引 ===
//   Robust（NSDMI）:   [10, 20, 30]  size=3
//   Fragile 有寫 {} : size=0（歸零成功）
//   Fragile 若寫成 RingBufferFragile rb; （少了大括號），
//   count 會從不確定值開始，push 可能立刻越界——
//   故本檔不實際示範，因為那是 UB，示範它的「結果」沒有意義。
