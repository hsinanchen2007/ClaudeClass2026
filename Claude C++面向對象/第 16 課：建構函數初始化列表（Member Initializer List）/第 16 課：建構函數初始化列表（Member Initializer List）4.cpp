#include <iostream>
#include <string>
using namespace std;

class Engine {
private:
    int horsepower;
    string fuelType;

public:
    // Engine 只有帶參建構函數，沒有預設建構函數
    Engine(int hp, const string& fuel) 
        : horsepower(hp), fuelType(fuel) 
    {
        cout << "  引擎建構: " << hp << " 馬力, " << fuel << endl;
    }
    
    void print() const {
        cout << "  引擎: " << horsepower << " HP (" << fuelType << ")" << endl;
    }
};

class Car {
private:
    string brand;
    Engine engine;   // Engine 沒有預設建構函數！

public:
    // 錯誤寫法：
    // Car(const string& b, int hp, const string& fuel) {
    //     brand = b;
    //     engine = Engine(hp, fuel);  // 編譯錯誤！engine 無法預設建構！
    // }
    
    // 正確寫法：在初始化列表中建構 engine
    // 初始化列表的語法：冒號後面跟著成員變數和對應的初始值
    // 注意：初始化列表的語法是冒號後面跟著成員變數和對應的初始值
    // 優點：效率更高，特別是對於沒有預設建構函數的成員來說，必須使用初始化列表來初始化，因為它們不能在函數體內賦值
    Car(const string& b, int hp, const string& fuel) 
        : brand(b), engine(hp, fuel)  // 直接把參數傳給 Engine 的建構函數
    {
        cout << "  汽車建構: " << brand << endl;
    }
    
    void print() const {
        cout << "  品牌: " << brand << endl;
        engine.print();
    }
};

int main() {
    Car car("BMW", 300, "汽油");
    cout << endl;
    car.print();
    
    return 0;
}
