// ============================================================
// 第 31～35 課總結：移動語義（Move Semantics）
// 涵蓋：右值引用、移動建構、移動賦值、五法則、std::move
// 編譯：g++ -std=c++17 -o move_semantics summary27-35-移動語義.cpp
// ============================================================

#include <iostream>
#include <cstring>
#include <algorithm>
#include <utility>     // std::move, std::swap
#include <vector>
#include <string>
#include <type_traits> // is_copy_constructible, etc.

using namespace std;

// ============================================================
// 第 31 課：右值引用（Rvalue Reference）入門
// ============================================================
// 【核心觀念】
// Lvalue（左值）：有名字、可取位址的表達式 → int x = 10; x 是左值
// Rvalue（右值）：臨時的、沒有名字的表達式 → 10, x+y, func() 回傳的臨時物件
//
// 【引用類型與綁定規則】
//   T&        左值引用     → 只能綁定左值
//   const T&  const左值引用 → 可綁定左值和右值（萬能引用讀取）
//   T&&       右值引用     → 只能綁定右值
//
// 【重要陷阱】右值引用變數本身是左值！
//   int&& rref = 42;   // rref 綁定右值 42
//   // 但 rref 本身有名字、可取位址 → rref 是左值！
//   // int&& rref2 = rref;  // ❌ 錯誤！rref 是左值，不能綁定到 T&&
//   // int&& rref2 = std::move(rref);  // ✅ 用 std::move 轉成右值

namespace Lesson31 {

void processLvalue(int& x)        { cout << "  左值引用版本: " << x << endl; }
void processRvalue(int&& x)       { cout << "  右值引用版本: " << x << endl; }

// 函數重載：const T& vs T&& → 編譯器優先匹配 T&&
void overloaded(const string& s)  { cout << "  const T& 版本: " << s << endl; }
void overloaded(string&& s)       { cout << "  T&& 版本: " << s << endl; }

void demo() {
    cout << "=== 第 31 課：右值引用入門 ===" << endl;

    // 綁定規則
    cout << "\n--- 綁定規則 ---" << endl;
    int a = 10;
    int& lref = a;           // 左值引用 → 綁定左值 ✅
    const int& clref = 42;   // const 左值引用 → 綁定右值 ✅（萬能）
    int&& rref = 42;         // 右值引用 → 綁定右值 ✅

    processLvalue(a);         // 左值 → T&
    processRvalue(100);       // 右值 → T&&
    // processRvalue(a);      // ❌ 左值不能綁到 T&&
    processRvalue(move(a));   // ✅ std::move 把左值轉成右值

    // 重要陷阱：右值引用變數本身是左值
    cout << "\n--- 右值引用變數是左值 ---" << endl;
    int&& r = 42;
    cout << "  r 的值: " << r << endl;
    cout << "  &r = " << &r << " ← 可取位址，所以 r 是左值" << endl;
    // int&& r2 = r;    // ❌ 編譯錯誤！r 是左值
    int&& r2 = move(r); // ✅ 需要 std::move
    cout << "  r2 = " << r2 << endl;

    // 函數重載選擇
    cout << "\n--- 函數重載：const T& vs T&& ---" << endl;
    string name = "Alice";
    overloaded(name);              // 左值 → const T& 版本
    overloaded("Bob"s);            // 右值 → T&& 版本（優先匹配）
    overloaded(move(name));        // move 後 → T&& 版本
}

} // namespace Lesson31

// ============================================================
// 第 32 課：移動建構函數（Move Constructor）
// ============================================================
// 【核心觀念】
// 簽名：ClassName(ClassName&& other) noexcept
//
// 移動三步驟：
//   1. 偷取（steal）：把 other 的指標複製給自己
//   2. 歸零（nullify）：把 other 的指標設為 nullptr
//   3. 複製基本型別：直接複製 int, size_t 等
//
// 移動後的 other 處於「有效但未指定」狀態（valid but unspecified）
// → other 可以安全解構（delete nullptr 是安全的）
//
// 【noexcept 很重要！】
//   vector 擴容時，如果移動建構沒有 noexcept，vector 會用拷貝而非移動
//   → 因為 vector 需要強異常安全保證
//
// 【自動生成規則】
//   如果定義了自訂解構函數、拷貝建構或拷貝賦值 → 編譯器不會自動生成移動建構
//   → 這就是為什麼需要「五法則」

