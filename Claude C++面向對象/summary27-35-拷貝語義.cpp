// ============================================================
// 第 27～30 課總結：拷貝語義（Copy Semantics）
// 涵蓋：淺拷貝 vs 深拷貝、拷貝建構函數、拷貝賦值運算子、三法則
// 編譯：g++ -std=c++17 -o copy_semantics summary27-35-拷貝語義.cpp
// ============================================================

#include <iostream>
#include <cstring>    // strlen, strcpy
#include <algorithm>  // std::copy, std::swap
#include <utility>    // std::swap

using namespace std;

// ============================================================
// 第 27 課：淺拷貝 vs 深拷貝（Shallow Copy vs Deep Copy）
// ============================================================
// 【核心觀念】
// 淺拷貝（Shallow Copy）：只複製指標本身，兩個物件指向同一塊記憶體
//   → 問題：其中一個物件解構後，另一個變成 dangling pointer → double free
// 深拷貝（Deep Copy）：分配新的記憶體，把資料內容完整複製過去
//   → 兩個物件各自擁有獨立的記憶體，互不影響

namespace Lesson27 {

// --- 淺拷貝問題示範 ---
class ShallowString {
    char* m_data;
    size_t m_len;
public:
    ShallowString(const char* str) {
        m_len = strlen(str);
        m_data = new char[m_len + 1];
        strcpy(m_data, str);
        cout << "  [建構] 分配記憶體 @" << (void*)m_data << endl;
    }

    ~ShallowString() {
        cout << "  [解構] 釋放記憶體 @" << (void*)m_data << endl;
        delete[] m_data;  // 淺拷貝時，兩個物件都會 delete 同一塊 → crash!
    }

    // 編譯器預設產生的拷貝建構函數 = 淺拷貝
    // ShallowString(const ShallowString& other)
    //     : m_data(other.m_data), m_len(other.m_len) {}
    // ↑ 只複製指標值，不複製指標指向的資料！

    void print() const { cout << "  內容: " << m_data << endl; }
};

// --- 深拷貝解決方案 ---
class DeepString {
    char* m_data;
    size_t m_len;
public:
    DeepString(const char* str) {
        m_len = strlen(str);
        m_data = new char[m_len + 1];
        strcpy(m_data, str);
        cout << "  [建構] 分配記憶體 @" << (void*)m_data << endl;
    }

    // 深拷貝建構函數：分配新記憶體 + 複製資料
    DeepString(const DeepString& other) {
        m_len = other.m_len;
        m_data = new char[m_len + 1];     // ← 分配新的記憶體
        strcpy(m_data, other.m_data);     // ← 複製資料內容
        cout << "  [深拷貝建構] 新記憶體 @" << (void*)m_data
             << " (來源 @" << (void*)other.m_data << ")" << endl;
    }

    ~DeepString() {
        cout << "  [解構] 釋放記憶體 @" << (void*)m_data << endl;
        delete[] m_data;  // 各自釋放各自的記憶體，安全！
    }

    void print() const { cout << "  內容: " << m_data << endl; }
};

void demo() {
    cout << "=== 第 27 課：淺拷貝 vs 深拷貝 ===" << endl;

    // 淺拷貝問題（僅說明，不實際執行以避免 crash）
    cout << "\n--- 淺拷貝問題（概念說明）---" << endl;
    cout << "  ShallowString a(\"hello\");" << endl;
    cout << "  ShallowString b = a;  // 淺拷貝：b.m_data == a.m_data" << endl;
    cout << "  // a 解構 → delete m_data" << endl;
    cout << "  // b 解構 → delete 同一塊 m_data → 💥 double free!" << endl;

    // 深拷貝正確示範
    cout << "\n--- 深拷貝正確示範 ---" << endl;
    {
        DeepString a("Hello");
        DeepString b = a;  // 呼叫深拷貝建構函數
        a.print();
        b.print();
        cout << "  （a 和 b 有各自獨立的記憶體，解構安全）" << endl;
    } // a, b 各自 delete 各自的記憶體 → OK
}

} // namespace Lesson27

// ============================================================
// 第 28 課：拷貝建構函數（Copy Constructor）
// ============================================================
// 【核心觀念】
// 簽名：ClassName(const ClassName& other)
// 參數必須是 const 引用（避免無限遞迴 + 接受 const/rvalue）
//
// 【六大觸發場景】
//  1. 直接初始化：MyClass b(a);
//  2. 拷貝初始化：MyClass b = a;
//  3. 函數傳值參數：void func(MyClass obj) → 傳入時拷貝
//  4. 函數回傳值：return obj; → 回傳時拷貝（可能被 RVO 省略）
//  5. 陣列初始化：MyClass arr[] = {a, b};
//  6. 容器操作：vector.push_back(obj);
//
// 【Copy Elision / RVO / NRVO】
//  RVO (Return Value Optimization)：回傳臨時物件時省略拷貝
//    → return MyClass("hi");  // 直接在呼叫者的空間建構
//  NRVO (Named RVO)：回傳具名物件時省略拷貝
//    → MyClass obj("hi"); return obj;
//  C++17 起，RVO 是強制的（mandatory），NRVO 仍是可選優化

