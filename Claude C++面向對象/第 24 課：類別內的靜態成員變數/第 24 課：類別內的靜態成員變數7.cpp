#include <iostream>
using namespace std;

class MathConstants {
public:
    // constexpr static：編譯期常量，最高效
    // C++17 以後，constexpr static 成員變數不需要在類外定義
    // 常量定義
    static constexpr double PI = 3.14159265358979;
    static constexpr double E  = 2.71828182845905;
    static constexpr int    MAX_DIMENSION = 3;

    // 編譯期計算
    // 其他常量可以基於 PI 計算得出
    // 這些也是編譯期常量，使用時不會有運算開銷
    static constexpr double TWO_PI = PI * 2.0;
    static constexpr double PI_SQUARED = PI * PI;

    static double circleArea(double r) {
        return PI * r * r;
    }

    static double sphereVolume(double r) {
        return (4.0 / 3.0) * PI * r * r * r;
    }
};

// constexpr static 不需要類別外定義（C++17）
// constexpr static 成員變數在編譯期就有值了，不需要在類外定義
// 這些定義如果存在，會導致鏈接錯誤（multiple definition）
// constexpr double MathConstants::PI;  // ❌ 不需要定義

int main() {
    cout << "=== constexpr static ===" << endl;
    cout << "  PI = " << MathConstants::PI << endl;
    cout << "  E  = " << MathConstants::E << endl;
    cout << "  2*PI = " << MathConstants::TWO_PI << endl;
    cout << "  PI^2 = " << MathConstants::PI_SQUARED << endl;

    cout << "\n  圓面積(r=5)：" << MathConstants::circleArea(5) << endl;
    cout << "  球體積(r=3)：" << MathConstants::sphereVolume(3) << endl;

    // 可以用在編譯期需要常量的地方
    int arr[MathConstants::MAX_DIMENSION] = {1, 2, 3};  // ✅
    cout << "\n  陣列大小：" << sizeof(arr) / sizeof(arr[0]) << endl;

    return 0;
}
