// ============================================================================
// Parallel STL：execution policy、純函式要求與可攜性邊界
// ============================================================================
// C++17 演算法可接 seq/par/par_unseq/unseq policy。par 允許多 thread；unseq 允許同一
// thread 內交錯/vectorize；par_unseq 兩者皆可。實作是否真的平行、使用多少 worker、
// 是否需 TBB 等 backend 是 toolchain 決定，不是語言保證，必須 benchmark/build 驗證。
//
// policy callable 不可依賴執行順序、不可 data race；unseq/par_unseq 下不可呼叫 mutex
// lock 等 vectorization-unsafe operation。標準對 allocation/deallocation 有明文例外，不能
// 泛稱「配置即違規」；但配置仍可能昂貴、爭用 allocator 或失敗，不適合 hot callback。
// reduce 需要運算近似可結合；浮點加法非結合，因此平行 reduce 可能與 sequential bit
// pattern 不同。
//
// 本機 libstdc++ 的 policy overload 使用 oneTBB。預設教材可用一般 C++20 指令建置；
// 若要實際啟用平行 backend：
//   g++ -std=c++20 -O2 -pthread -DCODEX_ENABLE_PARALLEL_STL 09_parallel_stl.cpp -ltbb

#include <algorithm>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

#ifdef CODEX_ENABLE_PARALLEL_STL
#include <execution>
#endif

void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

template <class InputIterator, class OutputIterator, class UnaryOperation>
void policy_transform(InputIterator first,
                      InputIterator last,
                      OutputIterator output,
                      UnaryOperation operation)
{
#ifdef CODEX_ENABLE_PARALLEL_STL
    std::transform(std::execution::par, first, last, output, operation);
#else
    // 不具/未連結平行 backend 時仍保留完全相同語意，只有執行策略不同。
    std::transform(first, last, output, operation);
#endif
}

void basic_demo()
{
    std::vector<int> input{1, 2, 3, 4};
    std::vector<int> output(input.size());

    policy_transform(input.begin(), input.end(), output.begin(),
                     [](int value) noexcept { return value * 2; });
    expect((output == std::vector<int>{2, 4, 6, 8}), "parallel transform mismatch");

    const int total = std::accumulate(output.begin(), output.end(), 0);
    expect(total == 20, "transformed total mismatch");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 977. Squares of a Sorted Array（有序陣列的平方）
// 題目：將升冪整數陣列各元素平方後仍以升冪回傳；例如 [-7,-3,2,3,11] 得 [4,9,9,49,121]。
// 為何使用本章主題：各 index 的平方可獨立 transform，教學上可套 parallel policy；後續 sort 使本解 O(N log N)，非雙指標 O(N) 最佳解。
// 思路：1. 預配置 result。2. policy_transform 各自平方。3. 依 build 選平行或順序 sort。4. 回傳排序結果。
// 複雜度：時間 O(N log N)、輸出空間 O(N)，平行 backend 另有分割、排程與同步成本。
// 易錯點：policy 不保證實際平行且可能需 TBB；callback 要無 data race/不拋，int 平方也須受輸入範圍限制。
// -----------------------------------------------------------------------------
std::vector<int> sorted_squares(const std::vector<int>& numbers)
{
    std::vector<int> result(numbers.size());
    policy_transform(numbers.begin(), numbers.end(), result.begin(),
                     [](int value) noexcept { return value * value; });
#ifdef CODEX_ENABLE_PARALLEL_STL
    std::sort(std::execution::par, result.begin(), result.end());
#else
    std::sort(result.begin(), result.end());
#endif
    return result;
}

void leetcode_demo()
{
    const auto result = sorted_squares({-7, -3, 2, 3, 11});
    expect((result == std::vector<int>{4, 9, 9, 49, 121}),
           "sorted squares mismatch");
}

// -----------------------------------------------------------------------------
// 【日常實務範例】商品價格的批次稅額轉換
// 情境：每個價格獨立乘上 1+rate，例如 [10,20,40] 套 10% 後得到 [11,22,44]，輸出 index 必須對齊輸入。
// 為何使用本章主題：純函式 transform 沒有跨元素依賴，適合 execution policy；預配置輸出避免 callback 競爭 push_back。
// 設計：1. 配置同尺寸 result。2. lambda 按值捕獲 rate。3. policy_transform 各 index。4. 完成後回傳。
// 成本：總工作與輸出空間 O(N)，平行化是否划算取決於 N、backend 與每元素運算量。
// 上線注意：需驗證負值、NaN/Infinity、稅率範圍與貨幣 rounding；policy callback 若逸出例外可能直接 terminate。
// -----------------------------------------------------------------------------
std::vector<double> apply_tax(const std::vector<double>& prices, double rate)
{
    std::vector<double> result(prices.size());
    policy_transform(prices.begin(), prices.end(), result.begin(),
                     [rate](double price) noexcept { return price * (1.0 + rate); });
    return result;
}

void practical_demo()
{
    const auto taxed = apply_tax({10.0, 20.0, 40.0}, 0.10);
    expect(taxed.size() == 3U, "tax result size mismatch");
    expect(taxed.at(0) == 11.0 && taxed.at(2) == 44.0, "tax calculation mismatch");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "Parallel STL：execution policy 與獨立 transform 測試通過\n";
}

// 【陷阱】在 par_unseq callback 內 push_back 同一 vector 是 data race；先配置輸出範圍。
// 【注意】new/delete 並非因 unsequenced policy 自動違規，但 allocation cost 仍可能吞掉加速。
// 【陷阱】policy overload 若 user callable 丟 exception，標準 policy 常導致 terminate；
//         不可照搬一般 sequential exception handling 心智模型。
// 【面試】為何 parallel sort 小資料可能更慢？partition、排程、同步與 cache overhead。
// 【練習】在有 TBB/backend 的環境將 seq 換 par，驗證正確後 benchmark crossover size。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '09_parallel_stl.cpp' -o '/tmp/codex_cpp_C_MultiThread_09_parallel_stl' && '/tmp/codex_cpp_C_MultiThread_09_parallel_stl'
//
// === 預期輸出（節錄）===
// Parallel STL：execution policy 與獨立 transform 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
