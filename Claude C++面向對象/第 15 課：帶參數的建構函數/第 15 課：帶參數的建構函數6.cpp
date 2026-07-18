#include <iostream>
#include <string>
using namespace std;

class Solution3 {
private:
    string name;
    int age;

public:
    // 初始化列表中，括號外是成員變數，括號內是參數
    // 即使同名也不會衝突！
    // 這種風格的優點是參數名稱和成員變數名稱可以相同，代碼簡潔，且 this 指針明確指出正在訪問的是成員變數。
    // 但是缺點是如果不習慣使用 this 指針，可能會導致代碼可讀性降低，尤其是對於初學者來說。
    Solution3(const string& name, int age) 
        : name(name), age(age) { }
    
    void print() const {
        cout << "name = " << name << ", age = " << age << endl;
    }
};

int main() {
    Solution3 s("王五", 25);
    s.print();  // name = 王五, age = 25
    return 0;
}