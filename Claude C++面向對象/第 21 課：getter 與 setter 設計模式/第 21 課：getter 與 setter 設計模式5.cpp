#include <iostream>
#include <string>
using namespace std;

// ===== 風格 1：get/set 前綴（Java 風格）=====
class Style1 {
private:
    int hp_;
    string name_;
public:
    int getHp() const { return hp_; }
    void setHp(int hp) { hp_ = hp; }
    const string& getName() const { return name_; }
    void setName(const string& name) { name_ = name; }
};

// ===== 風格 2：無前綴，同名函數重載（C++ 風格）=====
class Style2 {
private:
    int hp_;
    string name_;
public:
    // getter：無參數
    int hp() const { return hp_; }
    const string& name() const { return name_; }

    // setter：有參數
    void hp(int newHp) { hp_ = newHp; }
    void name(const string& newName) { name_ = newName; }
};

// ===== 風格 3：STL 風格（getter 用名詞，沒有 setter）=====
// STL 容器的做法：vector::size(), string::length()
// 通常不提供 setter，只提供行為函數

int main() {
    cout << "=== 命名風格比較 ===" << endl;

    // 風格 1 的使用
    cout << "\n--- 風格 1：get/set 前綴 ---" << endl;
    Style1 s1;
    s1.setHp(100);
    s1.setName("風格一");
    cout << "  " << s1.getName() << " HP:" << s1.getHp() << endl;

    // 風格 2 的使用
    cout << "\n--- 風格 2：同名重載 ---" << endl;
    Style2 s2;
    s2.hp(200);             // setter
    s2.name("風格二");       // setter
    cout << "  " << s2.name() << " HP:" << s2.hp() << endl;  // getter

    cout << "\n兩種風格都可以，關鍵是在項目中保持一致。" << endl;

    return 0;
}
