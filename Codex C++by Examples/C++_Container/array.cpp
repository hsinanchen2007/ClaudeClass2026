// ============================================================================
// array：編譯期固定長度、具容器介面的連續陣列
// ============================================================================
// std::array<T, N> 把內建陣列包成一般 value type：可複製、可比較、能取得 size、
// begin/end/data，且不會像函式參數中的 T[] 自動退化成 pointer。N 是型別的一部分，
// 所以 array<int, 3> 與 array<int, 4> 是不同型別。
//
// 元素通常直接位於 array 物件內，不做額外 heap 配置。所有索引與迭代皆 O(1)
// 或 O(n) 線性走訪；長度永遠不變，因此沒有 push_back/erase，也沒有擴容失效。
// array 賦值只對既有元素做 assignment，不結束元素 subobject 的生命；既有 pointer/
// reference 仍指向相同位置，只是看到新值。只有 array 物件（以及元素）生命結束後，
// 這些 handle 才會懸空。

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <span>

void basic_demo()
{
    std::array<int, 4> ports{8080, 443, 22, 5432};
    std::ranges::sort(ports);
    assert((ports == std::array<int, 4>{22, 443, 5432, 8080}));
    assert(ports.front() == 22 && ports.back() == 8080);
    assert(std::accumulate(ports.begin(), ports.end(), 0) == 13'977);

    // at() 有邊界檢查；operator[] 越界是 UB。正常路徑以 assert 驗證有效索引。
    assert(ports.at(1) == 443);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 121. Best Time to Buy and Sell Stock（買賣股票的最佳時機）
// 題目：輸入每日股價，最多完成一次先買後賣，求最大利潤；例如 [7,1,5,3,6,4] 回傳 5。
// 為何使用本章主題：std::array 保存固定筆數測資，演算法則以 span 接受任意連續容器且不複製資料。
// 思路：先以首價初始化最低價；由第二天起更新「今日賣出」利潤；同步保留歷史最低買價。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為股價筆數。
// 易錯點：必須先買後賣；空輸入回 0；span 不擁有來源，呼叫期間來源不得失效。
// -----------------------------------------------------------------------------
int max_profit(std::span<const int> prices)
{
    if (prices.empty()) {
        return 0;
    }
    int lowest = prices.front();
    int best = 0;
    for (const int price : prices.subspan(1)) {
        best = std::max(best, price - lowest);
        lowest = std::min(lowest, price);
    }
    return best;
}

void leetcode_demo()
{
    const std::array prices{7, 1, 5, 3, 6, 4};
    assert(max_profit(prices) == 5);
    const std::array falling{7, 6, 4, 3, 1};
    assert(max_profit(falling) == 0);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定四通道感測器校正
// 情境：硬體每次回傳四個通道的原始值，需逐通道套用 offset 與 scale 產生校正值。
// 為何使用本章主題：通道數是協定的一部分，array 把長度寫入型別，比可變長 vector 更早拒絕錯誤尺寸。
// 設計：定義 FourChannels；逐索引讀取三組對應值；計算 (raw-offset)*scale 並回傳新 array。
// 成本：固定四次運算，時間 O(C)、額外空間 O(C)，此處 C 固定為 4 且沒有 heap 配置。
// 上線注意：需驗證校正常數來源與浮點非有限值；若保存元素位址，array 本體生命週期必須涵蓋使用期。
// -----------------------------------------------------------------------------
using FourChannels = std::array<double, 4>;

FourChannels calibrate(const FourChannels& raw,
                       const FourChannels& offset,
                       const FourChannels& scale)
{
    FourChannels result{};
    for (std::size_t index = 0; index < result.size(); ++index) {
        result.at(index) = (raw.at(index) - offset.at(index)) * scale.at(index);
    }
    return result;
}

void practical_demo()
{
    const FourChannels raw{11.0, 22.0, 33.0, 44.0};
    const FourChannels offset{1.0, 2.0, 3.0, 4.0};
    const FourChannels scale{0.5, 0.5, 0.5, 0.5};
    assert((calibrate(raw, offset, scale) == FourChannels{5, 10, 15, 20}));
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "array：固定大小資料與 span 介面測試通過\n";
}

// 【陷阱】std::array<int, 4> a; 對基本型別不保證零初始化；用 a{} 才清零。
// 【陷阱】空 array 的 data() 不可解參考；begin()==end() 才是可依賴的判斷。
// 【面試】何時 array 優於 vector？固定協定大小、stack/value semantics、免配置。
// 【練習】將 calibrate 改成 template<size_t N>，支援任意固定通道數。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'array.cpp' -o '/tmp/codex_cpp_C_Container_array' && '/tmp/codex_cpp_C_Container_array'
//
// === 預期輸出（節錄）===
// array：固定大小資料與 span 介面測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
