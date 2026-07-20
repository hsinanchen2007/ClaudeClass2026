/*
 * Min/Max 家族：面試前完整速查章
 * =================================
 *
 * API                 回傳                空範圍       複雜度 / tie
 * min/max(a,b)        const T&            不適用       1 比較，tie 回第一參數
 * min/max({..})       T                   不可空       O(N)
 * min_element         iterator            end          N-1，第一個最小
 * max_element         iterator            end          N-1，第一個最大
 * minmax(a,b)         pair<const T&,...>  不適用       1，tie 分別第一/第二
 * minmax_element      pair<iterator,...>  end/end       約 3N/2，第一 min/最後 max
 * clamp(v,lo,hi)      const T&            不適用       <=2，要求 !(hi<lo)
 *
 * 生命週期速記：回 reference 的 min/max/minmax/clamp 不擁有物件；參數若是 temporary，
 * 不可把 reference 留到下一個 statement。*_element 的 iterator 受容器失效規則約束。
 * 空範圍的 iterator 不可解參考。若只需數值且型別便宜，複製通常最安全。
 *
 * 比較器契約：strict weak ordering，使用 < 意義的函式；不可用 <=、不可依賴會變動
 * 的外部狀態。NaN 會使一般全序直覺失效。複合型別要明確定義 tie-breaking。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

struct Statistics {
    int minimum;
    int maximum;
    long long spread;
    std::size_t minimum_index;
    std::size_t maximum_index;
};

// 實務整合：單趟極值後產生監控摘要；空輸入用 optional 表達，而非 magic value。
std::optional<Statistics> practical_summarize(
    const std::vector<int>& samples) {
    if (samples.empty()) {
        return std::nullopt;
    }
    const auto [low, high] = std::minmax_element(samples.begin(), samples.end());
    // 先升格兩個運算元再相減；先做 int subtraction、事後再 cast 已經太晚。
    const long long spread = static_cast<long long>(*high) -
                             static_cast<long long>(*low);
    return Statistics{*low,
                      *high,
                      spread,
                      static_cast<std::size_t>(std::distance(samples.begin(), low)),
                      static_cast<std::size_t>(std::distance(samples.begin(), high))};
}

// LeetCode 121：running min + running max-result；寬回傳型別也涵蓋題目約束外的極值。
long long leetcode_stock_profit(const std::vector<int>& prices) {
    if (prices.empty()) {
        return 0;
    }
    int lowest = prices.front();
    long long profit = 0;
    for (int price : prices) {
        lowest = std::min(lowest, price);
        const long long candidate = static_cast<long long>(price) -
                                    static_cast<long long>(lowest);
        profit = std::max(profit, candidate);
    }
    return profit;
}

int main() {
    const auto stats = practical_summarize({8, 3, 9, 3, 9});
    assert(stats.has_value());
    assert(stats->minimum == 3 && stats->maximum == 9 && stats->spread == 6);
    assert(stats->minimum_index == 1U);  // 第一個 minimum
    assert(stats->maximum_index == 4U);  // minmax_element 回最後一個 maximum
    assert(!practical_summarize({}).has_value());

    const auto extremes = practical_summarize(
        {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()});
    assert(extremes.has_value());
    assert(extremes->spread ==
           static_cast<long long>(std::numeric_limits<int>::max()) -
               static_cast<long long>(std::numeric_limits<int>::min()));

    assert(std::clamp(120, 0, 100) == 100);
    assert(leetcode_stock_profit({7, 1, 5, 3, 6, 4}) == 5);
    assert(leetcode_stock_profit({std::numeric_limits<int>::min(),
                                  std::numeric_limits<int>::max()}) ==
           extremes->spread);
    std::cout << "Min/Max 家族整合複習完成\n";
}

/*
 * 面試 Q&A
 * Q: 需要 value 與 index 應選哪個？ A: *_element，先驗 end 再解參考/算距離。
 * Q: 同時找 min/max 為何用 minmax_element？ A: 約 1.5N 比較，少於兩掃約 2N。
 * Q: std::max({}) 可否空？ A: 不可；沒有可回傳的值，屬錯誤前置條件。
 * Q: clamp 的 low>high 呢？ A: 前置條件違反，不能期待自動交換。
 * Q: tie 規則重要嗎？ A: 重要；minmax_element 最大值選最後一個，可能影響 index。
 *
 * 練習：
 * 1. summarize 改用 comparator 支援自訂 Reading；
 * 2. 把輸入擴充為 long long，設計 checked/unsigned spread 以涵蓋更寬的差值；
 * 3. 寫 benchmark 比較 minmax_element 與 min_element+max_element 的比較次數。
 *
 * 【選擇流程】
 * 1. 只有兩個值：min/max；同時要兩者：minmax。
 * 2. 一個範圍且要位置：min_element/max_element；同時要兩端：minmax_element。
 * 3. 只要把單值限制區間：clamp，不必建容器。
 * 4. 空範圍是合法輸入：回 optional、iterator end 或業務 error，不要硬解參考。
 *
 * 【複雜度比較】
 * min_element/max_element 各 N-1 次比較；分開跑約 2N-2。
 * minmax_element 兩兩成對比較，約 3N/2，單趟且 cache 行為通常也更好。
 * clamp 固定至多兩次比較。min({list}) 與 max({list}) 各 O(N) 且回值。
 *
 * 【reference / iterator 生命週期】
 * min(a,b)、max(a,b)、minmax(a,b)、clamp(v,lo,hi) 的兩值版本可能回 reference。
 * 若候選含 temporary，請立即複製結果。*_element 回 iterator；vector insert/erase
 * 或 reallocation 後依規則失效。演算法不會延長容器或元素生命。
 *
 * 【整數與浮點陷阱】
 * maximum-minimum 可能 signed overflow；必須先把兩個運算元升格，再做 subtraction。
 * `static_cast<long long>(maximum-minimum)` 無效，因 overflow 已在 int 運算發生。
 * 平均值的 sum 也要用寬型別；若最寬輸入的差仍不可表示，改用 checked arithmetic。
 * 浮點 NaN 不滿足一般全序，min/max 的結果會受參數順序影響；資料進演算法前先
 * 驗 std::isfinite，或定義明確 total-order policy。
 *
 * 【tie 規則速背】
 * min_element：第一個 min；max_element：第一個 max；minmax_element：第一個 min
 * 與最後一個 max。兩值 min/max tie 回第一參數；兩值 minmax 的 max tie 指第二。
 * 業務若要其他規則，將 timestamp/id 納入 comparator，不要依賴輸入偶然順序。
 *
 * 【LeetCode 模式】
 * running min：股票最大利潤；running max：Kadane；一次雙極值：Smallest Range；
 * index：Dominant Index；限制邊界：像素/座標正規化。先找問題需要 value、index、
 * 一端還是兩端，再選 API，避免為了使用演算法而扭曲題目。
 *
 * 【實務測試清單】
 * 空資料、單值、全相同、極值重複、負數、INT_MIN/INT_MAX、NaN/Infinity、custom
 * comparator tie、容器修改後 iterator、low>high 的拒絕路徑都要覆蓋。
 *
 * 【面試白板模板】
 * 先問輸入是否可能空、是否需要 index、是否同時需要兩端、是否允許修改輸入。
 * 若只掃一次，寫 `for (const auto& value : range)` 並維持清楚不變量；不要為了
 * 使用 STL 而重複掃描昂貴 input range。自訂型別 comparator 只讀取穩定 key。
 *
 * 【實務 API 回傳設計】
 * 空輸入可回 optional<Statistics>；也可回 expected<Statistics,Error> 說明原因。
 * 不建議回 {0,0}，因 0 可能是合法極值。若回 iterator pair，文件要寫清楚原容器
 * 必須活著且不可執行會失效的修改。
 *
 * 【效能判讀】
 * Big-O 相同時，比較器成本可能主導。例如 locale 字串比較昂貴，可先抽取/快取
 * normalization key。若資料在 GPU 或分散節點，reduction 的同步與傳輸成本遠高於
 * 幾次比較；仍應維持相同「identity、combine、tie」正確性模型。
 *
 * 【LeetCode 與實務共同原則】
 * 題目約束決定型別與空值策略，實務資料則不能假設永遠符合約束。先寫 deterministic
 * unit tests，再加入 property：minimum<=每個值<=maximum、spread>=0（寬型別）、
 * clamp 結果永遠落在合法區間。
 *
 * 最後易錯提醒：不要只背 API 名稱。先說需求、契約與 tie，再選函式；這比在面試
 * 中寫出能編譯但空範圍解參考、reference 懸空的答案更重要。
 * 練習完成後，以 sanitizer 與極端值測試重新驗證所有假設。
 */
