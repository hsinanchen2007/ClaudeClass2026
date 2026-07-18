// ============================================================
// 第 28 課 總結：拷貝建構函數（Copy Constructor）
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【簽名】ClassName(const ClassName& other)
//   參數必須是 const 引用：
//     - 如果是傳值 → 傳值本身會觸發拷貝建構 → 無限遞迴！
//     - const 讓函數能接受 const 物件和臨時物件
//
// 【六大觸發場景】
//   1. 直接初始化：      MyClass b(a);
//   2. 拷貝初始化：      MyClass b = a;
//   3. 函數傳值參數：    void func(MyClass obj) ← 傳入時拷貝
//   4. 函數回傳值：      return obj; ← 回傳時拷貝（可能被優化省略）
//   5. 陣列初始化：      MyClass arr[] = {a, b};
//   6. 容器操作：        vector.push_back(obj);
//
// 【Copy Elision / RVO / NRVO】
//   RVO（Return Value Optimization）：回傳臨時物件時省略拷貝
//     return MyClass("hi");  → 直接在呼叫者的空間建構
//   NRVO（Named RVO）：回傳具名物件時省略拷貝
//     MyClass obj("hi"); return obj;
//   C++17 起 RVO 是強制的（mandatory），NRVO 仍是可選優化
//   編譯時加 -fno-elide-constructors 可觀察不省略的行為
//
// 【含指標的類別必須深拷貝】
//   用 std::copy 或 memcpy 複製陣列資料到新配置的記憶體
// ============================================================

#include <iostream>
#include <cstring>
#include <algorithm>  // std::copy

// ============================================================
// 追蹤拷貝行為的類別
// ============================================================
class SimpleString {
    char* m_data;
    std::size_t m_len;
public:
    // 一般建構
    SimpleString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, str);
        std::cout << "  [一般建構] \"" << m_data << "\"\n";
    }

    // ★ 拷貝建構函數（深拷貝）
    SimpleString(const SimpleString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data
                  << "\" (新 @" << static_cast<void*>(m_data)
                  << " 源 @" << static_cast<const void*>(other.m_data) << ")\n";
    }

    // 拷貝賦值
    SimpleString& operator=(const SimpleString& other) {
        if (this == &other) return *this;
        delete[] m_data;
        m_len = other.m_len;
        m_data = new char[m_len + 1];
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝賦值] \"" << m_data << "\"\n";
        return *this;
    }

    // 解構
    ~SimpleString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr") << "\"\n";
        delete[] m_data;
    }

    const char* c_str() const { return m_data; }
};

// 場景 3：按值傳參 → 觸發拷貝建構
void passByValue(SimpleString s) {
    std::cout << "    函數內：s = \"" << s.c_str() << "\"\n";
} // s 離開作用域被解構

// 場景 4：按 const 引用傳參 → 不觸發拷貝！
void passByConstRef(const SimpleString& s) {
    std::cout << "    函數內：s = \"" << s.c_str() << "\"\n";
}

// 場景 4（續）：按值回傳 → 可能觸發拷貝（通常被 NRVO 省略）
SimpleString createString() {
    SimpleString local("Created Inside");
    return local;  // NRVO 可能省略拷貝
}

// ============================================================
// 含指標陣列的深拷貝示範
// ============================================================
class IntArray {
    int* m_data;
    std::size_t m_size;
public:
    explicit IntArray(std::size_t size)
        : m_size(size), m_data(new int[size]{}) {
        std::cout << "  [IntArray 建構] size=" << m_size << "\n";
    }

    // 深拷貝建構：用 std::copy 複製陣列
    IntArray(const IntArray& other)
        : m_size(other.m_size), m_data(new int[other.m_size]) {
        std::copy(other.m_data, other.m_data + m_size, m_data);
        std::cout << "  [IntArray 拷貝建構] size=" << m_size << "\n";
    }

    // 拷貝賦值
    IntArray& operator=(const IntArray& other) {
        if (this == &other) return *this;
        delete[] m_data;
        m_size = other.m_size;
        m_data = new int[m_size];
        std::copy(other.m_data, other.m_data + m_size, m_data);
        return *this;
    }

    ~IntArray() { delete[] m_data; }

    int& operator[](std::size_t i) { return m_data[i]; }
    const int& operator[](std::size_t i) const { return m_data[i]; }
    std::size_t size() const { return m_size; }

    void print(const char* label) const {
        std::cout << "  " << label << ": [";
        for (std::size_t i = 0; i < m_size; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << m_data[i];
        }
        std::cout << "]\n";
    }
};

int main() {
    // ============================================================
    // 場景 1 & 2：直接初始化 / 拷貝初始化
    // ============================================================
    std::cout << "===== 場景 1 & 2：直接初始化 / 拷貝初始化 =====\n";
    SimpleString a("Alpha");
    SimpleString b(a);       // 場景 1：直接初始化 → 拷貝建構
    SimpleString c = a;      // 場景 2：拷貝初始化 → 也是拷貝建構（不是賦值！）
    std::cout << "\n";

    // ============================================================
    // 場景 3：按值傳參
    // ============================================================
    std::cout << "===== 場景 3：按值傳參（觸發拷貝）=====\n";
    passByValue(a);  // 拷貝 a → 函數內的 s → 函數結束時 s 被解構
    std::cout << "\n";

    // 對比：按 const 引用 → 不拷貝
    std::cout << "===== 對比：按 const 引用（不拷貝）=====\n";
    passByConstRef(a);
    std::cout << "\n";

    // ============================================================
    // 場景 4：按值回傳（NRVO 可能省略拷貝）
    // ============================================================
    std::cout << "===== 場景 4：按值回傳（NRVO）=====\n";
    SimpleString d = createString();
    std::cout << "  d = \"" << d.c_str() << "\"\n";
    std::cout << "  （如果只看到一次建構 → NRVO 省略了拷貝）\n\n";

    // ============================================================
    // 拷貝建構 vs 拷貝賦值的區別
    // ============================================================
    std::cout << "===== 拷貝建構 vs 拷貝賦值 =====\n";
    SimpleString e("Echo");
    std::cout << "  e = a（e 已存在 → 拷貝賦值，不是拷貝建構）:\n";
    e = a;  // 拷貝「賦值」— 因為 e 已經存在
    std::cout << "\n";

    // ============================================================
    // IntArray 深拷貝示範
    // ============================================================
    std::cout << "===== IntArray 深拷貝 =====\n";
    IntArray arr1(5);
    for (std::size_t i = 0; i < arr1.size(); ++i)
        arr1[i] = static_cast<int>((i + 1) * 10);
    arr1.print("arr1");

    IntArray arr2 = arr1;  // 深拷貝建構
    arr2[0] = 999;         // 修改 arr2
    arr1.print("arr1");    // arr1 不受影響
    arr2.print("arr2");    // 只有 arr2 改變
    std::cout << "\n";

    // ============================================================
    // Copy Elision 說明
    // ============================================================
    std::cout << "===== Copy Elision / RVO / NRVO =====\n";
    std::cout << "  RVO：return MyClass(\"x\");  直接在呼叫者空間建構\n";
    std::cout << "  NRVO：MyClass m(\"x\"); return m;  省略具名物件拷貝\n";
    std::cout << "  C++17 起 RVO 強制生效，NRVO 仍是可選優化\n";
    std::cout << "  加 -fno-elide-constructors 可觀察原本會發生的拷貝\n";

    std::cout << "\n===== 開始解構 =====\n";
    return 0;
}
