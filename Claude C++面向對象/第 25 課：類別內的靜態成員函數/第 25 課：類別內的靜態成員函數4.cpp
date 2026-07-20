// =============================================================================
//  第 25 課：類別內的靜態成員函數 4  —  工具函式類（utility class）
// =============================================================================
//
// 【主題資訊 Information】
//   模式:  class Util {
//          public:
//              static R f(...);       // 全部是靜態函式
//              Util() = delete;       // 明確禁止實例化
//          };
//   標準版本: = delete 是 C++11；靜態成員函式本身是 C++98
//   標頭檔: <cmath>（sqrt、abs）
//   本檔函式複雜度: 全部 O(1)
//
// 【詳細解釋 Explanation】
//
// 【1. 什麼樣的函式該放進工具類】
//   判準是「這個函式需不需要物件狀態」。
//   clamp(value, min, max) 的結果完全由參數決定 ——
//   沒有任何「某個 MathUtil 物件的狀態」會影響答案。
//   這種純函式若寫成非靜態成員，呼叫端就得先生一個毫無意義的物件出來：
//       MathUtil u;  u.clamp(150, 0, 100);   // 那個 u 完全沒用到
//   寫成靜態函式，MathUtil::clamp(150, 0, 100) 直接呼叫，語意乾淨。
//
// 【2. 為什麼要 = delete 建構子】
//   工具類不該有實例。C++11 之前的作法是把建構子設成 private，
//   但那只是「藏起來」——類別自己的成員與 friend 仍然造得出來，
//   而且錯誤訊息是「無法存取」，讀者未必看得出是刻意設計。
//   = delete 的差別在於它是「明確刪除」：
//     * 任何人（含成員函式）都造不出來；
//     * 錯誤訊息直接是 "use of deleted function"，意圖一目了然。
//   注意刪掉建構子之後，這個類別也不能被繼承後實例化。
//
// 【3. clamp 與 lerp 的邊界處理】
//   本檔 lerp 先把 t 夾在 [0,1] 再插值，這是刻意的：
//   t = 1.5 時若不夾住會「外插」，回傳超出 [a,b] 的值。
//   遊戲的過場動畫、UI 進度條都不希望這種結果。
//   標準庫從 C++17 起提供 std::clamp（<algorithm>），
//   C++20 起提供 std::lerp（<cmath>）；
//   要注意 std::lerp 刻意「不」夾住 t，因為外插在數學上是合法需求。
//
// 【4. 工具類 vs 命名空間】
//   若這些函式完全不需要碰任何 private 或 static 狀態，
//   命名空間其實更輕量：
//       namespace MathUtil { int clamp(...); }
//   命名空間可以跨檔案擴充、不必寫 = delete、也不會被誤繼承。
//   選類別的理由通常是：需要 private 的輔助函式、
//   需要共用的 static 狀態，或想用 template 參數化整組工具。
//
// 【概念補充 Concept Deep Dive】
//   * distance() 用 sqrt 開根號。若只是要「比大小」（誰比較近），
//     可以直接比平方距離省掉 sqrt —— 這是遊戲與圖形程式的常見最佳化。
//   * isInRange 用 abs(value - center) <= range 比較浮點數。
//     浮點數的相等比較本來就不可靠，這種「帶容差」的比較才是正確做法。
//   * 靜態函式的位址是普通函式指標，所以 MathUtil::clamp 可以直接
//     餵給需要 callback 的 C API 或 std::function。
//   * 工具類的函式全是純函式，天生執行緒安全 ——
//     因為它們不讀寫任何共享狀態。這是純函式設計的附帶好處。
//
// 【注意事項 Pay Attention】
//   1. = delete 建構子之後類別無法實例化，也無法被繼承後實例化。
//   2. std::lerp（C++20）不夾住 t，與本檔的實作行為不同，別混用。
//   3. 浮點數比較要帶容差，不要用 ==。
//   4. 若函式不需要碰 private / static 狀態，用命名空間比類別更輕量。
//   5. clamp 的參數順序各家不一，std::clamp 是 (v, lo, hi)，呼叫前先確認。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】工具函式類
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼工具類要把建構子 = delete?和設成 private 差在哪?
//     答：兩者都能阻止外部建立物件，但語意不同。
//         private 只是「不給外部存取」——類別自己的成員函式與 friend
//         仍然造得出實例，而且錯誤訊息是「無法存取」，看不出是刻意設計。
//         = delete（C++11）是「這個函式不存在」，任何人都造不出來，
//         錯誤訊息直接是 use of deleted function，意圖非常明確。
//     追問：那用命名空間不是更簡單?
//         → 若這些函式不需要存取 private 或共用的 static 狀態，
//         命名空間確實更輕量，還能跨檔案擴充。
//         選類別的理由是需要私有輔助函式、共用狀態，或要用 template 參數化。
//
// 🔥 Q2. 什麼樣的函式適合寫成靜態成員函式?
//     答：結果完全由參數決定、不依賴任何物件狀態的「純函式」。
//         判斷方法很直接：如果寫成非靜態，函式體裡完全沒用到 this
//         指向的任何成員，那它就不該是非靜態的。
//         這類函式若強制要求先建立物件，呼叫端會被迫生出一個沒有意義的實例。
//     追問：純函式有什麼附帶好處?
//         → 天生執行緒安全（不讀寫共享狀態）、易於單元測試
//         （不需要架設物件狀態）、結果可快取。
//
// ⚠️ 陷阱. 「本檔的 lerp 和 C++20 的 std::lerp 一樣，換過去就好。」
//     答：行為不同。本檔的 lerp 會先把 t 夾在 [0,1]，
//         所以 t = 1.5 時回傳 b；std::lerp 刻意不夾住，
//         t = 1.5 會做「外插」回傳超出 [a,b] 的值。
//         直接替換會讓動畫在邊界處出現預期外的過衝。
//     為什麼會錯：看到同名函式就假設語意相同。
//         實際上標準庫刻意保留外插能力（那是數學上合法且有用的需求），
//         是本檔的版本額外加了夾取。名字相同不代表契約相同，
//         替換前一定要看清楚邊界行為。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cmath>
#include <vector>
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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】兩題純函式，正好示範工具類的適用範圍
//   共同點：結果完全由參數決定，沒有任何物件狀態可言 ——
//           這正是「該寫成靜態成員函式」的判準。
// -----------------------------------------------------------------------------
class ArrayUtil {
public:
    ArrayUtil() = delete;                 // 同樣禁止實例化