namespace Lesson28 {

class Tracked {
    string m_name;
public:
    Tracked(const string& name) : m_name(name) {
        cout << "  [建構] " << m_name << endl;
    }
    Tracked(const Tracked& other) : m_name(other.m_name + "_copy") {
        cout << "  [拷貝建構] " << m_name << " ← " << other.m_name << endl;
    }
    ~Tracked() { cout << "  [解構] " << m_name << endl; }
    const string& name() const { return m_name; }
};

// 場景3：函數傳值 → 觸發拷貝建構
void passByValue(Tracked obj) {
    cout << "  函數內使用: " << obj.name() << endl;
}

// 場景4：函數回傳 → 可能觸發拷貝（但通常被 RVO/NRVO 省略）
Tracked createTracked() {
    Tracked local("fromFunc");
    return local;  // NRVO 可能省略拷貝
}

// --- 含指標的深拷貝建構 ---
class IntArray {
    int* m_data;
    size_t m_size;
public:
    IntArray(size_t size, int initVal = 0)
        : m_data(new int[size]), m_size(size) {
        for (size_t i = 0; i < size; ++i) m_data[i] = initVal;
    }

    // 深拷貝建構函數
    IntArray(const IntArray& other)
        : m_data(new int[other.m_size]), m_size(other.m_size) {
        // 使用 std::copy 複製陣列內容
        copy(other.m_data, other.m_data + m_size, m_data);
        cout << "  [IntArray 深拷貝] size=" << m_size << endl;
    }

    ~IntArray() { delete[] m_data; }

    void set(size_t idx, int val) { m_data[idx] = val; }
    int get(size_t idx) const { return m_data[idx]; }
    size_t size() const { return m_size; }
};

void demo() {
    cout << "\n=== 第 28 課：拷貝建構函數 ===" << endl;

    // 場景 1 & 2：直接初始化 / 拷貝初始化
    cout << "\n--- 場景 1 & 2：直接初始化 / 拷貝初始化 ---" << endl;
    Tracked a("original");
    Tracked b(a);       // 場景1：直接初始化
    Tracked c = a;      // 場景2：拷貝初始化（效果同上）

    // 場景 3：傳值參數
    cout << "\n--- 場景 3：函數傳值 ---" << endl;
    passByValue(a);     // 傳入時觸發拷貝建構

    // 場景 4：函數回傳（通常被 NRVO 省略）
    cout << "\n--- 場景 4：函數回傳（NRVO 可能省略拷貝）---" << endl;
    Tracked d = createTracked();
    cout << "  得到: " << d.name() << endl;

    // 含指標的深拷貝
    cout << "\n--- IntArray 深拷貝示範 ---" << endl;
    IntArray arr1(3, 10);   // [10, 10, 10]
    IntArray arr2 = arr1;   // 深拷貝
    arr2.set(0, 99);        // 修改 arr2 不影響 arr1
    cout << "  arr1[0]=" << arr1.get(0) << " arr2[0]=" << arr2.get(0) << endl;
    // 輸出：arr1[0]=10 arr2[0]=99（各自獨立）
}

} // namespace Lesson28

// ============================================================
// 第 29 課：拷貝賦值運算子（Copy Assignment Operator）
// ============================================================
// 【核心觀念】
// 簽名：ClassName& operator=(const ClassName& other)
// 拷貝建構 vs 拷貝賦值的區別：
//   MyClass b = a;  → 拷貝建構（物件尚未存在，初始化時呼叫）
//   b = a;          → 拷貝賦值（物件已存在，賦值時呼叫）
//
// 【自我賦值檢查】
//   if (this == &other) return *this;
//   → 防止 a = a 時先 delete 自己的資料再複製（已被刪除的）資料
//
// 【Copy-and-Swap 慣用法】推薦的安全寫法
//   ClassName& operator=(ClassName other) {  // 注意：傳值（觸發拷貝）
//       swap(*this, other);
//       return *this;
//   }
//   → 異常安全（如果 new 失敗，原物件不受影響）
//   → 自動處理自我賦值
//
// 【隱式刪除的情況】
//   類別含 const 成員或引用成員 → 編譯器不會生成 copy assignment
//   → 若需要，可用 placement new 搭配手動解構來繞過（進階技巧）

