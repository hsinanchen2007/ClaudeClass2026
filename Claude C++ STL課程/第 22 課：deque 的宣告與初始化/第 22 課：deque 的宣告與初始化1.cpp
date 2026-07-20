// =============================================================================
//  第 22 課：deque 的宣告與初始化 1  —  九種建構方式，以及 deque 與 vector 的分水嶺
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：#include <deque>
//   類別：  template<class T, class Allocator = std::allocator<T>> class deque;
//
//   建構子（與 vector 幾乎同一套，這是刻意的設計）：
//     deque();                                   // (1) 預設，空容器
//     explicit deque(size_type n);               // (2) n 個 value-initialized 元素
//     deque(size_type n, const T& v);            // (3) n 個 v 的複本
//     deque(InputIt first, InputIt last);        // (4) 迭代器範圍
//     deque(const deque& other);                 // (5) 複製建構
//     deque(deque&& other) noexcept;             // (6) 移動建構
//     deque(std::initializer_list<T> il);        // (7) 初始化列表
//     void assign(...);                          // (8) 建構後重新指定內容
//
//   複雜度：(1) O(1)；(2)(3)(7) O(n)；(4) O(n)；(5) O(n)；(6) O(1)
//   標準版本：(6)(7) 是 C++11 起；CTAD（deque d{1,2,3} 免寫 <int>）是 C++17 起
//             （已用 -pedantic-errors 驗證：C++14 下 CTAD 會編譯失敗）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼建構子長得跟 vector 一模一樣】
// STL 的序列容器（vector / deque / list）刻意共用同一套建構子介面，這叫
// 「concept 一致性」。好處是換容器時**宣告與初始化的程式碼幾乎不用改**：
//     vector<int> v(4, 77);
//     deque<int>  d(4, 77);   // 一模一樣的寫法
// 這讓「先用 vector 寫，效能不對再換 deque」這種重構成本極低。
// 但介面一樣**不代表底層一樣** —— 這正是本課真正要講的重點。
//
// 【2. deque 不是連續記憶體：map + chunk 的兩層結構】
// vector 是「一塊連續的緩衝區」；deque 則是**分段**的：
//
//     map（中央控制陣列，本身是連續的 T** 指標陣列）
//     ┌────┬────┬────┬────┬────┐
//     │ p0 │ p1 │ p2 │ p3 │ p4 │      ← 每格是一根指標
//     └─┬──┴─┬──┴─┬──┴─┬──┴─┬──┘
//       │    │    │    │    │
//       ▼    ▼    ▼    ▼    ▼
//     [chunk][chunk][chunk] …          ← 每個 chunk 是固定大小的連續小塊
//
// 元素存在 chunk 裡，chunk 之間**彼此不相鄰**。map 只存「指向各 chunk 的指標」。
// 這個結構直接推導出 deque 的三個特徵：
//   (a) 頭尾都能 O(1) 增長 —— 只要在 map 的兩端多掛一個 chunk 即可。
//   (b) **沒有 data()**  —— 元素不連續，根本無法回傳「一根指向全部元素的指標」。
//   (c) **沒有 reserve() / capacity()** —— 沒有「單一緩衝區容量」這個概念。
//
// 本機以 SFINAE 編譯期偵測實測（libstdc++ 15.2.0）：
//     deque<int>::data()          → 不存在
//     deque<int>::capacity()      → 不存在
//     deque<int>::reserve()       → 不存在
//     deque<int>::shrink_to_fit() → **存在**（這個 deque 有，容易記錯）
//
// 【3. 小括號 vs 大括號：C++ 最著名的初始化陷阱】
//     deque<int> d8(5, 10);   // → 5 個 10       ：呼叫 (size, value) 建構子
//     deque<int> d9{5, 10};   // → 兩個元素 5、10：呼叫 initializer_list 建構子
// 規則：**只要 initializer_list 建構子「能」匹配，大括號就一定選它**，
// 即使有「更合適」的其他建構子也一樣。這是 C++11 為了讓 {} 語意可預測而定的
// 硬規則，代價就是這個違反直覺的行為。
// 記憶法：想要「n 個 v」→ 一定用小括號；想要「就是這幾個元素」→ 用大括號。
//
// 【概念補充 Concept Deep Dive】
//
// (A) chunk 大小是實作定義的
//   標準完全沒有規定 chunk 多大。libstdc++ 的規則（本機 g++ 15.2.0 實測）是：
//       chunk 位元組數 = 512
//       每 chunk 元素數 = max(1, 512 / sizeof(T))
//   實測數據：
//       sizeof(char)=1   → 512 個 / chunk
//       sizeof(int)=4    → 128 個 / chunk
//       sizeof(double)=8 →  64 個 / chunk
//       sizeof(T)=256    →   2 個 / chunk
//       sizeof(T)=600    →   1 個 / chunk（超過 512 就退化成每 chunk 一個）
//   MSVC 的 STL 用 16 bytes，libc++ 用 4096 bytes —— **完全不同**。
//   所以任何「deque 每塊 512 bytes」的說法都必須標明是哪個實作。
//
// (B) 空的 deque 仍然會配置記憶體（vector / list 不會）
//   本機以覆寫 operator new 攔截實測：
//       空 deque<int>  建構 → 2 次配置，共 576 bytes
//       空 vector<int> 建構 → 0 次配置
//       空 list<int>   建構 → 0 次配置
//   576 = 512（先配一個 chunk）+ 64（map 陣列，初始 8 根指標 × 8 bytes）。
//   deque 的實作需要「隨時有個合法的中間位置」讓 begin()/end() 指得到，
//   所以即使空的也先開一個 chunk。這是**實作細節、非標準保證**，但它解釋了
//   為什麼「開一大堆空 deque」比「開一大堆空 vector」貴得多。
//   本機 sizeof 實測：deque<int>=80、vector<int>=24、list<int>=24 bytes。
//
// (C) 移動建構為什麼是 O(1)
//   移動只是把 map 指標、start/finish 迭代器搬過去，再把來源清空 ——
//   完全不碰任何 chunk 內的元素。所以 deque(deque&&) 是 noexcept 且 O(1)，
//   跟裡面有 3 個還是 300 萬個元素無關。
//
// 【注意事項 Pay Attention】
// 1. 被移動後的來源物件處於「有效但未指定」狀態。實務上 libstdc++ 會把它變成
//    空的（本課實測輸出 size=0），但**標準只保證「有效」**，不保證是空的。
//    唯一安全的操作是重新賦值或呼叫無前置條件的成員（如 clear()、size()）。
// 2. deque<int> d(5) 的 5 個元素是 **value-initialized**（int 會是 0），
//    不是未初始化的垃圾值。這點跟 C 陣列 int a[5]; 不同。
// 3. explicit：deque<int> d = 5; 不能編譯（(2) 是 explicit），必須寫 d(5)。
// 4. 從迭代器範圍建構時，來源可以是**任何**容器（vector、list、set、甚至
//    istream_iterator），只要 value_type 可轉換即可。
// 5. 容器的複製是「逐元素複製」。若元素是**裸指標**，複製的是指標值本身，
//    指向的資料仍共用 —— 這是淺複製陷阱，見同課 2.cpp 專門討論。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 的宣告與初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. deque 和 vector 的介面幾乎一樣，那 deque 少了哪些 vector 有的成員？為什麼？
//     答：少了 data()、capacity()、reserve()。根本原因是 deque **不保證連續記憶體**，
//         它是 map + 多個固定大小 chunk 的結構。沒有單一緩衝區，
//         就沒有「容量」可言，也無法回傳一根涵蓋全部元素的指標。
//         注意 shrink_to_fit() 是**有**的（本機 SFINAE 實測確認）。
//     追問：那要把 deque 的資料交給 C API（吃 const int*）怎麼辦？
//         → 只能先複製到 vector：vector<int> v(d.begin(), d.end())，再傳 v.data()。
//
// 🔥 Q2. deque<int> d(5, 10) 和 deque<int> d{5, 10} 差在哪？
//     答：d(5,10) 是 5 個 10（size=5）；d{5,10} 是兩個元素 5 和 10（size=2）。
//         大括號會優先匹配 initializer_list 建構子，只要它「能」匹配就一定選它，
//         即使 (size, value) 版本看起來更合適。
//     追問：那 deque<string> d{5, "x"} 呢？
//         → initializer_list<string> 匹配不了（5 不能轉 string），
//           所以會退回 (size, value) 建構子 → 5 個 "x"。
//           **同樣的括號、不同的元素型別，語意就翻轉了**，這正是它危險的地方。
//
// 🔥 Q3. deque 的移動建構為什麼是 O(1)？被移動的來源之後還能用嗎？
//     答：移動只搬 map 指標與 start/finish 迭代器，不碰任何元素，所以 O(1) 且 noexcept。
//         來源之後處於「有效但未指定（valid but unspecified）」狀態 ——
//         可以安全地重新賦值、呼叫 clear()/size()，但**不可假設它是空的**。
//     追問：本課輸出顯示被移動後 size=0，這算標準保證嗎？
//         → 不算。那是 libstdc++ 的實際行為，標準沒有規定。寫程式不可依賴它。
//
// ⚠️ 陷阱. 「deque 每個 chunk 固定 512 bytes，所以 deque<int> 每塊放 128 個」——
//          這句話錯在哪？
//     答：錯在把「libstdc++ 的實作細節」講成「C++ 的規定」。標準**完全沒有**
//         規定 chunk 大小。本機 libstdc++ 15.2.0 實測確實是 512 bytes /
//         max(1, 512/sizeof(T)) 個元素，但 MSVC 是 16 bytes、libc++ 是 4096 bytes。
//         而且 sizeof(T) > 512 時會退化成每 chunk 只放 1 個（本機 600 bytes 實測驗證）。
//     為什麼會錯：多數人只在一種編譯器上量過，就把量到的數字當成語言規範。
//         凡是「實作定義」的數值，回答時都必須附上「在哪個實作上量到的」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <vector>
#include <string>
using namespace std;

