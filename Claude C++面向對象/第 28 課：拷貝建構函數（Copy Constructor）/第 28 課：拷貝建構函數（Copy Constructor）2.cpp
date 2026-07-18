// lesson28_elision.cpp
// 比較有無拷貝省略的效果
// 編譯（允許優化）：g++ -std=c++17 -Wall -o elision lesson28_elision.cpp
// 編譯（禁用省略）：g++ -std=c++17 -Wall -fno-elide-constructors -o no_elision lesson28_elision.cpp

#include <iostream>
#include <cstring>

class Tracer {
private:
    char* m_data;

public:
    Tracer(const char* str) {
        m_data = new char[std::strlen(str) + 1];
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    Tracer(const Tracer& other) {
        m_data = new char[std::strlen(other.m_data) + 1];
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    ~Tracer() {
        std::cout << "  [解構] \"" << m_data << "\"\n";
        delete[] m_data;
    }

    const char* c_str() const { return m_data; }
};

Tracer makeTracer() {
    Tracer t("from function");
    return t;   // NRVO 可能省略拷貝
}

int main() {
    std::cout << "--- 測試 RVO / NRVO ---\n";
    Tracer result = makeTracer();
    std::cout << "result = \"" << result.c_str() << "\"\n";
    std::cout << "--- 結束 ---\n";
    return 0;
}
