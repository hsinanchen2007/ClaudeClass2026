#include <iostream>
#include <string>

class Verbose {
public:
    Verbose(const std::string& n) : name_(n) {
        std::cout << "  [建構] " << name_ << "\n";
    }
    ~Verbose() {
        std::cout << "  [解構] " << name_ << "\n";
    }
    const std::string& name() const { return name_; }
private:
    std::string name_;
};

Verbose make_object() {
    return Verbose("臨時物件");
}

int main() {
    std::cout << "--- 情境 1：沒有綁定，臨時物件立即銷毀 ---\n";
    make_object();  // 臨時物件在這行結束就銷毀
    std::cout << "  （繼續執行）\n\n";

    std::cout << "--- 情境 2：const T& 延長生命週期 ---\n";
    const Verbose& ref = make_object();
    std::cout << "  ref 仍然有效: " << ref.name() << "\n";
    std::cout << "  （ref 離開作用域前）\n\n";

    std::cout << "--- 情境 3：T&& 延長生命週期 ---\n";
    Verbose&& rref = make_object();
    std::cout << "  rref 仍然有效: " << rref.name() << "\n";
    rref = Verbose("新物件");  // 還可以修改！（const T& 做不到）
    std::cout << "  rref 修改後: " << rref.name() << "\n";
    std::cout << "  （rref 離開作用域前）\n";

    std::cout << "\n--- 離開 main ---\n";
    return 0;
}
