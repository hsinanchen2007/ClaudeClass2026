#include <iostream>
#include <functional>

int main() {
    std::cout << "=== 邏輯類函數物件 ===" << std::endl;
    
    // logical_and<T>：邏輯 AND
    std::logical_and<bool> land;
    std::cout << "logical_and: true && false = " << (land(true, false) ? "true" : "false") << std::endl;
    std::cout << "logical_and: true && true = " << (land(true, true) ? "true" : "false") << std::endl;
    
    // logical_or<T>：邏輯 OR
    std::logical_or<bool> lor;
    std::cout << "logical_or: true || false = " << (lor(true, false) ? "true" : "false") << std::endl;
    std::cout << "logical_or: false || false = " << (lor(false, false) ? "true" : "false") << std::endl;
    
    // logical_not<T>：邏輯 NOT
    std::logical_not<bool> lnot;
    std::cout << "logical_not: !true = " << (lnot(true) ? "true" : "false") << std::endl;
    std::cout << "logical_not: !false = " << (lnot(false) ? "true" : "false") << std::endl;
    
    return 0;
}
