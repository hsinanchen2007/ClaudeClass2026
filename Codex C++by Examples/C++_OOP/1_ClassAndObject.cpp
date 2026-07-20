// ============================================================================
// 課題 1：Class（類別）與 Object（物件）
// ============================================================================
//
// class 是自訂型別的藍圖，描述「每個物件保存什麼狀態」以及「可做哪些操作」；
// object/instance 是依該藍圖建立、在記憶體中真正存在的值。兩個 Student 物件使用
// 同一份 member function 程式碼，但各自有 name/score。
//
// OOP 的第一個價值不是「所有東西都包 class」，而是把有固定關係的 state 與
// behavior 放在同一個型別，讓誰負責維持資料正確變清楚。單純資料載體可用 struct；
// class/struct 能力幾乎相同，主要語法差異是 class 預設 private、struct 預設 public。
//
// 每個非 static member function 都有隱含 this pointer，`student.print()` 讓 this
// 指向 student，因此 function 內 `name` 等價於 `this->name`。
//
// 【面試】class 與 object 差異？class 是型別描述；object 是該型別的一個值。
// 【陷阱】未初始化 fundamental member（如 int total;）後讀取是 undefined behavior。
// 【設計】若資料有 invariant，下一課應把 member 設 private，不讓外界任意破壞。
// ============================================================================

#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Student {
public:
    Student(std::string student_name, int student_score)
        : name(std::move(student_name)), score(student_score) {}

    std::string description() const
    {
        return name + ":" + std::to_string(score);
    }

    std::string name;
    int score;
};

void basic_example()
{
    Student ada("Ada", 95);
    Student bjarne("Bjarne", 90);
    assert(ada.description() == "Ada:95");
    assert(bjarne.description() == "Bjarne:90");
    ada.score = 96;
    assert(bjarne.score == 90);  // 每個 object 有自己的 state。
    std::cout << "[基礎] " << ada.description() << '\n';
}

// LeetCode 1480：把「累積狀態 + 加入數字」包成物件。
class RunningSum {
public:
    int add(int value)
    {
        total_ += value;
        return total_;
    }

private:
    int total_ = 0;
};

void leetcode_1480_example()
{
    RunningSum sum;
    std::vector<int> output;
    for (const int value : {1, 2, 3, 4}) {
        output.push_back(sum.add(value));
    }
    assert((output == std::vector<int>{1, 3, 6, 10}));
    std::cout << "[LeetCode 1480] running sum=1,3,6,10\n";
}

// 工作案例：Sensor 同時保存 id 與校正值，讀值操作永遠套同一校正。
class Sensor {
public:
    Sensor(int sensor_id, double offset) : id_(sensor_id), offset_(offset) {}
    int id() const { return id_; }
    double calibrate(double raw) const { return raw + offset_; }

private:
    int id_;
    double offset_;
};

void practical_example()
{
    const Sensor cpu_sensor(7, -1.5);
    assert(cpu_sensor.id() == 7);
    assert(cpu_sensor.calibrate(80.0) == 78.5);
    std::cout << "[實務] sensor 7 校正後=78.5C\n";
}

int main()
{
    basic_example();
    leetcode_1480_example();
    practical_example();
}

// 實務提醒：class 應把資料與維持資料正確的操作放在一起，而不是只把 struct 換關鍵字。
// 練習：把 Student 的 score 改 private，新增只接受 0..100 的 set_score。
// 複雜度：簡單 field access O(1)，但 member function 的成本由實作決定，class 不代表自動變慢。
// 生命週期：每個 object 各自擁有 data members；reference/pointer 只在該 object 存活時有效。