namespace Lesson29 {

class ManagedBuffer {
    int* m_data;
    size_t m_size;
public:
    ManagedBuffer(size_t size, int initVal = 0)
        : m_data(new int[size]), m_size(size) {
        for (size_t i = 0; i < size; ++i) m_data[i] = initVal;
    }

    // 深拷貝建構函數
    ManagedBuffer(const ManagedBuffer& other)
        : m_data(new int[other.m_size]), m_size(other.m_size) {
        copy(other.m_data, other.m_data + m_size, m_data);
        cout << "  [ManagedBuffer 拷貝建構]" << endl;
    }

    // 拷貝賦值運算子 — 傳統寫法
    ManagedBuffer& operator=(const ManagedBuffer& other) {
        cout << "  [ManagedBuffer 拷貝賦值]" << endl;

        if (this == &other) return *this;  // ① 自我賦值檢查

        delete[] m_data;                   // ② 釋放舊資源

        m_size = other.m_size;             // ③ 複製大小
        m_data = new int[m_size];          // ④ 分配新記憶體
        copy(other.m_data, other.m_data + m_size, m_data);  // ⑤ 複製資料

        return *this;                      // ⑥ 回傳自身引用（支援鏈式賦值）
    }

    ~ManagedBuffer() { delete[] m_data; }

    int get(size_t i) const { return m_data[i]; }
    void set(size_t i, int v) { m_data[i] = v; }
    size_t size() const { return m_size; }
};

// --- Copy-and-Swap 慣用法（推薦）---
class SwapBuffer {
    int* m_data;
    size_t m_size;
public:
    SwapBuffer(size_t size, int initVal = 0)
        : m_data(new int[size]), m_size(size) {
        for (size_t i = 0; i < size; ++i) m_data[i] = initVal;
    }

    SwapBuffer(const SwapBuffer& other)
        : m_data(new int[other.m_size]), m_size(other.m_size) {
        copy(other.m_data, other.m_data + m_size, m_data);
    }

    // 提供 swap 函數
    friend void swap(SwapBuffer& a, SwapBuffer& b) noexcept {
        using std::swap;
        swap(a.m_data, b.m_data);
        swap(a.m_size, b.m_size);
    }

    // Copy-and-Swap：參數傳值（自動觸發拷貝建構）
    SwapBuffer& operator=(SwapBuffer other) {  // ← 傳值，不是 const&
        cout << "  [SwapBuffer Copy-and-Swap 賦值]" << endl;
        swap(*this, other);  // 交換後，other 持有舊資料
        return *this;        // other 離開作用域時自動釋放舊資料
    }
    // 優點：
    //   1. 異常安全 — new 失敗時（在拷貝建構階段），原物件不受影響
    //   2. 自動處理自我賦值 — a = a 時，other 是 a 的拷貝，交換後仍正確
    //   3. 程式碼簡潔

    ~SwapBuffer() { delete[] m_data; }

    int get(size_t i) const { return m_data[i]; }
    void set(size_t i, int v) { m_data[i] = v; }
};

// --- 隱式刪除示範 ---
class HasConst {
    const int m_id;     // const 成員 → 編譯器不生成 copy assignment
    string m_name;
public:
    HasConst(int id, const string& name) : m_id(id), m_name(name) {}
    // HasConst& operator=(const HasConst&) 被隱式刪除
    // HasConst a(1,"A"), b(2,"B"); b = a; → 編譯錯誤！
    void print() const { cout << "  id=" << m_id << " name=" << m_name << endl; }
};

class HasRef {
    int& m_ref;         // 引用成員 → 編譯器不生成 copy assignment
    string m_name;
public:
    HasRef(int& ref, const string& name) : m_ref(ref), m_name(name) {}
    // HasRef& operator=(const HasRef&) 被隱式刪除
};

void demo() {
    cout << "\n=== 第 29 課：拷貝賦值運算子 ===" << endl;

    // 拷貝建構 vs 拷貝賦值
    cout << "\n--- 拷貝建構 vs 拷貝賦值 ---" << endl;
    ManagedBuffer a(3, 10);
    ManagedBuffer b = a;   // 拷貝「建構」— b 尚未存在
    ManagedBuffer c(2, 20);
    c = a;                 // 拷貝「賦值」— c 已存在

    cout << "  a[0]=" << a.get(0) << " b[0]=" << b.get(0)
         << " c[0]=" << c.get(0) << endl;

    // 鏈式賦值
    cout << "\n--- 鏈式賦值 ---" << endl;
    ManagedBuffer x(2, 1), y(2, 2), z(2, 3);
    x = y = z;  // z 賦值給 y，y 賦值給 x
    cout << "  x[0]=" << x.get(0) << " y[0]=" << y.get(0)
         << " z[0]=" << z.get(0) << endl;  // 全部是 3

    // Copy-and-Swap 示範
    cout << "\n--- Copy-and-Swap ---" << endl;
    SwapBuffer s1(3, 100), s2(2, 200);
    s2 = s1;
    cout << "  s2[0]=" << s2.get(0) << endl;  // 100

    // 隱式刪除說明
    cout << "\n--- 隱式刪除說明 ---" << endl;
    cout << "  含 const 成員 → 拷貝賦值被隱式刪除" << endl;
    cout << "  含引用成員   → 拷貝賦值被隱式刪除" << endl;
    cout << "  （因為 const 不能重新賦值，引用不能重新綁定）" << endl;
}

} // namespace Lesson29

