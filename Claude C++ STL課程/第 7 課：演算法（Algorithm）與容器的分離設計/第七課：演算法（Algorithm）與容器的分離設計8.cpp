// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計8.cpp
//    —  填值家族：fill / fill_n / generate / generate_n
// =============================================================================
//
// 【主題資訊 Information】
//   void  fill      (FwdIt f, FwdIt l, const T& value);          // C++98
//   OutIt fill_n    (OutIt f, Size n,  const T& value);          // C++98（C++11 起回傳 OutIt）
//   void  generate  (FwdIt f, FwdIt l, Generator g);             // C++98
//   OutIt generate_n(OutIt f, Size n,  Generator g);             // C++98（C++11 起回傳 OutIt）
//
//   標準版本：全部 C++98；**fill_n / generate_n 在 C++11 之前回傳 void**，
//             C++11 起才回傳「寫入結束位置」
//   迭代器需求：fill / generate 要 Forward Iterator；
//               fill_n / generate_n 只要 Output Iterator（不需要 last）
//   複雜度：fill/generate 為 O(last - first)；_n 版本為 O(n)
//   標頭檔：<algorithm>
//
// 【詳細解釋 Explanation】
//
// 【1. fill 與 generate 的分工：常數 vs 每次都算】
//     fill     → 每個位置寫入**同一個值**（值只求值一次）
//     generate → 每個位置呼叫一次 g()，寫入**它的回傳值**
// 換句話說，generate 是 fill 的「動態版」。要填入序號、亂數、時間戳、
// 或任何「每格都不一樣」的東西就用 generate。
// 注意 generate 的 g 是**無參數**的（nullary）：它不知道自己在填第幾格，
// 也拿不到原本的值。若需要知道位置或原值，那不是 generate 的工作——
// 用 iota（填連續值）或 transform（依原值產生新值）。
//
// 【2. 為什麼會有 _n 版本：因為有些輸出根本沒有 last】
// fill(first, last, v) 需要知道結束位置；fill_n(first, n, v) 只需要個數。
// 這個差異看起來只是方便，實際上是**能力差異**：
// 對 std::back_inserter(vec) 這種 insert iterator，根本**不存在 last**
// （它可以無限寫下去）。所以：
//     std::fill_n(std::back_inserter(v), 5, 0);   // ✓ 合法，append 5 個 0
//     std::fill(std::back_inserter(v), ???, 0);   // ✗ 沒有 last 可以給
// 這也是為什麼 _n 版本只需要 Output Iterator，而非 _n 版本需要 Forward Iterator。
// 同樣的邏輯適用於 copy_n（見本課第 6 個檔案）。
//
// 【3. generate 的 g 有狀態時，順序保證是什麼？】
// 本檔的 Fibonacci 範例，lambda 捕獲了 a、b 兩個變數，**依賴呼叫順序**。
// 標準規定 generate / generate_n 對範圍中的每個位置**依序**呼叫 g 恰好一次，
// 所以這個寫法是安全的。這與 transform 不同——transform 不保證套用順序。
// 但要注意：**加上執行策略的平行版本就不再有順序保證**，
// 有狀態的產生器在平行版本下會產生競爭條件。
//
// 【4. fill 對 vector<bool> 的特殊性】
// std::vector<bool> 是位元壓縮的特化版本，它的 iterator 解參考回傳的是
// **代理物件（proxy）**而非真正的 bool&。fill 仍然可用（賦值透過 proxy 轉發），
// 但這類特化在泛型程式碼中常帶來意外，是 vector<bool> 惡名的來源之一。
//
// 【概念補充 Concept Deep Dive】
//
// (A) fill 對 trivially copyable 型別會退化成 memset
//   libstdc++ 對 char / unsigned char 這類單位元組型別、且 iterator 為連續
//   記憶體時，會把 fill 特化成一次 memset 呼叫。對 int 填 0 也可能走 memset
//   （因為全 0 的位元樣式相同）。**這是實作最佳化，不是標準保證**，
//   但主流實作都有做，所以不必為了效能改寫成手動 memset。
//
// (B) 為什麼 fill 的 value 是 const T&，且只求值一次
//   簽名是 const T& value，整個範圍共用同一個引用，不會對每個位置重新求值。
//   所以寫 std::fill(v.begin(), v.end(), rand()) **只會產生一個亂數**，
//   全部填成同一個值。要每格不同必須用 generate。這是最常見的誤用。
//
// (C) 初始化 vector 其實不太需要 fill
//   std::vector<int> v(5, 42);        // 建構時就填好，比先建再 fill 快
//   v.assign(10, 0);                  // 重設大小並填值
//   v.resize(n, 0);                   // 只有新增的部分填 0
//   fill 的真正用途是「**重設一個已存在的緩衝區**」——
//   例如每輪迴圈開始前清空累加器，避免重新配置記憶體。
//
// 【注意事項 Pay Attention】
// 1. **fill 的值只求值一次**：std::fill(..., rand()) 全部會是同一個數。
//    要每格不同請用 generate。
// 2. fill / generate 需要 Forward Iterator（要能重複走訪）；
//    fill_n / generate_n 只要 Output Iterator，因此可搭配 back_inserter。
// 3. **_n 版本不檢查目的地容量**：n 大於剩餘空間就是緩衝區溢位（未定義行為）。
// 4. generate 的 g 是無參數的，拿不到位置也拿不到原值；
//    要連續值用 std::iota，要依原值轉換用 std::transform。
// 5. generate 保證依序呼叫，所以有狀態的產生器可用；
//    但**平行版本不保證順序**。
// 6. fill_n / generate_n 的回傳值是 C++11 才加的（之前回傳 void）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】填值與產生家族
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::fill(v.begin(), v.end(), rand()) 會發生什麼事？
//     答：整個 vector 會被填成**同一個亂數**。因為 fill 的第三個參數是
//         const T& value，rand() 在呼叫 fill **之前**就已經求值完成，
//         整個範圍共用那一個值。要每格都不同必須用
//         std::generate(v.begin(), v.end(), [](){ return rand(); })，
//         generate 才會對每個位置各呼叫一次產生器。
//     追問：那 generate 保證呼叫順序嗎？→ 保證，標準規定依序呼叫恰好一次，
//         所以有狀態的產生器（如 Fibonacci）可以正常運作；
//         但加上執行策略的平行版本就不保證了。
//
// 🔥 Q2. 為什麼有了 fill 還要 fill_n？只是為了少寫一個參數嗎？
//     答：不只是方便，是**能力差異**。fill 需要 last，但 insert iterator
//         （如 std::back_inserter(v)）根本不存在 last，它可以無限寫下去。
//         fill_n 只需要起點與個數，因此能搭配 back_inserter 往容器尾端追加，
//         fill 做不到。這也反映在迭代器需求上：fill 要 Forward Iterator，
//         fill_n 只要 Output Iterator。
//     追問：copy_n 也是同樣的道理嗎？→ 是，_n 系列的共同動機都是
//         「輸出端沒有可用的終點」。
//
// ⚠️ 陷阱. std::fill_n(v.begin(), 100, 0); 其中 v.size() == 10，會怎樣？
//     答：越界寫入 90 個元素，未定義行為。fill_n 完全不檢查目的地容量——
//         它只拿到起點和個數，和 copy 一樣沒有能力查詢剩餘空間。
//     為什麼會錯：因為 fill(v.begin(), v.end(), 0) 用起來很安全
//         （範圍由容器自己給），就以為 _n 版本也一樣安全。
//         實際上 _n 版本把長度控制權交給了呼叫端，安全網也一併移除了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <iterator>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】監控系統的環形統計緩衝區重設
//   情境：監控程式每分鐘統計一次各 HTTP 狀態碼的次數，每小時結算後
//         必須把計數陣列歸零重新開始。這裡不重新配置記憶體，只重設內容——
//         這正是 fill 的典型用途（不是初始化，是「重設既有緩衝區」）。
//   另外示範用 generate_n + back_inserter 產生連續的分鐘標籤，
//   這是 fill 做不到的（目的地沒有 last）。
// -----------------------------------------------------------------------------
struct MinuteStats {
    std::vector<int> counters;      // 索引 0..4 對應 2xx,3xx,4xx,5xx,other

