#include <iostream>
#include <string>
#include <cstring> // for strcpy

// C 語言風格
typedef struct {
    char name[50];
    int age;
    float gpa;
} Student;

// 必須手動調用！而且有可能忘記調用！
void student_init(Student* s, const char* name, int age, float gpa) {
    strcpy(s->name, name);
    s->age = age;
    s->gpa = gpa;
}

int main() {
    Student s;
    // 如果忘記調用 student_init()，s 的內容就是垃圾值
    // 編譯器不會警告你！
    student_init(&s, "張三", 20, 3.8);
    return 0;
}
