#include <iostream>
using namespace std;

class WithStatic {
    int a_;                          // 4 bytes
    int b_;                          // 4 bytes
    inline static int shared_ = 0;    // 不計入 sizeof, 所有對象共享同一份資料
    inline static double config_ = 0; // 不計入 sizeof, 所有對象共享同一份資料
};

class WithoutStatic {
    int a_;   // 4 bytes
    int b_;   // 4 bytes
};

int main() {
    cout << "=== sizeof 與靜態成員 ===" << endl;
    cout << "  WithStatic 大小：" << sizeof(WithStatic) << " bytes" << endl;
    cout << "  WithoutStatic 大小：" << sizeof(WithoutStatic) << " bytes" << endl;
    cout << "  兩者相同！靜態成員不佔對象空間。" << endl;

    return 0;
}