    MinuteStats() : counters(5, 0) {}

    void record(int bucket) { ++counters[bucket]; }

    void print(const std::string& label) const {
        std::cout << "  " << label << ": ";
        for (int c : counters) std::cout << c << " ";
        std::cout << std::endl;
    }

    void resetHour() {
        // 重設既有緩衝區，不重新配置（保留 capacity）
        std::fill(counters.begin(), counters.end(), 0);
    }
};

int main() {
    // fill：用固定值填充, 這裡將 vec 的所有元素填充為 42
    std::cout << "=== fill ===" << std::endl;
    std::vector<int> vec(5);
    std::fill(vec.begin(), vec.end(), 42);
    std::cout << "fill 42: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // fill_n：填充前 n 個, 這裡將 vec 的前 3 個元素填充為 100
    // ★ fill_n 不檢查容量：n 超過剩餘空間就是未定義行為
    std::cout << "\n=== fill_n ===" << std::endl;
    std::fill_n(vec.begin(), 3, 100);
    std::cout << "fill_n 前 3 個為 100: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // generate：用函數生成值, 這裡使用 lambda 表達式生成遞增的數字乘以 10
    // ★ 與 fill 的關鍵差別：generate 對每個位置各呼叫一次 g()
    std::cout << "\n=== generate ===" << std::endl;
    int counter = 0;
    std::generate(vec.begin(), vec.end(), [&counter]() {
        return ++counter * 10;
    });
    std::cout << "generate: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // generate_n：生成前 n 個, 這裡使用 lambda 表達式生成 Fibonacci 數列的前 10 個數字
    // ★ 有狀態的產生器之所以安全，是因為標準保證 generate 依序呼叫恰好一次
    std::cout << "\n=== generate_n ===" << std::endl;
    std::vector<int> fibs(10);
    int a = 0, b = 1;
    std::generate_n(fibs.begin(), 10, [&a, &b]() {
        int result = a;
        int temp = a + b;
        a = b;
        b = temp;
        return result;
    });
    std::cout << "Fibonacci: ";
    for (int n : fibs) std::cout << n << " ";
    std::cout << std::endl;

    // ★ fill 的值只求值一次 —— 這是最常見的誤用
    std::cout << "\n=== fill 的值只求值一次（用可預測的計數器示範）===" << std::endl;
    int callCount = 0;
    auto nextValue = [&callCount]() { return ++callCount; };
    std::vector<int> byFill(5);
    std::fill(byFill.begin(), byFill.end(), nextValue());   // nextValue() 先算完才傳進去
    std::cout << "fill(..., nextValue()):     ";
    for (int n : byFill) std::cout << n << " ";
    std::cout << "  ← 全部相同，只呼叫了 1 次" << std::endl;

    callCount = 0;
    std::vector<int> byGenerate(5);
    std::generate(byGenerate.begin(), byGenerate.end(), nextValue);  // 傳的是函式本身
    std::cout << "generate(..., nextValue):   ";
    for (int n : byGenerate) std::cout << n << " ";
    std::cout << "  ← 每格各呼叫一次" << std::endl;

    // ★ _n 版本可以搭配 back_inserter；非 _n 版本不行（沒有 last）
    std::cout << "\n=== _n 版本才能搭配 back_inserter ===" << std::endl;
    std::vector<int> growing = {7, 8};
    std::fill_n(std::back_inserter(growing), 3, 0);   // 往尾端追加 3 個 0
    std::cout << "fill_n + back_inserter 追加 3 個 0: ";
    for (int n : growing) std::cout << n << " ";
    std::cout << std::endl;
    // std::fill(std::back_inserter(growing), ???, 0);  // 不可能：沒有 last 可給

    std::cout << "\n=== 日常實務：每小時重設監控計數器 ===" << std::endl;
    MinuteStats stats;
    stats.record(0); stats.record(0); stats.record(2); stats.record(3);
    stats.print("結算前");
    stats.resetHour();
    stats.print("結算後");
    std::cout << "  (fill 只改內容不動 capacity，避免每小時重新配置記憶體)" << std::endl;

    // generate_n + back_inserter 產生分鐘標籤（大小未知的輸出）
    std::vector<int> minuteMarks;
    int m = 0;
    std::generate_n(std::back_inserter(minuteMarks), 6, [&m]() { return m += 10; });
    std::cout << "  取樣分鐘標記: ";
    for (int n : minuteMarks) std::cout << n << " ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計8.cpp -o demo8

// === 預期輸出 ===
// === fill ===
// fill 42: 42 42 42 42 42
//
// === fill_n ===
// fill_n 前 3 個為 100: 100 100 100 42 42
//
// === generate ===
// generate: 10 20 30 40 50
//
// === generate_n ===
// Fibonacci: 0 1 1 2 3 5 8 13 21 34
//
// === fill 的值只求值一次（用可預測的計數器示範）===
// fill(..., nextValue()):     1 1 1 1 1   ← 全部相同，只呼叫了 1 次
// generate(..., nextValue):   1 2 3 4 5   ← 每格各呼叫一次
//
// === _n 版本才能搭配 back_inserter ===
// fill_n + back_inserter 追加 3 個 0: 7 8 0 0 0
//
// === 日常實務：每小時重設監控計數器 ===
//   結算前: 2 0 1 1 0
//   結算後: 0 0 0 0 0
//   (fill 只改內容不動 capacity，避免每小時重新配置記憶體)
//   取樣分鐘標記: 10 20 30 40 50 60
