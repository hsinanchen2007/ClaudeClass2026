#include <iostream>
#include <string>
using namespace std;

// ===== 反面教材：忘記加 const =====
class BadDesign {
private:
    string name_;
    int value_;

public:
    BadDesign(const string& n, int v) : name_(n), value_(v) {}

    // 忘記加 const！這些函數明明不修改對象
    // 這會導致無法在 const 對象上調用這些函數，限制了它們的使用場景
    // 這裡的 getName()、getValue() 和 print() 函數都應該是 const 成員函數，但它們缺少 const 修飾符
    string getName() { return name_; }       // 缺少 const
    int getValue() { return value_; }        // 缺少 const
    void print() { cout << name_ << ":" << value_ << endl; } // 缺少 const
};

// ===== 正確設計：const 正確 =====
class GoodDesign {
private:
    string name_;
    int value_;

public:
    GoodDesign(const string& n, int v) : name_(n), value_(v) {}

    // 所有不修改對象的函數都加 const
    // 這允許在 const 對象上調用這些函數，並且編譯器會強制執行這一承諾
    // 這裡的 getName()、getValue() 和 print() 函數都正確地標記為 const 成員函數，表示它們不修改對象的狀態
    const string& getName() const { return name_; }
    int getValue() const { return value_; }
    void print() const { cout << name_ << ":" << value_ << endl; }

    // 只有修改對象的函數才不加 const
    // 這裡的 setValue() 函數會修改 value_，所以不能是 const
    void setValue(int v) { value_ = v; }
};

// 需要 const 引用的函數
// 這個函數接受一個 const GoodDesign&，表示它只能「看」這個對象，但不能修改它
// 這裡的 processGood() 函數接受一個 const GoodDesign&，表示它只能「看」這個對象，但不能修改它
// 這裡的 processBad() 函數接受一個 const BadDesign&，但由於 BadDesign 的成員函數沒有標記為 const，所以幾乎什麼都做不了
void processBad(const BadDesign& b) {
    // b.getName();   // ❌ 編譯錯誤！getName 不是 const
    // b.print();     // ❌ 編譯錯誤！print 不是 const
    cout << "  BadDesign：什麼都不能做！" << endl;
}

void processGood(const GoodDesign& g) {
    g.print();         // ✅ 完美
    cout << "  name = " << g.getName() << endl;  // ✅
    cout << "  value = " << g.getValue() << endl; // ✅
}

int main() {
    cout << "=== const 正確性 ===" << endl;

    BadDesign bad("壞設計", 42);
    GoodDesign good("好設計", 42);

    cout << "\n--- 用 const 引用傳遞 ---" << endl;
    processBad(bad);      // 幾乎什麼都做不了
    processGood(good);    // 正常工作

    return 0;
}
