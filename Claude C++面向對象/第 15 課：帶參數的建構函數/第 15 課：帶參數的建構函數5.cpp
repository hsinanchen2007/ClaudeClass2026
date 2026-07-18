#include <iostream>
#include <string>
using namespace std;

// 風格 A：參數加底線前綴
// 這種風格的優點是參數名稱和成員變數名稱完全區分開來，不會混淆。
// 但是缺點是參數名稱不夠直觀，可能需要額外的註釋來說明參數的含義。
// 風格 B：成員變數加 m_ 前綴（很多公司採用的風格）
// 這種風格的優點是成員變數名稱明確，容易區分成員變數和參數，且參數名稱可以直接使用有意義的名稱。
// 但是缺點是成員變數名稱較長，可能會增加代碼的冗長性。
// 風格 C：成員變數加底線後綴（Google C++ 風格指南）
// 這種風格的優點是成員變數名稱明確，容易區分成員變數和參數，且參數名稱可以直接使用有意義的名稱。
// 但是缺點是成員變數名稱較長，可能會增加代碼的冗長性。
// 風格 D：使用 this 指針（很多人習慣的風格）
// 這種風格的優點是參數名稱和成員變數名稱可以相同，代碼簡潔，且 this 指針明確指出正在訪問的是成員變數。
// 但是缺點是如果不習慣使用 this 指針，可能會導致代碼可讀性降低，尤其是對於初學者來說。
class Solution2A {
private:
    string name;
    int age;

public:
    Solution2A(const string& _name, int _age) {
        name = _name;
        age = _age;
    }
};

// 風格 B：成員變數加 m_ 前綴（很多公司採用的風格）
// 這種風格的優點是成員變數名稱明確，容易區分成員變數和參數，且參數名稱可以直接使用有意義的名稱。
// 但是缺點是成員變數名稱較長，可能會增加代碼的冗長性。
// 風格 C：成員變數加底線後綴（Google C++ 風格指南）
// 這種風格的優點是成員變數名稱明確，容易區分成員變數和參數，且參數名稱可以直接使用有意義的名稱。
// 但是缺點是成員變數名稱較長，可能會增加代碼的冗長性。
class Solution2B {
private:
    string m_name;
    int m_age;

public:
    Solution2B(const string& name, int age) {
        m_name = name;
        m_age = age;
    }
};

// 風格 C：成員變數加底線後綴（Google C++ 風格指南）
// 這種風格的優點是成員變數名稱明確，容易區分成員變數和參數，且參數名稱可以直接使用有意義的名稱。
// 但是缺點是成員變數名稱較長，可能會增加代碼的冗長性。
class Solution2C {
private:
    string name_;
    int age_;

public:
    Solution2C(const string& name, int age) {
        name_ = name;
        age_ = age;
    }
};

// 風格 D：使用 this 指針（很多人習慣的風格）
// 這種風格的優點是參數名稱和成員變數名稱可以相同，代碼簡潔，且 this 指針明確指出正在訪問的是成員變數。
// 但是缺點是如果不習慣使用 this 指針，可能會導致代碼可讀性降低，尤其是對於初學者來說。
class Solution2D {
private:
    string name;
    int age;    
public:
    Solution2D(const string& name, int age) {   
        this->name = name;   // this->name 是成員變數，name 是參數, 這裡是把參數 name 賦值給成員變數 name！
        this->age = age;     // 同理，this->age 是成員變數，age 是參數, 這裡是把參數 age 賦值給成員變數 age！
    }   
};

int main() {
    Solution2A s1("Alice", 25);
    Solution2B s2("Bob", 30);
    Solution2C s3("Charlie", 35);
    Solution2D s4("David", 40);
    return 0;
}

