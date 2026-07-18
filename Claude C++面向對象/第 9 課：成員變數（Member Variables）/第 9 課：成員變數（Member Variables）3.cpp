#include <iostream>
#include <string>
using namespace std;

class Engine {
public:
    int horsepower = 0;
    string fuelType = "汽油";

    void start() {
        cout << horsepower << " 匹馬力的" << fuelType << "引擎啟動！" << endl;
    }
};

class Car {
public:
    string brand;
    Engine engine;  // Car「擁有」一個 Engine —— 這就是組合（composition）

    void drive() {
        cout << brand << " 開始行駛" << endl;
        engine.start();
    }
};

int main() {
    Car car;
    car.brand = "Toyota";
    car.engine.horsepower = 150;    // 存取內層對象的成員
    car.engine.fuelType = "油電混合";

    car.drive();

    return 0;
}