void print(const string& label, const deque<int>& dq) {
    cout << label << ": ";
    for (int val : dq) {
        cout << val << " ";
    }
    cout << "(size=" << dq.size() << ")" << endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 641. Design Circular Deque
//   題目：設計一個固定容量的雙端佇列，支援頭尾插入/刪除、取頭尾、判空判滿。
//   為什麼用到本主題：這題要求「**建構時就指定容量**」，正好逼你思考
//       deque 沒有 reserve()/capacity() 這件事 —— 容量上限得自己用一個
//       成員變數維護，不能靠容器本身。內部用 deque 是最自然的實作：
//       push_front / push_back / pop_front / pop_back 全是 O(1)。
//   複雜度：所有操作 O(1)。
// -----------------------------------------------------------------------------
class MyCircularDeque {
    deque<int> dq_;
    size_t     cap_;
public:
    // 注意：這裡只能自己記 cap_，deque 沒有 capacity() 可問
    explicit MyCircularDeque(int k) : cap_(static_cast<size_t>(k)) {}

    bool insertFront(int value) {
        if (isFull()) return false;
        dq_.push_front(value);
        return true;
    }
    bool insertLast(int value) {
        if (isFull()) return false;
        dq_.push_back(value);
        return true;
    }
    bool deleteFront() {
        if (isEmpty()) return false;
        dq_.pop_front();
        return true;
    }
    bool deleteLast() {
        if (isEmpty()) return false;
        dq_.pop_back();
        return true;
    }
    int  getFront() const { return isEmpty() ? -1 : dq_.front(); }
    int  getRear()  const { return isEmpty() ? -1 : dq_.back(); }
    bool isEmpty()  const { return dq_.empty(); }
    bool isFull()   const { return dq_.size() >= cap_; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】感測器最近 N 筆讀數的滑動視窗（環狀緩衝）
//   情境：工廠的溫度感測器每秒回報一次，監控程式只關心「最近 5 秒」的讀數，
//         用來算移動平均、判斷是否過熱。舊資料要從頭端淘汰、新資料從尾端進來。
//   為什麼用 deque：
//     - 頭端淘汰 pop_front() 是 O(1)；vector 的 erase(begin()) 是 O(n)。
//     - 容量固定且很小，不需要 reserve —— 正好避開 deque 沒有 reserve 的限制。
//   注意：這裡用「建構時指定視窗大小」，正是本課建構子知識的實際用途。
// -----------------------------------------------------------------------------
class SensorWindow {
    deque<double> readings_;
    size_t        window_;
public:
    explicit SensorWindow(size_t window) : window_(window) {}

    void push(double celsius) {
        readings_.push_back(celsius);
        if (readings_.size() > window_) {
            readings_.pop_front();   // O(1)：deque 的看家本領
        }
    }
    double average() const {
        if (readings_.empty()) return 0.0;
        double sum = 0.0;
        for (double r : readings_) sum += r;
        return sum / static_cast<double>(readings_.size());
    }
    double peak() const {
        if (readings_.empty()) return 0.0;
        double m = readings_.front();
        for (double r : readings_) if (r > m) m = r;
        return m;
    }
    size_t count() const { return readings_.size(); }
};

int main() {
    cout << "===== 九種初始化方式 =====" << endl;

    // 1. 預設建構
    deque<int> d1;
    print("d1 空deque", d1);

    // 2. 指定數量與預設值（value-initialized → int 為 0）
    deque<int> d2(5);
    print("d2 五個0 ", d2);

    // 3. 指定數量與指定值
    deque<int> d3(4, 77);
    print("d3 四個77", d3);

    // 4. 初始化列表
    deque<int> d4 = {10, 20, 30, 40, 50};
    print("d4 列表  ", d4);

    // 5. 複製建構
    deque<int> d5(d4);
    print("d5 複製d4", d5);

    // 驗證深複製（元素本身是 int，複製後互不影響）
    d5[0] = 999;
    print("修改d5後 ", d5);
    print("d4不受影響", d4);

    // 6. 移動建構（O(1)：只搬 map 指標，不碰元素）
    deque<int> d6(move(d5));
    print("d6 移動d5", d6);
    print("d5 被移走 ", d5);   // libstdc++ 實測為空；標準只保證「有效但未指定」

    // 7. 從 vector 的迭代器範圍建構（來源可以是任何容器）
    vector<int> vec = {100, 200, 300};
    deque<int> d7(vec.begin(), vec.end());
    print("d7 從vec ", d7);

    // 8. assign 重新指定
    d7.assign({7, 8, 9, 10});
    print("d7 assign", d7);

    // 9. 小括號 vs 大括號（最著名的初始化陷阱）
    deque<int> d8(5, 10);
    deque<int> d9{5, 10};
    print("d8 (5,10)", d8);
    print("d9 {5,10}", d9);

    // ── 同一組括號、換個元素型別，語意就翻轉 ──────────────────────
    cout << "\n===== 括號陷阱：換成 string 就翻轉 =====" << endl;
    deque<string> s1{5, "x"};   // initializer_list<string> 匹配不了 → 退回 (n, v)
    cout << "deque<string> s1{5,\"x\"} → size=" << s1.size()
         << "，內容:";
    for (const auto& s : s1) cout << " " << s;
    cout << "\n（同樣寫 {5, ...}，int 版得到 2 個元素，string 版得到 5 個！）" << endl;

    // ── deque 沒有 capacity/reserve/data ─────────────────────────
    cout << "\n===== deque 沒有 vector 的容量介面 =====" << endl;
    cout << "deque 有 size()        → " << d4.size() << endl;
    cout << "deque 有 max_size()    → 很大（實作定義）" << endl;
    cout << "deque 沒有 capacity() / reserve() / data()（不連續，編譯不過）" << endl;
    cout << "但 deque 有 shrink_to_fit()（容易記錯的一個）" << endl;
    // d4.data();      // ← 取消註解會編譯失敗：deque 無 data()
    // d4.reserve(10); // ← 取消註解會編譯失敗：deque 無 reserve()

    cout << "\n===== LeetCode 641. Design Circular Deque =====" << endl;
    MyCircularDeque cd(3);
    cout << "insertLast(1)  → " << boolalpha << cd.insertLast(1)  << endl;
    cout << "insertLast(2)  → " << cd.insertLast(2)  << endl;
    cout << "insertFront(3) → " << cd.insertFront(3) << endl;
    cout << "insertFront(4) → " << cd.insertFront(4) << " (已滿，失敗)" << endl;
    cout << "getRear()      → " << cd.getRear()      << endl;
    cout << "isFull()       → " << cd.isFull()       << endl;
    cout << "deleteLast()   → " << cd.deleteLast()   << endl;
    cout << "insertFront(4) → " << cd.insertFront(4) << endl;
    cout << "getFront()     → " << cd.getFront()     << endl;

    cout << "\n===== 日常實務：感測器最近 5 筆讀數 =====" << endl;
    SensorWindow win(5);
    const double samples[] = {22.5, 23.1, 24.8, 26.0, 27.3, 31.9, 33.4};
    for (double s : samples) {
        win.push(s);
        cout << "  進料 " << s << "°C → 視窗內 " << win.count()
             << " 筆, 平均 " << win.average()
             << ", 峰值 " << win.peak() << endl;
    }
    cout << "（視窗滿 5 筆後，每進一筆就 pop_front 淘汰最舊的，O(1)）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 22\ 課：deque\ 的宣告與初始化1.cpp -o deque_init1

// === 預期輸出 ===
// ===== 九種初始化方式 =====
// d1 空deque: (size=0)
// d2 五個0 : 0 0 0 0 0 (size=5)
// d3 四個77: 77 77 77 77 (size=4)
// d4 列表  : 10 20 30 40 50 (size=5)
// d5 複製d4: 10 20 30 40 50 (size=5)
// 修改d5後 : 999 20 30 40 50 (size=5)
// d4不受影響: 10 20 30 40 50 (size=5)
// d6 移動d5: 999 20 30 40 50 (size=5)
// d5 被移走 : (size=0)
// d7 從vec : 100 200 300 (size=3)
// d7 assign: 7 8 9 10 (size=4)
// d8 (5,10): 10 10 10 10 10 (size=5)
// d9 {5,10}: 5 10 (size=2)
//
// ===== 括號陷阱：換成 string 就翻轉 =====
// deque<string> s1{5,"x"} → size=5，內容: x x x x x
// （同樣寫 {5, ...}，int 版得到 2 個元素，string 版得到 5 個！）
//
// ===== deque 沒有 vector 的容量介面 =====
// deque 有 size()        → 5
// deque 有 max_size()    → 很大（實作定義）
// deque 沒有 capacity() / reserve() / data()（不連續，編譯不過）
// 但 deque 有 shrink_to_fit()（容易記錯的一個）
//
// ===== LeetCode 641. Design Circular Deque =====
// insertLast(1)  → true
// insertLast(2)  → true
// insertFront(3) → true
// insertFront(4) → false (已滿，失敗)
// getRear()      → 2
// isFull()       → true
// deleteLast()   → true
// insertFront(4) → true
// getFront()     → 4
//
// ===== 日常實務：感測器最近 5 筆讀數 =====
//   進料 22.5°C → 視窗內 1 筆, 平均 22.5, 峰值 22.5
//   進料 23.1°C → 視窗內 2 筆, 平均 22.8, 峰值 23.1
//   進料 24.8°C → 視窗內 3 筆, 平均 23.4667, 峰值 24.8
//   進料 26°C → 視窗內 4 筆, 平均 24.1, 峰值 26
//   進料 27.3°C → 視窗內 5 筆, 平均 24.74, 峰值 27.3
//   進料 31.9°C → 視窗內 5 筆, 平均 26.62, 峰值 31.9
//   進料 33.4°C → 視窗內 5 筆, 平均 28.68, 峰值 33.4
// （視窗滿 5 筆後，每進一筆就 pop_front 淘汰最舊的，O(1)）
