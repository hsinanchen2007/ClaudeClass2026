// ============================================================
// 第 2.3 章 總結：移動建構子（Move Constructor）— 實作與原理
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 注意：本課原始檔案為空，以下根據課程標題提供完整重點整理
// 移動建構子的詳細內容可參考面向對象課程第 32 課
//
// 【移動建構子簽名】
//   ClassName(ClassName&& other) noexcept
//
// 【實作三步驟】
//   1. 偷取（steal）：m_data = other.m_data;
//   2. 歸零（nullify）：other.m_data = nullptr;
//   3. 複製基本型別：m_size = other.m_size; other.m_size = 0;
//
// 【vs 拷貝建構子】
//   拷貝建構：new 記憶體 + 複製資料 → O(n)
//   移動建構：偷指標 + 歸零         → O(1)
//
// 【noexcept 的重要性】
//   vector 擴容時：
//     有 noexcept → 使用移動（快）
//     無 noexcept → 退回拷貝（慢，但安全）
//
// 【觸發條件】
//   傳入 rvalue 時觸發移動建構：
//     MyClass a = std::move(b);        // 明確移動
//     MyClass a = func();              // 回傳臨時物件（可能被 RVO 省略）
//     MyClass a = MyClass("temp");     // 從臨時物件
// ============================================================

#include <iostream>
#include <cstring>
#include <utility>
#include <vector>
using namespace std;

class MoveDemo {
    char* m_data;
    size_t m_size;
public:
    // 一般建構
    MoveDemo(const char* str = "")
        : m_size(strlen(str)), m_data(new char[m_size + 1]) {
        strcpy(m_data, str);
        cout << "  [建構] \"" << m_data << "\"\n";
    }

    // 拷貝建構（深拷貝）— O(n)
    MoveDemo(const MoveDemo& other)
        : m_size(other.m_size), m_data(new char[m_size + 1]) {
        strcpy(m_data, other.m_data);
        cout << "  [拷貝建構💰] \"" << m_data << "\" — new + copy\n";
    }

    // ★ 移動建構 — O(1)
    MoveDemo(MoveDemo&& other) noexcept
        : m_data(other.m_data)          // ① 偷取指標
        , m_size(other.m_size)          // ③ 複製基本型別
    {
        other.m_data = nullptr;         // ② 歸零源物件
        other.m_size = 0;
        cout << "  [移動建構⚡] \"" << m_data << "\" — 只搬指標\n";
    }

    // 解構
    ~MoveDemo() {
        if (m_data) cout << "  [解構] \"" << m_data << "\"\n";
        else        cout << "  [解構] (nullptr)\n";
        delete[] m_data;
    }

    const char* c_str() const { return m_data ? m_data : "(null)"; }
};

int main() {
    // ============================================================
    // 1. 拷貝 vs 移動
    // ============================================================
    cout << "===== 1. 拷貝 vs 移動 =====\n";
    {
        MoveDemo a("Hello World");

        cout << "\n  拷貝建構（深拷貝）：\n";
        MoveDemo b = a;           // 拷貝 — O(n)

        cout << "\n  移動建構（偷指標）：\n";
        MoveDemo c = move(a);     // 移動 — O(1)
        cout << "  a = \"" << a.c_str() << "\" (已被掏空)\n";
        cout << "  c = \"" << c.c_str() << "\"\n";
    }
    cout << "\n";

    // ============================================================
    // 2. 從臨時物件移動
    // ============================================================
    cout << "===== 2. 從臨時物件移動 =====\n";
    {
        MoveDemo d = MoveDemo("Temporary");
        cout << "  d = \"" << d.c_str() << "\"\n";
    }
    cout << "\n";

    // ============================================================
    // 3. noexcept 影響 vector 擴容
    // ============================================================
    cout << "===== 3. vector 擴容使用移動 =====\n";
    {
        vector<MoveDemo> vec;
        vec.reserve(2);

        cout << "  push #1:\n";
        vec.push_back(MoveDemo("Alpha"));

        cout << "  push #2:\n";
        vec.push_back(MoveDemo("Beta"));

        cout << "  push #3（觸發擴容）:\n";
        vec.push_back(MoveDemo("Gamma"));
        // 因為有 noexcept → 搬移 Alpha 和 Beta 用移動而非拷貝
    }

    cout << "\n=== 重點 ===\n";
    cout << "  移動建構三步驟：偷指標 → 歸零源 → 複製基本型別\n";
    cout << "  拷貝 O(n)，移動 O(1)\n";
    cout << "  noexcept 讓 vector 擴容使用移動\n";
    cout << "  移動後的源物件處於有效但未指定狀態\n";

    return 0;
}
