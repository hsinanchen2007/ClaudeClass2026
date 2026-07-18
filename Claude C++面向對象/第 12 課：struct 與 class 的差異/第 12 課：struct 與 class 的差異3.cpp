#include <iostream>
#include <string>
using namespace std;

// ===== C 語言的 struct（在 C++ 中也能編譯）=====
// 只能有資料，不能有函數, 沒有存取控制，沒有繼承
// C 中必須寫 struct C_Style s; （C++ 中可省略 struct 關鍵字）
// 注意：C++ 中的 struct 和 class 的唯一区别是預設的存取控制不同：struct 預設 public，class 預設 private
struct C_Style {
    char name[50];
    int age;
    float score;
};
// C 中必須寫 struct C_Style s; （C++ 中可省略 struct 關鍵字）

// ===== C++ 的 struct =====
// 可以有函數、建構函數、存取控制、繼承……
// C++ 中 struct 和 class 的唯一区别是預設的存取控制不同：struct 預設 public，class 預設 private
// 下面是 C++ 風格的 struct，功能更完整
struct CPP_Style {
    string name;
    int age = 0;
    float score = 0.0f;

    void show() {
        cout << name << ", " << age << " 歲, " << score << " 分" << endl;
    }

    bool isPassing() {
        return score >= 60.0f;
    }
};

int main() {
    // C 風格
    C_Style cs;
    // strcpy(cs.name, "小明");  // 需要 cstring
    cs.age = 20;
    cs.score = 85.5f;

    // C++ 風格
    CPP_Style cpps;
    cpps.name = "小明";
    cpps.age = 20;
    cpps.score = 85.5f;
    cpps.show();
    cout << "及格: " << (cpps.isPassing() ? "是" : "否") << endl;

    return 0;
}
