#include <iostream>
#include <string>

template <typename T>
class Box {
private:
    T content;
    bool has_content;
    
public:
    Box() : has_content(false) {}
    
    void put(const T& item) {
        content = item;
        has_content = true;
    }
    
    T get() const {
        if (!has_content) {
            throw std::runtime_error("Box is empty!");
        }
        return content;
    }
    
    bool is_empty() const {
        return !has_content;
    }
};

int main() {
    // 裝 int 的盒子
    Box<int> int_box;
    int_box.put(42);
    std::cout << "int_box contains: " << int_box.get() << std::endl;
    
    // 裝 string 的盒子
    Box<std::string> string_box;
    string_box.put("Hello, STL!");
    std::cout << "string_box contains: " << string_box.get() << std::endl;
    
    // 裝 double 的盒子
    Box<double> double_box;
    double_box.put(3.14159);
    std::cout << "double_box contains: " << double_box.get() << std::endl;
    
    return 0;
}