namespace Lesson32 {

class MoveString {
    char* m_data;
    size_t m_len;
public:
    MoveString(const char* str = "") : m_len(strlen(str)) {
        m_data = new char[m_len + 1];
        strcpy(m_data, str);
        cout << "  [建構] \"" << m_data << "\"" << endl;
    }

    // 拷貝建構（深拷貝）— 昂貴
    MoveString(const MoveString& other)
        : m_len(other.m_len), m_data(new char[other.m_len + 1]) {
        strcpy(m_data, other.m_data);
        cout << "  [拷貝建構💰] \"" << m_data << "\"" << endl;
    }

    // 移動建構 — 便宜！只是偷指標
    MoveString(MoveString&& other) noexcept  // ← noexcept 很重要！
        : m_data(other.m_data),               // ① 偷取指標
          m_len(other.m_len)                   // ③ 複製基本型別
    {
        other.m_data = nullptr;               // ② 歸零源物件
        other.m_len = 0;
        cout << "  [移動建構⚡] \"" << m_data << "\"" << endl;
    }

    ~MoveString() {
        if (m_data) cout << "  [解構] \"" << m_data << "\"" << endl;
        else        cout << "  [解構] (已被移動，nullptr)" << endl;
        delete[] m_data;  // delete nullptr 是安全的
    }

    const char* c_str() const { return m_data ? m_data : "(null)"; }
};

void demo() {
    cout << "\n=== 第 32 課：移動建構函數 ===" << endl;

    // 基本移動
    cout << "\n--- 基本移動 ---" << endl;
    MoveString a("Hello");
    MoveString b(move(a));   // 移動建構：偷取 a 的資源
    cout << "  a: " << a.c_str() << endl;  // (null) — 已被移動
    cout << "  b: " << b.c_str() << endl;  // Hello

    // noexcept 對 vector 的影響
    cout << "\n--- noexcept 對 vector 的影響 ---" << endl;
    cout << "  有 noexcept → vector 擴容時使用移動（快）" << endl;
    cout << "  無 noexcept → vector 擴容時使用拷貝（慢）" << endl;
    vector<MoveString> vec;
    vec.reserve(2);  // 預留空間避免干擾
    vec.push_back(MoveString("Vec1"));  // 移動臨時物件
    vec.push_back(MoveString("Vec2"));  // 移動臨時物件

    // 自動生成規則
    cout << "\n--- 自動生成規則 ---" << endl;
    cout << "  定義了解構函數    → 不自動生成移動建構/賦值" << endl;
    cout << "  定義了拷貝建構    → 不自動生成移動建構/賦值" << endl;
    cout << "  定義了拷貝賦值    → 不自動生成移動建構/賦值" << endl;
    cout << "  → 所以要嘛全不寫（Rule of Zero），要嘛五個都寫（Rule of Five）" << endl;
}

} // namespace Lesson32

// ============================================================
// 第 33 課：移動賦值運算子（Move Assignment Operator）
// ============================================================
// 【核心觀念】
// 簽名：ClassName& operator=(ClassName&& other) noexcept
//
// 移動賦值步驟：
//   1. 自我賦值檢查：if (this == &other) return *this;
//   2. 釋放自己的舊資源：delete[] m_data;
//   3. 偷取 other 的資源：m_data = other.m_data;
//   4. 歸零 other：other.m_data = nullptr;
//
// 【統一賦值運算子（Unified Assignment）】推薦寫法
//   ClassName& operator=(ClassName other) {  // 傳值
//       swap(*this, other);
//       return *this;
//   }
//   → 一個函數同時處理拷貝賦值和移動賦值！
//   → 傳左值時：觸發拷貝建構 → swap
//   → 傳右值時：觸發移動建構 → swap

