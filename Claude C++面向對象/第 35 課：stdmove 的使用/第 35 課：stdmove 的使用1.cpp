// lesson35_move_usage.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson35 lesson35_move_usage.cpp

#include <iostream>
#include <string>
#include <vector>
#include <utility>

// 追蹤用的字串類別
class TrackedString {
private:
    std::string m_data;
    static int s_copyCount;
    static int s_moveCount;

public:
    TrackedString(const char* s = "") : m_data(s) {
        std::cout << "    [建構] \"" << m_data << "\"\n";
    }

    TrackedString(const TrackedString& other) : m_data(other.m_data) {
        ++s_copyCount;
        std::cout << "    [拷貝] \"" << m_data << "\" (累計 "
                  << s_copyCount << " 次拷貝)\n";
    }

    TrackedString(TrackedString&& other) noexcept : m_data(std::move(other.m_data)) {
        ++s_moveCount;
        std::cout << "    [移動] \"" << m_data << "\" (累計 "
                  << s_moveCount << " 次移動)\n";
    }

    TrackedString& operator=(const TrackedString& other) {
        m_data = other.m_data;
        ++s_copyCount;
        std::cout << "    [拷貝賦值] \"" << m_data << "\"\n";
        return *this;
    }

    TrackedString& operator=(TrackedString&& other) noexcept {
        m_data = std::move(other.m_data);
        ++s_moveCount;
        std::cout << "    [移動賦值] \"" << m_data << "\"\n";
        return *this;
    }

    const std::string& str() const { return m_data; }

    static void resetCounters() { s_copyCount = 0; s_moveCount = 0; }
    static void printCounters() {
        std::cout << "    >>> 總計：拷貝 " << s_copyCount
                  << " 次，移動 " << s_moveCount << " 次\n";
    }
};

int TrackedString::s_copyCount = 0;
int TrackedString::s_moveCount = 0;

// 接收按值參數，move 進成員
class Hero {
    TrackedString m_name;
    TrackedString m_title;
public:
    Hero(TrackedString name, TrackedString title)
        : m_name(std::move(name))
        , m_title(std::move(title))
    {}
};

int main() {
    // ===== 場景 1：放棄所有權 =====
    std::cout << "===== 場景 1：放棄所有權 =====\n";
    TrackedString::resetCounters();
    {
        TrackedString a("Excalibur");
        std::cout << "  不用 move：\n";
        TrackedString b = a;                // 拷貝
        std::cout << "  用 move：\n";
        TrackedString c = std::move(a);     // 移動
    }
    TrackedString::printCounters();
    std::cout << "\n";

    // ===== 場景 2：建構函數中 move 進成員 =====
    std::cout << "===== 場景 2：建構函數中 move 進成員 =====\n";

    std::cout << "  --- 傳入左值 ---\n";
    TrackedString::resetCounters();
    {
        TrackedString name("Arthur");
        TrackedString title("King");
        Hero h1(name, title);   // 按值傳入：拷貝建構 name, title → move 進成員
    }
    TrackedString::printCounters();

    std::cout << "\n  --- 傳入右值 ---\n";
    TrackedString::resetCounters();
    {
        TrackedString name("Arthur");
        TrackedString title("King");
        Hero h2(std::move(name), std::move(title));  // 移動建構 → move 進成員
    }
    TrackedString::printCounters();
    std::cout << "\n";

    // ===== 場景 3：push_back 到 vector =====
    std::cout << "===== 場景 3：push_back 到 vector =====\n";
    TrackedString::resetCounters();
    {
        std::vector<TrackedString> vec;
        vec.reserve(3);   // 預留空間，避免擴容干擾

        TrackedString s1("Alpha");
        TrackedString s2("Beta");
        TrackedString s3("Gamma");

        std::cout << "  push_back 拷貝：\n";
        vec.push_back(s1);                // 拷貝

        std::cout << "  push_back 移動：\n";
        vec.push_back(std::move(s2));     // 移動

        std::cout << "  emplace_back：\n";
        vec.emplace_back("Delta");        // 直接建構（零拷貝零移動）
    }
    TrackedString::printCounters();
    std::cout << "\n";

    // ===== 場景 4：從容器移出元素 =====
    std::cout << "===== 場景 4：從容器移出元素 =====\n";
    TrackedString::resetCounters();
    {
        std::vector<TrackedString> vec;
        vec.reserve(3);
        vec.emplace_back("First");
        vec.emplace_back("Second");
        vec.emplace_back("Third");

        std::cout << "  拷貝取出最後一個：\n";
        TrackedString a = vec.back();               // 拷貝

        std::cout << "  移動取出最後一個：\n";
        TrackedString b = std::move(vec.back());    // 移動
        vec.pop_back();
    }
    TrackedString::printCounters();
    std::cout << "\n";

    // ===== 場景 5：swap =====
    std::cout << "===== 場景 5：swap（3 次移動，0 次拷貝）=====\n";
    TrackedString::resetCounters();
    {
        TrackedString x("Left");
        TrackedString y("Right");
        std::cout << "  swap 前：x=\"" << x.str() << "\"  y=\"" << y.str() << "\"\n";
        std::swap(x, y);
        std::cout << "  swap 後：x=\"" << x.str() << "\"  y=\"" << y.str() << "\"\n";
    }
    TrackedString::printCounters();

    return 0;
}
