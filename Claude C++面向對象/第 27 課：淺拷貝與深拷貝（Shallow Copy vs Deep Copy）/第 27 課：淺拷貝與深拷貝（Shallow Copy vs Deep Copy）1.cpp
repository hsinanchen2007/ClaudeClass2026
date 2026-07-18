// lesson27_shallow_problem.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o shallow lesson27_shallow_problem.cpp
// 執行：./shallow（可能會崩潰！）

#include <iostream>
#include <cstring>   // std::strlen, std::strcpy

class SimpleString {
private:
    char* m_data;       // 指向堆積上的 C 風格字串
    std::size_t m_len;  // 字串長度（不含 '\0'）

public:
    // 建構函數：配置記憶體並複製字串
    SimpleString(const char* str = "") {
        m_len = std::strlen(str);
        m_data = new char[m_len + 1];  // +1 for '\0'
        std::strcpy(m_data, str);
        std::cout << "  [建構] 配置記憶體 @ " << static_cast<void*>(m_data)
                  << " 內容=\"" << m_data << "\"\n";
    }

    // 解構函數：釋放記憶體
    ~SimpleString() {
        std::cout << "  [解構] 釋放記憶體 @ " << static_cast<void*>(m_data)
                  << " 內容=\"" << (m_data ? m_data : "nullptr") << "\"\n";
        delete[] m_data;
    }

    // ⚠️ 沒有自定義拷貝建構函數 → 編譯器使用淺拷貝
    // ⚠️ 沒有自定義拷貝賦值運算子 → 編譯器使用淺拷貝

    const char* c_str() const { return m_data; }

    void setFirstChar(char ch) {
        if (m_len > 0) {
            m_data[0] = ch;
        }
    }
};

int main() {
    std::cout << "=== 淺拷貝問題示範 ===\n\n";

    std::cout << "步驟 1：建立 original\n";
    SimpleString original("Hello");

    std::cout << "\n步驟 2：用 original 拷貝建構 copy\n";
    SimpleString copy = original;   // 呼叫編譯器自動生成的拷貝建構函數

    std::cout << "\n步驟 3：檢查兩者的指標位址\n";
    std::cout << "  original.m_data 指向: "
              << static_cast<const void*>(original.c_str()) << "\n";
    std::cout << "  copy.m_data     指向: "
              << static_cast<const void*>(copy.c_str()) << "\n";
    std::cout << "  （兩者位址相同！共享同一塊記憶體）\n";

    std::cout << "\n步驟 4：透過 copy 修改第一個字元\n";
    copy.setFirstChar('X');
    std::cout << "  original = \"" << original.c_str() << "\"\n";
    std::cout << "  copy     = \"" << copy.c_str() << "\"\n";
    std::cout << "  （original 也被改了！因為共享記憶體）\n";

    std::cout << "\n步驟 5：離開作用域，開始解構\n";
    // copy 先解構 → delete[] m_data → 記憶體被釋放
    // original 再解構 → delete[] m_data → 同一塊記憶體被釋放第二次！
    // ⚠️ Double Free → Undefined Behavior!

    return 0;
}
