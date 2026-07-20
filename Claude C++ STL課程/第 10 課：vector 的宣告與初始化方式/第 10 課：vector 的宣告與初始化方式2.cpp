// =============================================================================
//  第 10 課：vector 的宣告與初始化方式 2  —  指定大小的建構 vector(n) / vector(n, val)
// =============================================================================
//
// 【主題資訊 Information】
//   explicit vector(size_type count);                      // (1) C++11 起（值初始化 count 個元素）
//            vector(size_type count, const T& value);      // (2) count 個 value 的複本
//
//   標頭檔　：<vector>
//   標準版本：(2) 自 C++98；(1) 在 C++98 是 vector(count, T(), alloc) 的預設引數形式，
//             C++11 才拆成獨立且 explicit 的單參數版本
//   複雜度　：O(count)——會實際建構 count 個元素
//
// 【詳細解釋 Explanation】
//
// 【1. vector(n) 是「值初始化」，不是「未初始化」】
//   std::vector<int> v(5);  → {0, 0, 0, 0, 0}，五個元素保證都是 0。
//   這點和 C 的區域變數 int arr[5];（內容是垃圾）完全不同。
//   標準用語是 value-initialize：
//     * 對內建型別（int、double、指標）→ 補零
//     * 對有 default ctor 的 class（std::string）→ 呼叫 default ctor（空字串）
//   所以 vector<int> v(1000000) 一定會付出「寫入 4 MB 的 0」這個成本。
//   若你只是要預留空間、之後才逐一 push_back，請用 reserve(n)——
//   reserve 只配置記憶體不建構元素，size 仍是 0，這是常見的效能差異來源。
//
// 【2. vector(n) 為什麼是 explicit？】
//   因為 vector<int> v = 5; 這種寫法毫無意義且極易誤解（5 是「大小」還是「值」？）。
//   加上 explicit 之後，單參數版本只能用直接初始化 vector<int> v(5);。
//   而 (count, value) 的雙參數版本不需要 explicit——兩個參數本來就無法隱式轉換。
//
// 【3. vector(n, val) 做的是「複製 n 次」】
//   value 以 const T& 傳入，然後被複製 count 次。對 T 是 int 沒差，
//   但對 T 是 std::string 或自訂類別，就是 count 次 copy constructor。
//   （見本課第 8 個檔案的建構子計次實驗，實測 1 次 ctor + n 次 copy-ctor。）
//
// 【4. 這兩個建構子對 T 的要求不同】
//   * vector(n)      要求 T 可 default-construct（值初始化）
//   * vector(n, val) 要求 T 可 copy-construct
//   所以「沒有預設建構子的類別」不能寫 vector<T> v(5);，但可以寫 vector<T> v(5, t);
//   這是設計 class 時常被忽略的相容性考量。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 建構後的 capacity()：
//     本機 libstdc++ 實測 vector<int> v(5) 與 v(5, 42) 的 capacity() 都恰好是 5——
//     實作知道確切大小，所以一次配置剛好的量，不套用成長倍率。
//     這是【實作定義】行為，標準只要求 capacity() >= size()。
//     對比之下，用 push_back 逐一長到 5 個，capacity 會是 1→2→4→8（見輸出）。
//
//   ▸ 值初始化的實作優化：
//     對 trivially-constructible 且填 0 的情形，libstdc++ 會退化成 memset，
//     所以 vector<int> v(n) 通常比手寫 for 迴圈填 0 更快。
//     但這僅適用於 int 這類型別；vector<std::string> v(n) 仍需逐一呼叫 ctor。
//
// 【注意事項 Pay Attention】
//   1. ★ vector<int> v(5) 是 5 個 0；vector<int> v{5} 是 1 個元素值為 5。
//      括號形狀決定一切——這是本課最重要的陷阱，詳見第 4 個檔案。
//   2. vector<int> v(5) 之後 v.size() 就是 5，可以直接 v[0]..v[4] 索引，
//      不需要（也不應該）再 push_back——那會變成 6 個元素。
//   3. 要「預留容量」而非「建立元素」請用 reserve(n)：
//      reserve 後 size() 仍是 0，此時 v[0] 是 UB。兩者用途完全不同。
//   4. 傳入的 count 是 size_type（無號）。寫 vector<int> v(-1) 會被轉成天文數字，
//      通常導致丟出 std::length_error 或 std::bad_alloc，而不是給你一個空 vector。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector(n) 與 vector(n, val)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector<int> v(5); 裡面的值是什麼？和 int arr[5]; 有何不同？
//     答：五個元素保證都是 0——vector(n) 執行的是「值初始化」。
//         而區域變數 int arr[5]; 是預設初始化，內容未定義，讀取是 UB。
//         這是 vector 相對於 C 陣列的安全性優勢之一。
//     追問：那 vector<std::string> v(5) 呢？→ 五個「空字串」，
//         值初始化對 class type 就是呼叫其 default constructor。
//
// 🔥 Q2. vector<int> v(1000); 和 vector<int> v; v.reserve(1000); 差在哪？
//     答：前者 size()==1000、capacity()>=1000，並且真的寫入了 1000 個 0；
//         後者 size()==0、capacity()>=1000，一個元素都沒建構。
//         前者可立即 v[i] 索引；後者此時 v[0] 是 UB，必須先 push_back。
//     追問：哪個快？→ reserve 快，因為省下初始化成本。
//         若你接著要用 push_back 填滿，reserve 是正確選擇；
//         若你要用 v[i] = ... 隨機寫入，就必須用 vector(n)。
//
// ⚠️ 陷阱. 「vector<int> v(5); 然後 for 迴圈 v.push_back(x) 五次」——結果是什麼？
//     答：得到 10 個元素：前 5 個是 0，後 5 個才是你 push 的值。
//         vector(5) 已經「建立」了 5 個元素，不是「預留」5 個空位。
//     為什麼會錯：把 vector(n) 當成 Java 的 new ArrayList<>(5)
//         （那是預留容量、size 為 0）。C++ 對應 Java 那個語意的是 reserve(5)；
//         vector(5) 對應的其實是 Java 的 new int[5]。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 1480. Running Sum of 1d Array
//   題目：給定陣列 nums，回傳 runningSum，其中 runningSum[i] = nums[0]+...+nums[i]。
//   為什麼用到本主題：結果陣列的大小「事先就知道」= nums.size()，
//     所以用 vector<int> res(nums.size()) 一次配置到位，再用索引寫入。
//     這比「空 vector + push_back」少掉多次重新配置，是競賽與實務的標準寫法。
//   複雜度：時間 O(n)、額外空間 O(n)。
// -----------------------------------------------------------------------------
std::vector<int> runningSum(const std::vector<int>& nums) {
    std::vector<int> res(nums.size());          // n 個 0，可立即用索引寫入
    int acc = 0;
    for (std::size_t i = 0; i < nums.size(); ++i) {
        acc += nums[i];
        res[i] = acc;
    }
    return res;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 1920. Build Array from Permutation
//   題目：給定 0 到 n-1 的排列 nums，回傳 ans，其中 ans[i] = nums[nums[i]]。
//   為什麼用到本主題：必須「先有 n 個位置」才能隨機寫入 ans[i]。
//     若用空 vector + push_back 也能做，但 vector<int> ans(n) 語意更直接，
//     且明確表達「大小已知、不會增減」。
//   複雜度：時間 O(n)、額外空間 O(n)。
// -----------------------------------------------------------------------------
std::vector<int> buildArray(const std::vector<int>& nums) {
    std::vector<int> ans(nums.size());          // 大小已知 → vector(n)
    for (std::size_t i = 0; i < nums.size(); ++i) {
        ans[i] = nums[nums[i]];
    }
    return ans;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定長度的感測器環形緩衝區（ring buffer）
//   情境：溫度感測器每秒回報一次，我們只保留最近 N 秒的讀數做移動平均。
//   為什麼用 vector(n, val)：緩衝區長度固定不變，且需要一個明確的
//     「尚未取得讀數」初值（這裡用 -273.15，絕對零度，代表無效值）。
//     用 vector(n, val) 一行完成配置 + 初始化，之後只用索引覆寫，
//     全程不會觸發任何重新配置——這對即時系統的延遲穩定性很重要。
// -----------------------------------------------------------------------------
class SensorBuffer {
public:
    explicit SensorBuffer(std::size_t capacity)
        : samples_(capacity, kInvalid), next_(0) {}   // n 個無效值

    void record(double celsius) {
        samples_[next_] = celsius;                    // 只覆寫，不 push_back
        next_ = (next_ + 1) % samples_.size();        // 環形前進
    }

    // 只對「已取得的有效讀數」做平均
    double average() const {
        double sum = 0.0;
        int    n   = 0;
        for (double s : samples_) {
            if (s > kInvalid) { sum += s; ++n; }
        }
        return n == 0 ? kInvalid : sum / n;
    }

    std::size_t size() const { return samples_.size(); }

private:
    static constexpr double kInvalid = -273.15;       // 絕對零度 = 尚未有讀數
    std::vector<double>     samples_;
    std::size_t             next_;
};

int main() {
    std::cout << "=== vector(n)：n 個值初始化元素 ===\n";
    std::vector<int> v1(5);
    std::cout << "vector<int> v1(5)      -> size=" << v1.size()
              << " capacity=" << v1.capacity() << " 內容: ";
    for (int x : v1) std::cout << x << " ";
    std::cout << "\n（int 被值初始化為 0，不是垃圾值）\n";

    std::vector<std::string> vs(3);
    std::cout << "vector<string> vs(3)   -> size=" << vs.size()
              << "，每個都是空字串，長度: ";
    for (const std::string& s : vs) std::cout << s.size() << " ";
    std::cout << "\n";

    std::cout << "\n=== vector(n, val)：n 個 val 的複本 ===\n";
    std::vector<int> v2(5, 42);
    std::cout << "vector<int> v2(5, 42)  -> size=" << v2.size()
              << " capacity=" << v2.capacity() << " 內容: ";
    for (int x : v2) std::cout << x << " ";
    std::cout << "\n";

    std::vector<std::string> vs2(3, "N/A");
    std::cout << "vector<string> vs2(3, \"N/A\") -> 內容: ";
    for (const std::string& s : vs2) std::cout << "[" << s << "] ";
    std::cout << "\n";

    std::cout << "\n=== 對照：vector(n) 一次配置 vs push_back 逐步成長 ===\n";
    std::cout << "vector<int>(5) 的 capacity = " << v1.capacity()
              << "（實作知道確切大小，配置剛好；【實作定義】）\n";
    std::vector<int> grown;
    std::cout << "push_back 成長過程的 capacity: ";
    std::size_t last = grown.capacity();
    for (int i = 0; i < 5; ++i) {
        grown.push_back(i);
        if (grown.capacity() != last) {
            std::cout << grown.capacity() << " ";
            last = grown.capacity();
        }
    }
    std::cout << "\n（本機 libstdc++ 成長倍率為 2；此為【實作定義】，MSVC 約 1.5）\n";

    std::cout << "\n=== 陷阱示範：vector(5) 之後再 push_back ===\n";
    std::vector<int> trap(5);
    for (int i = 1; i <= 5; ++i) trap.push_back(i * 10);
    std::cout << "vector<int> trap(5) 再 push_back 五次 -> size=" << trap.size() << " 內容: ";
    for (int x : trap) std::cout << x << " ";
    std::cout << "\n（前 5 個 0 是 vector(5) 建立的，不是「預留空位」）\n";

    std::cout << "\n=== LeetCode 1480. Running Sum of 1d Array ===\n";
    for (int x : runningSum({1, 2, 3, 4}))     std::cout << x << " ";
    std::cout << "\n";
    for (int x : runningSum({1, 1, 1, 1, 1}))  std::cout << x << " ";
    std::cout << "\n";
    for (int x : runningSum({3, 1, 2, 10, 1})) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== LeetCode 1920. Build Array from Permutation ===\n";
    for (int x : buildArray({0, 2, 1, 5, 3, 4})) std::cout << x << " ";
    std::cout << "\n";
    for (int x : buildArray({5, 0, 1, 2, 3, 4})) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== 日常實務：固定長度感測器緩衝區 ===\n";
    SensorBuffer buf(5);
    std::cout << "緩衝區長度 = " << buf.size() << "，尚無讀數時平均 = "
              << buf.average() << "（無效值）\n";
    const double readings[] = {23.5, 24.0, 23.8, 24.2, 23.9, 25.1, 25.4};
    for (double r : readings) {
        buf.record(r);
        std::cout << "  記錄 " << r << " -> 目前移動平均 = " << buf.average() << "\n";
    }
    std::cout << "（只保留最近 5 筆；第 6、7 筆覆蓋掉最舊的兩筆）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：vector 的宣告與初始化方式2.cpp" -o lesson10_2

// === 預期輸出 ===
// === vector(n)：n 個值初始化元素 ===
// vector<int> v1(5)      -> size=5 capacity=5 內容: 0 0 0 0 0
// （int 被值初始化為 0，不是垃圾值）
// vector<string> vs(3)   -> size=3，每個都是空字串，長度: 0 0 0
//
// === vector(n, val)：n 個 val 的複本 ===
// vector<int> v2(5, 42)  -> size=5 capacity=5 內容: 42 42 42 42 42
// vector<string> vs2(3, "N/A") -> 內容: [N/A] [N/A] [N/A]
//
// === 對照：vector(n) 一次配置 vs push_back 逐步成長 ===
// vector<int>(5) 的 capacity = 5（實作知道確切大小，配置剛好；【實作定義】）
// push_back 成長過程的 capacity: 1 2 4 8
// （本機 libstdc++ 成長倍率為 2；此為【實作定義】，MSVC 約 1.5）
//
// === 陷阱示範：vector(5) 之後再 push_back ===
// vector<int> trap(5) 再 push_back 五次 -> size=10 內容: 0 0 0 0 0 10 20 30 40 50
// （前 5 個 0 是 vector(5) 建立的，不是「預留空位」）
//
// === LeetCode 1480. Running Sum of 1d Array ===
// 1 3 6 10
// 1 2 3 4 5
// 3 4 6 16 17
//
// === LeetCode 1920. Build Array from Permutation ===
// 0 1 2 4 5 3
// 4 5 0 1 2 3
//
// === 日常實務：固定長度感測器緩衝區 ===
// 緩衝區長度 = 5，尚無讀數時平均 = -273.15（無效值）
//   記錄 23.5 -> 目前移動平均 = 23.5
//   記錄 24 -> 目前移動平均 = 23.75
//   記錄 23.8 -> 目前移動平均 = 23.7667
//   記錄 24.2 -> 目前移動平均 = 23.875
//   記錄 23.9 -> 目前移動平均 = 23.88
//   記錄 25.1 -> 目前移動平均 = 24.2
//   記錄 25.4 -> 目前移動平均 = 24.48
// （只保留最近 5 筆；第 6、7 筆覆蓋掉最舊的兩筆）