namespace Lesson33 {

class Buffer {
    int* m_data;
    size_t m_size;
public:
    Buffer(size_t size = 0) : m_data(size ? new int[size]{} : nullptr), m_size(size) {
        cout << "  [Buffer 建構] size=" << m_size << endl;
    }

    // 拷貝建構
    Buffer(const Buffer& other)
        : m_data(other.m_size ? new int[other.m_size] : nullptr),
          m_size(other.m_size) {
        if (m_data) copy(other.m_data, other.m_data + m_size, m_data);
        cout << "  [Buffer 拷貝建構💰]" << endl;
    }

    // 移動建構
    Buffer(Buffer&& other) noexcept
        : m_data(other.m_data), m_size(other.m_size) {
        other.m_data = nullptr;
        other.m_size = 0;
        cout << "  [Buffer 移動建構⚡]" << endl;
    }

    // --- 方法一：分開寫拷貝賦值和移動賦值 ---
    // 拷貝賦值
    // Buffer& operator=(const Buffer& other) { ... }
    // 移動賦值
    // Buffer& operator=(Buffer&& other) noexcept { ... }

    // --- 方法二：統一賦值（推薦）---
    friend void swap(Buffer& a, Buffer& b) noexcept {
        using std::swap;
        swap(a.m_data, b.m_data);
        swap(a.m_size, b.m_size);
    }

    Buffer& operator=(Buffer other) {  // 傳值！
        // 傳左值 → other 由拷貝建構產生
        // 傳右值 → other 由移動建構產生
        cout << "  [Buffer 統一賦值] swap" << endl;
        swap(*this, other);
        return *this;
    } // other 離開作用域，自動釋放舊資源

    ~Buffer() {
        delete[] m_data;
    }

    void fill(int val) { for (size_t i = 0; i < m_size; ++i) m_data[i] = val; }
    int get(size_t i) const { return m_data ? m_data[i] : -1; }
    size_t size() const { return m_size; }
};

void demo() {
    cout << "\n=== 第 33 課：移動賦值運算子 ===" << endl;

    cout << "\n--- 統一賦值同時處理拷貝和移動 ---" << endl;
    Buffer a(3);
    a.fill(10);

    // 拷貝賦值路徑：傳左值 → 拷貝建構 → swap
    cout << "\n  拷貝賦值路徑（左值）：" << endl;
    Buffer b(2);
    b = a;  // a 是左值 → 拷貝建構 other → swap
    cout << "  b[0]=" << b.get(0) << endl;  // 10

    // 移動賦值路徑：傳右值 → 移動建構 → swap
    cout << "\n  移動賦值路徑（右值）：" << endl;
    Buffer c(2);
    c = move(a);  // move(a) 是右值 → 移動建構 other → swap
    cout << "  c[0]=" << c.get(0) << endl;  // 10
    cout << "  a.size()=" << a.size() << " (a 被交換為 c 的舊狀態)" << endl;
}

} // namespace Lesson33

// ============================================================
// 第 34 課：五法則（Rule of Five）
// ============================================================
// 【核心觀念】
// Rule of Five = Rule of Three + 移動建構 + 移動賦值
//   1. 解構函數           ~ClassName()
//   2. 拷貝建構函數       ClassName(const ClassName&)
//   3. 拷貝賦值運算子     ClassName& operator=(const ClassName&)
//   4. 移動建構函數       ClassName(ClassName&&) noexcept
//   5. 移動賦值運算子     ClassName& operator=(ClassName&&) noexcept
//
// 若使用統一賦值（pass-by-value + swap），只需要 4 個：
//   解構 + 拷貝建構 + 移動建構 + 統一賦值
//
// 【Rule of Zero】
//   如果類別只使用 RAII 成員（string, vector, unique_ptr...）
//   → 不需要自訂任何一個，全部用編譯器預設的
//
// 【type_traits 檢查】
//   is_copy_constructible_v<T>   是否可拷貝建構
//   is_move_constructible_v<T>   是否可移動建構
//   is_copy_assignable_v<T>      是否可拷貝賦值
//   is_move_assignable_v<T>      是否可移動賦值
//   is_destructible_v<T>         是否可解構

