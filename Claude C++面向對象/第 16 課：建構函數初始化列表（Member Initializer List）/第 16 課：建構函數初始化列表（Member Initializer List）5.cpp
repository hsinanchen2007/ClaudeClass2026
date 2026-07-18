#include <iostream>
#include <string>
using namespace std;

class Animal {
private:
    string species;

public:
    Animal(const string& s) : species(s) {
        cout << "  Animal 建構: " << species << endl;
    }
    
    string getSpecies() const { return species; }
};

class Dog : public Animal {
private:
    string name;

public:
    // 用初始化列表調用基類建構函數
    // Dog 的建構函數需要同時初始化基類 Animal 和自己的成員 name
    // 初始化列表的語法：冒號後面跟著成員變數和對應的初始值
    // 注意：初始化列表的語法是冒號後面跟著成員變數和對應的初始值
    // 優點：效率更高，特別是對於基類成員來說，必須使用初始化列表來初始化，因為它們不能在函數體內賦值
    Dog(const string& n) 
        : Animal("犬科"),   // 調用基類建構函數
          name(n)            // 初始化自己的成員
    {
        cout << "  Dog 建構: " << name << endl;
    }
    
    void print() const {
        cout << "  " << name << " (" << getSpecies() << ")" << endl;
    }
};

int main() {
    Dog d("旺財");
    d.print();
    return 0;
}
