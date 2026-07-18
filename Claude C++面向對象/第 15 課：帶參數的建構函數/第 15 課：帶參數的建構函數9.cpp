#include <iostream>
#include <string>
using namespace std;

class Distance {
private:
    double meters;

public:
    // 單參數建構函數 → 可以用於隱式轉換
    // 這個建構函數接受一個 double 類型的參數，並將其存儲在 meters 成員變量中。
    // 由於這是一個單參數建構函數，C++ 允許使用 double 類型的值來隱式創建 Distance 對象。
    // 例如，當我們寫 Distance d = 100.0; 時，編譯器會自動將 100.0 轉換為 Distance(100.0)，這就是隱式轉換的例子。
    // 這裡的隱式轉換可以讓我們更方便地使用 Distance 類，因為我們不需要顯式地創建 Distance 對象來表示距離。
    Distance(double m) : meters(m) {
        cout << "  建構 Distance: " << meters << " m" << endl;
    }
    
    double getMeters() const { return meters; }
    
    void print() const {
        cout << "  距離: " << meters << " 公尺" << endl;
    }
};

void showDistance(Distance d) {
    d.print();
}

int main() {
    cout << "=== 正常使用 ===" << endl;
    Distance d1(100.0);
    d1.print();
    
    cout << "\n=== 隱式轉換 ===" << endl;
    Distance d2 = 50.0;   // double 隱式轉換為 Distance！
    d2.print();
    
    cout << "\n=== 函數參數中的隱式轉換 ===" << endl;
    showDistance(200.0);   // double 被隱式轉換為 Distance！
    // 等同於 showDistance(Distance(200.0));
    
    return 0;
}
