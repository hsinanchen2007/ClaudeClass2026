#include <iostream>
using namespace std;

class Sensor {
public:
    int id;
    double temperature;
    bool isActive;
    char grade;

    void show() {
        cout << "ID: " << id << endl;
        cout << "溫度: " << temperature << " °C" << endl;
        cout << "啟用: " << (isActive ? "是" : "否") << endl;
        cout << "等級: " << grade << endl;
    }
};

int main() {
    Sensor s;
    s.id = 1001;
    s.temperature = 36.5;
    s.isActive = true;
    s.grade = 'A';

    s.show();

    return 0;
}
