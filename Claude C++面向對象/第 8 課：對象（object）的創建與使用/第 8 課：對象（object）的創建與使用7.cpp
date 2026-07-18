#include <iostream>
#include <string>
using namespace std;

class Student {
public:
    string name = "未命名";
    int score = 0;

    void show() {
        cout << name << ": " << score << " 分" << endl;
    }
};

int main() {
    // 建立包含 3 個 Student 對象的陣列
    Student students[3];

    // 分別設定
    students[0].name = "小明";
    students[0].score = 85;

    students[1].name = "小華";
    students[1].score = 92;

    students[2].name = "小美";
    students[2].score = 78;

    // 用迴圈遍歷
    cout << "===== 成績單 =====" << endl;
    for (int i = 0; i < 3; i++) {
        students[i].show();
    }

    // 找最高分
    int maxIdx = 0;
    for (int i = 1; i < 3; i++) {
        if (students[i].score > students[maxIdx].score) {
            maxIdx = i;
        }
    }
    cout << "\n最高分: " << students[maxIdx].name
         << " (" << students[maxIdx].score << " 分)" << endl;

    return 0;
}
