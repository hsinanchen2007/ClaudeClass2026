#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Classroom {
public:
    string teacherName;
    string roomNumber;
    vector<string> students;  // 動態陣列作為成員

    void addStudent(const string& name) {
        students.push_back(name);
    }

    void show() {
        cout << "教室: " << roomNumber << endl;
        cout << "老師: " << teacherName << endl;
        cout << "學生 (" << students.size() << " 人):" << endl;
        for (int i = 0; i < students.size(); i++) {
            cout << "  " << (i + 1) << ". " << students[i] << endl;
        }
    }
};

int main() {
    Classroom room;
    room.teacherName = "王老師";
    room.roomNumber = "A-301";

    room.addStudent("小明");
    room.addStudent("小華");
    room.addStudent("小美");

    room.show();

    return 0;
}
