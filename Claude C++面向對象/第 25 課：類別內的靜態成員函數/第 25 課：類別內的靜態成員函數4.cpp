#include <iostream>
#include <string>
#include <cmath>
using namespace std;

class MathUtil {
public:
    // 全部是靜態函數——這個類別不需要創建對象
    // 這些函數提供常用的數學工具，適合在遊戲開發中使用
    static int clamp(int value, int minVal, int maxVal) {
        if (value < minVal) return minVal;
        if (value > maxVal) return maxVal;
        return value;
    }

    static double lerp(double a, double b, double t) {
        t = (t < 0.0) ? 0.0 : (t > 1.0) ? 1.0 : t;
        return a + (b - a) * t;
    }

    static double distance(double x1, double y1, double x2, double y2) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return sqrt(dx * dx + dy * dy);
    }

    static bool isInRange(double value, double center, double range) {
        return abs(value - center) <= range;
    }

    // 禁止創建對象
    // 這個類別只是工具函數的集合，不需要實例化
    // delete 建構函數，讓編譯器報錯如果有人試圖創建對象
    // 這是一種常見的設計模式，稱為 "static class" 或 "utility class"
    MathUtil() = delete;
};

int main() {
    cout << "=== 工具函數類 ===" << endl;

    // 不需要創建對象，直接用類別名調用
    // 測試 clamp 函數
    // clamp 函數會將值限制在指定的範圍內
    // 例如，clamp(150, 0, 100) 會返回 100，因為 150 超過了上限
    // clamp(-50, 0, 100) 會返回 0，因為 -50 低於了下限
    // 測試 lerp 函數
    // lerp 函數會在 a 和 b 之間根據 t 的
    // 位置返回一個插值值
    // 例如，lerp(0, 100, 0.3)
    // 會返回 30，因為 0.3 表示在 0 和 100 之間的 30% 的位置
    // 測試 distance 函數
    // distance 函數會計算兩點之間的距離
    // 例如，distance(0,0, 3,4) 會返回 5，因為 (3,4) 和 (0,0) 之間的距離是 5
    // 測試 isInRange 函數 
    // isInRange 函數會檢查 value 是否在 center 的 range 範圍內
    // 例如，isInRange(5, 10, 3) 會返回 false，因為 5 不在 10 的 3 範圍內
    // isInRange(8, 10, 3) 會返回 true，
    // 因為 8 在 10 的 3 範圍內
    // 嘗試創建 MathUtil 對象會導致編譯錯誤
    cout << "  clamp(150, 0, 100) = " << MathUtil::clamp(150, 0, 100) << endl;
    cout << "  clamp(-50, 0, 100) = " << MathUtil::clamp(-50, 0, 100) << endl;

    cout << "  lerp(0, 100, 0.3) = " << MathUtil::lerp(0, 100, 0.3) << endl;
    cout << "  lerp(0, 100, 0.7) = " << MathUtil::lerp(0, 100, 0.7) << endl;

    cout << "  distance(0,0, 3,4) = " << MathUtil::distance(0, 0, 3, 4) << endl;

    cout << "  isInRange(5, 10, 3) = "
         << (MathUtil::isInRange(5, 10, 3) ? "true" : "false") << endl;
    cout << "  isInRange(8, 10, 3) = "
         << (MathUtil::isInRange(8, 10, 3) ? "true" : "false") << endl;

    // MathUtil m;  // ❌ 編譯錯誤！建構函數被 delete

    return 0;
}
