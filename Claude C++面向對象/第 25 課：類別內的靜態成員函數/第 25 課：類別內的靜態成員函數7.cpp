#include <iostream>
#include <string>
using namespace std;

// ===== 方式 1：全域函數 =====
// 全域函數沒有歸屬感，容易與其他函數名字衝突，且無法訪問類別的 private 成員
// 全域函數適合一些簡單的工具函數，但對於需要封裝和狀態管理的功能來說，並不是最佳選擇
// 全域函數的缺點：名字可能衝突、沒有歸屬感、不能訪問 private
// 全域函數示例
int globalAdd(int a, int b) { return a + b; }
// 問題：名字可能衝突、沒有歸屬感、不能訪問 private

// ===== 方式 2：命名空間函數 =====
// 命名空間函數有了命名空間的隔離，可以避免名字衝突，但仍然沒有歸屬感，且不能訪問類別的 private 成員
// 命名空間函數適合一些需要組織但不需要封裝的工具函數，但對於需要封裝和狀態管理的功能來說，仍然不是最佳選擇
// 命名空間函數示例
namespace MathUtils {
    int add(int a, int b) { return a + b; }
}
// 較好：有命名空間隔離，但不能訪問類別的 private

// ===== 方式 3：類別的靜態函數 =====
// 類別的靜態函數有了類別的歸屬感，可以訪問類別的 private 成員，並且可以維護內部狀態，是最適合封裝功能的方式
// 類別的靜態函數示例
class Calculator {
private:
    inline static int operationCount_ = 0;  // 可以有私有狀態

public:
    static int add(int a, int b) {
        operationCount_++;    // 可以維護內部狀態
        return a + b;
    }

    static int getOperationCount() { return operationCount_; }
};
// 最佳：有歸屬、可以訪問 private、可以維護狀態

int main() {
    cout << "=== 三種方式比較 ===" << endl;

    cout << "  全域函數：" << globalAdd(1, 2) << endl;
    cout << "  命名空間：" << MathUtils::add(3, 4) << endl;
    cout << "  靜態函數：" << Calculator::add(5, 6) << endl;

    Calculator::add(7, 8);
    Calculator::add(9, 10);
    cout << "  Calculator 運算次數：" << Calculator::getOperationCount() << endl;

    return 0;
}