    // 【LeetCode 66. Plus One】
    //   題目：用陣列表示一個非負整數（高位在前），對它加一後回傳新陣列。
    //   為什麼放這裡：純粹的數值運算，與 MathUtil::clamp 同類。
    //   關鍵：從個位往前處理進位；若最高位仍有進位（全 9 的情況），
    //         結果會多一位，例如 [9,9] → [1,0,0]。
    //   複雜度：時間 O(n)，空間 O(1)（全 9 時 O(n)）。
    static vector<int> plusOne(vector<int> digits) {
        for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
            if (digits[static_cast<size_t>(i)] < 9) {
                ++digits[static_cast<size_t>(i)];
                return digits;            // 沒有進位，直接結束
            }
            digits[static_cast<size_t>(i)] = 0;   // 9 → 0，繼續往前進位
        }
        // 走到這裡代表原本每一位都是 9
        digits.insert(digits.begin(), 1);
        return digits;
    }

    // 【LeetCode 1480. Running Sum of 1d Array】
    //   題目：回傳前綴和陣列，runningSum[i] = nums[0] + ... + nums[i]。
    //   為什麼放這裡：同樣是純函式，輸入決定輸出。
    //   複雜度：時間 O(n)，就地累加，額外空間 O(1)。
    static vector<int> runningSum(vector<int> nums) {
        for (size_t i = 1; i < nums.size(); ++i) {
            nums[i] += nums[i - 1];
        }
        return nums;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】監控圖表的資料點正規化
//   情境：把後端回傳的原始數值（例如 CPU 溫度）畫成一張高度 40 像素的圖。
//         需要兩步：先用 clamp 把離群值壓進合理範圍（感測器偶爾會噴出
//         -999 或 9999 這種錯誤讀數），再用線性映射換算成像素高度。
//   為什麼用靜態工具函式：這兩步都是純運算，
//         與「哪一張圖表物件」無關，不該綁在任何實例上。
// -----------------------------------------------------------------------------
class ChartScaler {
public:
    ChartScaler() = delete;

    // 把 [inMin, inMax] 的值映射到 [0, pixelHeight]，並先夾住離群值
    static int toPixelHeight(double value, double inMin, double inMax,
                             int pixelHeight) {
        const int clamped = MathUtil::clamp(static_cast<int>(value),
                                            static_cast<int>(inMin),
                                            static_cast<int>(inMax));
        if (inMax <= inMin) return 0;     // 防止除以零
        const double ratio = (clamped - inMin) / (inMax - inMin);
        return static_cast<int>(MathUtil::lerp(0, pixelHeight, ratio));
    }
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
    //              // 錯誤訊息：use of deleted function 'MathUtil::MathUtil()'

    // 邊界行為：本檔 lerp 會夾住 t，不做外插
    cout << "\n--- lerp 的邊界（t 被夾在 [0,1]，不外插）---" << endl;
    cout << "  lerp(0, 100, 1.5) = " << MathUtil::lerp(0, 100, 1.5)
         << "（t 被夾成 1.0，回傳 b；std::lerp 會回 150）" << endl;
    cout << "  lerp(0, 100, -0.5) = " << MathUtil::lerp(0, 100, -0.5)
         << "（t 被夾成 0.0，回傳 a）" << endl;

    cout << "\n=== LeetCode 66. Plus One ===" << endl;
    {
        auto show = [](const vector<int>& v) {
            cout << "[";
            for (size_t i = 0; i < v.size(); ++i) {
                cout << v[i];
                if (i + 1 < v.size()) cout << ",";
            }
            cout << "]";
        };

        vector<vector<int>> cases{{1, 2, 3}, {4, 3, 2, 1}, {9}, {9, 9}, {0}};
        for (const auto& c : cases) {
            cout << "  ";
            show(c);
            cout << " → ";
            show(ArrayUtil::plusOne(c));
            cout << endl;
        }
        cout << "  ↑ [9,9] → [1,0,0]：全 9 時進位會讓長度加一。" << endl;

        cout << "\n=== LeetCode 1480. Running Sum of 1d Array ===" << endl;
        vector<vector<int>> sums{{1, 2, 3, 4}, {1, 1, 1, 1, 1}, {3, 1, 2, 10, 1}};
        for (const auto& c : sums) {
            cout << "  ";
            show(c);
            cout << " → ";
            show(ArrayUtil::runningSum(c));
            cout << endl;
        }
    }

    cout << "\n=== 日常實務：監控圖表的資料點正規化 ===" << endl;
    {
        // 感測器讀數：其中 -999 與 9999 是明顯的錯誤讀數
        const double readings[] = {30, 45, 60, 75, 90, -999, 9999};
        const double kMin = 30, kMax = 90;
        const int    kHeight = 40;

        for (double r : readings) {
            const int h = ChartScaler::toPixelHeight(r, kMin, kMax, kHeight);
            cout << "  溫度 " << r << "°C → 高度 " << h << " 像素";
            if (r < kMin || r > kMax) cout << "（離群值已被 clamp 夾住）";
            cout << endl;
        }
        cout << "  ↑ 若不先 clamp，-999 會算出負的像素高度，畫面直接壞掉。" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 25 課：類別內的靜態成員函數4.cpp -o static_func4

// === 預期輸出 ===
// === 工具函數類 ===
//   clamp(150, 0, 100) = 100
//   clamp(-50, 0, 100) = 0
//   lerp(0, 100, 0.3) = 30
//   lerp(0, 100, 0.7) = 70
//   distance(0,0, 3,4) = 5
//   isInRange(5, 10, 3) = false
//   isInRange(8, 10, 3) = true
//
// --- lerp 的邊界（t 被夾在 [0,1]，不外插）---
//   lerp(0, 100, 1.5) = 100（t 被夾成 1.0，回傳 b；std::lerp 會回 150）
//   lerp(0, 100, -0.5) = 0（t 被夾成 0.0，回傳 a）
//
// === LeetCode 66. Plus One ===
//   [1,2,3] → [1,2,4]
//   [4,3,2,1] → [4,3,2,2]
//   [9] → [1,0]
//   [9,9] → [1,0,0]
//   [0] → [1]
//   ↑ [9,9] → [1,0,0]：全 9 時進位會讓長度加一。
//
// === LeetCode 1480. Running Sum of 1d Array ===
//   [1,2,3,4] → [1,3,6,10]
//   [1,1,1,1,1] → [1,2,3,4,5]
//   [3,1,2,10,1] → [3,4,6,16,17]
//
// === 日常實務：監控圖表的資料點正規化 ===
//   溫度 30°C → 高度 0 像素
//   溫度 45°C → 高度 10 像素
//   溫度 60°C → 高度 20 像素
//   溫度 75°C → 高度 30 像素
//   溫度 90°C → 高度 40 像素
//   溫度 -999°C → 高度 0 像素（離群值已被 clamp 夾住）
//   溫度 9999°C → 高度 40 像素（離群值已被 clamp 夾住）
//   ↑ 若不先 clamp，-999 會算出負的像素高度，畫面直接壞掉。