namespace Lesson34 {

class ManagedArray {
    int* m_data;
    size_t m_size;

public:
    // 建構函數
    explicit ManagedArray(size_t size = 0)
        : m_data(size ? new int[size]{} : nullptr), m_size(size) {}

    // ===== Rule of Five（使用統一賦值，實際只寫 4 個）=====

    // ① 解構函數
    ~ManagedArray() {
        delete[] m_data;
    }

    // ② 拷貝建構函數（深拷貝）
    ManagedArray(const ManagedArray& other)
        : m_data(other.m_size ? new int[other.m_size] : nullptr),
          m_size(other.m_size) {
        if (m_data) copy(other.m_data, other.m_data + m_size, m_data);
    }

    // ③ 移動建構函數（偷資源）
    ManagedArray(ManagedArray&& other) noexcept
        : m_data(other.m_data), m_size(other.m_size) {
        other.m_data = nullptr;
        other.m_size = 0;
    }

    // ④⑤ 統一賦值運算子（同時處理拷貝賦值和移動賦值）
    friend void swap(ManagedArray& a, ManagedArray& b) noexcept {
        using std::swap;
        swap(a.m_data, b.m_data);
        swap(a.m_size, b.m_size);
    }

    ManagedArray& operator=(ManagedArray other) {  // 傳值
        swap(*this, other);
        return *this;
    }

    // ===== Rule of Five 結束 =====

    size_t size() const { return m_size; }
    int& operator[](size_t i) { return m_data[i]; }
    const int& operator[](size_t i) const { return m_data[i]; }
};

void demo() {
    cout << "\n=== 第 34 課：五法則（Rule of Five）===" << endl;

    // 完整示範
    cout << "\n--- 完整 Rule of Five 類別使用 ---" << endl;
    ManagedArray a(3);
    a[0] = 10; a[1] = 20; a[2] = 30;

    ManagedArray b(a);          // 拷貝建構
    ManagedArray c(move(a));    // 移動建構（a 被清空）
    ManagedArray d(5);
    d = b;                      // 統一賦值（拷貝路徑）
    ManagedArray e(5);
    e = move(b);                // 統一賦值（移動路徑）

    cout << "  c[0]=" << c[0] << " c[1]=" << c[1] << " c[2]=" << c[2] << endl;
    cout << "  d[0]=" << d[0] << " d[1]=" << d[1] << " d[2]=" << d[2] << endl;
    cout << "  a.size()=" << a.size() << " (已被移動)" << endl;
    cout << "  b.size()=" << b.size() << " (已被移動)" << endl;

    // type_traits 檢查
    cout << "\n--- type_traits 檢查 ---" << endl;
    cout << boolalpha;
    cout << "  is_copy_constructible: " << is_copy_constructible_v<ManagedArray> << endl;
    cout << "  is_move_constructible: " << is_move_constructible_v<ManagedArray> << endl;
    cout << "  is_copy_assignable:    " << is_copy_assignable_v<ManagedArray> << endl;
    cout << "  is_move_assignable:    " << is_move_assignable_v<ManagedArray> << endl;
    cout << "  is_destructible:       " << is_destructible_v<ManagedArray> << endl;

    // Rule of Zero 說明
    cout << "\n--- Rule of Zero ---" << endl;
    cout << "  如果類別只用 string, vector, unique_ptr 等 RAII 成員" << endl;
    cout << "  → 不需要寫任何一個（解構、拷貝、移動）" << endl;
    cout << "  → 編譯器預設的就是正確的" << endl;

    struct SafeClass {   // Rule of Zero 範例
        string name;     // RAII 成員，自動管理記憶體
        vector<int> data;
        // 不需要寫解構、拷貝建構、拷貝賦值、移動建構、移動賦值！
    };
    SafeClass s1{"Hello", {1,2,3}};
    SafeClass s2 = s1;           // 安全拷貝
    SafeClass s3 = move(s1);     // 安全移動
    cout << "  SafeClass s2.name=" << s2.name << " s3.name=" << s3.name << endl;
}

} // namespace Lesson34

