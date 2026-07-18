#include <iostream>
#include <string>
using namespace std;

class TemperatureConverter {
public:
    double celsius = 0.0;

    // 基礎轉換函數
    double toFahrenheit() {
        return celsius * 9.0 / 5.0 + 32.0;
    }

    double toKelvin() {
        return celsius + 273.15;
    }

    // 判斷函數 —— 調用其他成員函數
    bool isBoiling() {
        return celsius >= 100.0;
    }

    bool isFreezing() {
        return celsius <= 0.0;
    }

    string getState() {
        if (isFreezing()) return "固態（冰）";    // 調用自己的成員函數
        if (isBoiling()) return "氣態（水蒸氣）";
        return "液態（水）";
    }

    // 綜合報告 —— 調用多個成員函數
    void report() {
        cout << "=========================" << endl;
        cout << "攝氏:   " << celsius << " °C" << endl;
        cout << "華氏:   " << toFahrenheit() << " °F" << endl;
        cout << "克式:   " << toKelvin() << " K" << endl;
        cout << "水的狀態: " << getState() << endl;
        cout << "=========================" << endl;
    }
};

int main() {
    TemperatureConverter t;

    t.celsius = -10;
    t.report();

    cout << endl;
    t.celsius = 25;
    t.report();

    cout << endl;
    t.celsius = 100;
    t.report();

    return 0;
}
