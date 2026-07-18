#include <iostream>
#include <string>
using namespace std;

class SafeDistance {
private:
    double meters;

public:
    // explicit：禁止隱式轉換
    // 這個建構函數接受一個 double 類型的參數，並將其存儲在 meters 成員變量中。
    // 由於這個建構函數被標記為 explicit，C++ 不允許使用 double 類型的值來隱式創建 SafeDistance 對象。
    // 例如，當我們寫 SafeDistance d = 100.0; 時，編譯器會報錯，因為 explicit 禁止了這種隱式轉換。
    // 這裡的 explicit 可以防止意外的隱式轉換，確保我們在創建 SafeDistance 對象時必須明確地使用建構函數，這有助於提高代碼的安全性和可讀性。
    explicit SafeDistance(double m) : meters(m) {
        cout << "  建構 SafeDistance: " << meters << " m" << endl;
    }
    
    void print() const {
        cout << "  距離: " << meters << " 公尺" << endl;
    }
};

void showDistance(SafeDistance d) {
    d.print();
}

int main() {
    // OK：直接初始化
    SafeDistance d1(100.0);
    d1.print();
    
    // 編譯錯誤！explicit 禁止隱式轉換
    // SafeDistance d2 = 50.0;       // 錯誤！
    // showDistance(200.0);           // 錯誤！
    
    // 必須明確使用建構函數
    SafeDistance d2(50.0);            // OK
    showDistance(SafeDistance(200.0)); // OK：明確轉換
    
    return 0;
}
