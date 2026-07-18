#include <iostream>
#include <string>
#include <utility>

// ===== 危險版本 =====
std::string&& bad_function() {
    std::string local = "danger";
    std::cout << "  函式內 local 的地址: " << &local << "\n";
    return std::move(local);
    // local 在這之後被解構
}

// ===== 安全版本 =====
std::string good_function() {   // 回傳值，不是參考
    std::string local = "safe";
    std::cout << "  函式內 local 的地址: " << &local << "\n";
    return local;  // 編譯器會套用 RVO 或移動建構
}

int main() {
    std::cout << "=== bad_function ===\n";
    // 以下是未定義行為，任何結果都可能出現
    std::string&& ref = bad_function();
    std::cout << "  回傳的參考指向地址: " << &ref << "\n";
    std::cout << "  嘗試讀取: \"" << ref << "\"\n";  // 可能是亂碼、空字串、或剛好正確
    // ↑ 這就像打開一扇門，門後的房間已經被拆除了

    std::cout << "\n=== good_function ===\n";
    std::string value = good_function();
    std::cout << "  value 的地址: " << &value << "\n";
    std::cout << "  讀取: \"" << value << "\"\n";  // 永遠安全

    return 0;
}