// ============================================================
// 第 35 課：std::move 的使用
// ============================================================
// 【核心觀念】
// std::move(x) 本身不移動任何東西！
// 它只是一個 static_cast<T&&>(x)，把左值轉成右值引用
// 真正的移動發生在接收端的移動建構/移動賦值中
//
// 【五大使用場景】
//  1. 明確不再需要的局部變數 → 傳給函數或容器
//  2. 建構函數中的 pass-by-value + move 慣用法
//  3. push_back(move(obj)) 避免不必要的拷貝
//  4. 從容器中提取元素 → auto item = move(container.back())
//  5. 實現 swap → 透過三次移動交換兩個物件
//
// 【注意事項】
//  - 不要 move const 物件 → const T&& 無法觸發移動，會退化為拷貝
//  - 不要 move 函數回傳的局部變數 → 會阻止 NRVO 優化
//  - move 後的物件處於有效但未指定狀態，不要再使用其值

namespace Lesson35 {

class TrackedString {
    string m_data;
    static int s_copyCount;
    static int s_moveCount;
public:
    TrackedString(const string& s = "") : m_data(s) {}

    TrackedString(const TrackedString& other) : m_data(other.m_data) {
        ++s_copyCount;
        cout << "  [TrackedString 拷貝💰] \"" << m_data << "\"" << endl;
    }

    TrackedString(TrackedString&& other) noexcept : m_data(move(other.m_data)) {
        ++s_moveCount;
        cout << "  [TrackedString 移動⚡] \"" << m_data << "\"" << endl;
    }

    TrackedString& operator=(TrackedString other) {
        swap(m_data, other.m_data);
        return *this;
    }

    const string& str() const { return m_data; }

    static void resetCounters() { s_copyCount = s_moveCount = 0; }
    static void printCounters() {
        cout << "  拷貝次數: " << s_copyCount
             << "  移動次數: " << s_moveCount << endl;
    }
};
int TrackedString::s_copyCount = 0;
int TrackedString::s_moveCount = 0;

// --- 場景 2：建構函數中 pass-by-value + move ---
class Hero {
    TrackedString m_name;
    int m_hp;
public:
    // 傳值 + move 慣用法
    // 傳左值：拷貝到 name → move 到 m_name（1 拷貝 + 1 移動）
    // 傳右值：移動到 name → move 到 m_name（2 移動，零拷貝）
    Hero(TrackedString name, int hp)
        : m_name(move(name)),  // ← move 傳值參數到成員
          m_hp(hp) {}