// ============================================================
// 第 30 課：三法則（Rule of Three）
// ============================================================
// 【核心觀念】
// 如果類別需要自訂以下任何一個，通常三個都需要自訂：
//   1. 解構函數（Destructor）
//   2. 拷貝建構函數（Copy Constructor）
//   3. 拷貝賦值運算子（Copy Assignment Operator）
//
// 為什麼？
//   → 需要自訂解構函數 = 類別管理了動態資源（如 new 出來的記憶體）
//   → 管理動態資源 = 預設的淺拷貝不安全
//   → 所以拷貝建構和拷貝賦值也必須自訂（做深拷貝）
//
// 【完整示範：ManagedString — 三法則的完整實現】

namespace Lesson30 {

class ManagedString {
    char* m_data;
    size_t m_len;

public:
    // 一般建構函數
    ManagedString(const char* str = "") {
        m_len = strlen(str);
        m_data = new char[m_len + 1];
        strcpy(m_data, str);
        cout << "  [建構] \"" << m_data << "\" @" << (void*)m_data << endl;
    }

    // ===== Rule of Three 三法則 =====

    // ① 解構函數
    ~ManagedString() {
        cout << "  [解構] \"" << m_data << "\" @" << (void*)m_data << endl;
        delete[] m_data;
    }

    // ② 拷貝建構函數（深拷貝）
    ManagedString(const ManagedString& other)
        : m_len(other.m_len), m_data(new char[other.m_len + 1]) {
        strcpy(m_data, other.m_data);
        cout << "  [拷貝建構] \"" << m_data << "\" @" << (void*)m_data
             << " ← @" << (void*)other.m_data << endl;
    }

    // ③ 拷貝賦值運算子（Copy-and-Swap）
    friend void swap(ManagedString& a, ManagedString& b) noexcept {
        using std::swap;
        swap(a.m_data, b.m_data);
        swap(a.m_len, b.m_len);
    }

    ManagedString& operator=(ManagedString other) {  // 傳值觸發拷貝
        cout << "  [拷貝賦值 Copy-and-Swap]" << endl;
        swap(*this, other);
        return *this;
    }

    // ===== 三法則結束 =====

    void print() const {
        cout << "  \"" << m_data << "\" (len=" << m_len
             << ", @" << (void*)m_data << ")" << endl;
    }
};

void demo() {
    cout << "\n=== 第 30 課：三法則（Rule of Three）===" << endl;

    cout << "\n--- 完整生命週期示範 ---" << endl;
    {
        ManagedString a("Hello");     // 建構
        ManagedString b(a);           // 拷貝建構（深拷貝）
        ManagedString c("World");     // 建構
        c = a;                        // 拷貝賦值（Copy-and-Swap）

        cout << "\n  三個物件的狀態：" << endl;
        cout << "  a: "; a.print();
        cout << "  b: "; b.print();
        cout << "  c: "; c.print();
        cout << "  （三個物件有各自獨立的記憶體）" << endl;
    }
    cout << "  （三個物件安全解構，無 double free）" << endl;

    cout << "\n--- 三法則速查 ---" << endl;
    cout << "  需要自訂解構函數？ → 也需要自訂拷貝建構 + 拷貝賦值" << endl;
    cout << "  判斷依據：類別是否管理動態資源（new/malloc/file handle...）" << endl;
    cout << "  如果只用 string, vector 等 RAII 容器 → 不需要三法則" << endl;
}

} // namespace Lesson30

// ============================================================
// main
// ============================================================
int main() {
    Lesson27::demo();
    Lesson28::demo();
    Lesson29::demo();
    Lesson30::demo();

    cout << "\n========================================" << endl;
    cout << "第 27～30 課總結完畢（拷貝語義）" << endl;
    cout << "下一步：summary27-35-移動語義.cpp" << endl;
    cout << "========================================" << endl;

    return 0;
}