    void print() const {
        cout << "  Hero: " << m_name.str() << " HP=" << m_hp << endl;
    }
};

void demo() {
    cout << "\n=== 第 35 課：std::move 的使用 ===" << endl;

    // 場景 1：明確不再需要的局部變數
    cout << "\n--- 場景 1：移動局部變數到容器 ---" << endl;
    TrackedString::resetCounters();
    vector<TrackedString> party;
    party.reserve(3);  // 預留空間避免擴容干擾

    TrackedString warrior("Warrior");
    party.push_back(warrior);         // 拷貝（warrior 之後還要用）
    party.push_back(move(warrior));   // 移動（warrior 之後不用了）
    cout << "  warrior 移動後: \"" << warrior.str() << "\" (空字串)" << endl;
    TrackedString::printCounters();

    // 場景 2：pass-by-value + move 建構函數
    cout << "\n--- 場景 2：pass-by-value + move ---" << endl;
    TrackedString::resetCounters();
    cout << "  傳左值：" << endl;
    TrackedString name1("Alice");
    Hero h1(name1, 100);      // 拷貝到參數 → move 到成員
    TrackedString::printCounters();

    TrackedString::resetCounters();
    cout << "  傳右值：" << endl;
    Hero h2(TrackedString("Bob"), 200);  // 移動到參數 → move 到成員
    TrackedString::printCounters();

    // 場景 3：push_back(move(x)) vs emplace_back
    cout << "\n--- 場景 3：push_back vs emplace_back ---" << endl;
    cout << "  push_back(obj)       → 拷貝" << endl;
    cout << "  push_back(move(obj)) → 移動" << endl;
    cout << "  emplace_back(args)   → 原地建構（最佳，零拷貝零移動）" << endl;
    vector<string> names;
    names.reserve(3);
    string s = "Charlie";
    names.push_back(s);           // 拷貝
    names.push_back(move(s));     // 移動
    names.emplace_back("David");  // 原地建構（直接在 vector 記憶體中建構）

    // 場景 4：從容器提取元素
    cout << "\n--- 場景 4：從容器提取元素 ---" << endl;
    vector<string> items = {"Sword", "Shield", "Potion"};
    string extracted = move(items.back());  // 移動而非拷貝
    items.pop_back();
    cout << "  提取: " << extracted << endl;
    cout << "  剩餘 items.size()=" << items.size() << endl;

    // 場景 5：用 move 實現 swap
    cout << "\n--- 場景 5：用 move 實現 swap ---" << endl;
    string x = "AAA", y = "BBB";
    cout << "  交換前: x=" << x << " y=" << y << endl;
    // std::swap 的內部實現大致如下：
    // string temp = move(x);  // ① x → temp
    // x = move(y);            // ② y → x
    // y = move(temp);         // ③ temp → y
    swap(x, y);
    cout << "  交換後: x=" << x << " y=" << y << endl;

    // 注意事項
    cout << "\n--- 注意事項 ---" << endl;
    cout << "  ❌ 不要 move const 物件 → 會退化為拷貝" << endl;
    cout << "     const string cs = \"hi\";" << endl;
    cout << "     string s2 = move(cs);  // 實際上是拷貝！" << endl;
    cout << "  ❌ 不要 move 函數回傳的局部變數 → 會阻止 NRVO" << endl;
    cout << "     return move(localObj);  // 不好！阻止編譯器優化" << endl;
    cout << "     return localObj;        // 好！讓編譯器做 NRVO" << endl;
    cout << "  ❌ move 後不要再使用物件的值（可以重新賦值或解構）" << endl;

    // 驗證 const move 退化為拷貝
    cout << "\n--- 驗證 const move 退化為拷貝 ---" << endl;
    TrackedString::resetCounters();
    const TrackedString constStr("Immovable");
    TrackedString copied = move(constStr);  // const → 無法移動 → 退化為拷貝
    TrackedString::printCounters();  // 拷貝: 1, 移動: 0
}

} // namespace Lesson35

// ============================================================
// main
// ============================================================
int main() {
    Lesson31::demo();
    Lesson32::demo();
    Lesson33::demo();
    Lesson34::demo();
    Lesson35::demo();

    cout << "\n========================================" << endl;
    cout << "第 31～35 課總結完畢（移動語義）" << endl;
    cout << "========================================" << endl;
    cout << "\n--- 完整知識路線圖 ---" << endl;
    cout << "第 27 課：淺拷貝 vs 深拷貝 → 認識問題" << endl;
    cout << "第 28 課：拷貝建構函數     → 解決「初始化」時的拷貝" << endl;
    cout << "第 29 課：拷貝賦值運算子   → 解決「賦值」時的拷貝" << endl;
    cout << "第 30 課：三法則           → 統整拷貝語義" << endl;
    cout << "第 31 課：右值引用         → 為移動語義鋪路" << endl;
    cout << "第 32 課：移動建構函數     → 偷資源代替深拷貝" << endl;
    cout << "第 33 課：移動賦值運算子   → 賦值時也能偷資源" << endl;
    cout << "第 34 課：五法則           → 統整拷貝+移動語義" << endl;
    cout << "第 35 課：std::move        → 實戰使用技巧" << endl;

    return 0;
}
