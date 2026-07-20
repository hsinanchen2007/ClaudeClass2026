// =============================================================================
//  第 16 課-13：空 vector 的迭代器 —— 半開區間為什麼讓「空」自動安全
// =============================================================================
//
// 【主題資訊 Information】
//   iterator begin() noexcept;   // 空容器時等於 end()
//   iterator end()   noexcept;   // 恆為「最後元素之後」的哨兵，不可解參考
//   bool     empty() const noexcept;   // 等價於 begin() == end()
//   標準保證：對空 vector，begin() == end()；兩者都是合法可比較的值
//   標準版本：C++98 起；C++11 起標明 noexcept；C++20 起 empty() 標 [[nodiscard]]
//   複雜度：全部 O(1)
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 半開區間 [begin, end) 的核心價值：空是自然的邊界情況】
//   STL 用「最後一個元素之後」當結尾，而不是「最後一個元素」。
//   這個選擇帶來一個很優雅的結果：空容器不需要任何特例。
//     begin() == end()  →  for 迴圈的條件一開始就不成立  →  一次都不執行
//   如果 STL 當初選的是閉區間 [first, last]，那空容器要怎麼表達？
//   last 必須指向「begin 之前」，那是不存在的位置——
//   於是每個演算法都得先寫一行 if (empty()) return;。
//   半開區間把這個特例消滅在設計層面，這是 STL 最漂亮的決定之一。
//
// 【2. 半開區間的第二個好處：長度就是相減】
//   end() - begin() 直接就是元素個數，不必 +1。
//   區間 [first, last) 的長度恆為 last - first，
//   而且相鄰區間可以無縫接合：[a,b) + [b,c) = [a,c)，
//   中間那個 b 不會被算兩次也不會漏掉。
//   陣列切割、二分搜尋、分治演算法都因此變得乾淨。
//
// 【3. 空 vector 的 begin()/end() 到底是什麼值】
//   標準只保證兩者相等且可比較，並沒有規定它們一定是 nullptr。
//   一個預設建構的 vector 通常還沒配置任何記憶體，
//   此時內部三根指標（begin/end/capacity_end）多半都是 nullptr，
//   所以 data() 常常回傳 nullptr——但這是實作細節，不是標準保證。
//   你唯一能依賴的是「begin() == end()」這個關係本身。
//
// 【4. 合法 vs 可解參考 —— 兩件不同的事】
//   end() 永遠是「合法的迭代器值」：可以複製、可以比較、可以被 -1。
//   但它「不可解參考」：*v.end() 是 undefined behavior。
//   空容器時 begin() 就等於 end()，所以連 *v.begin() 也不能寫。
//   同理 v.front() / v.back() 對空容器都是 UB——
//   它們內部就是 *begin() 與 *prev(end())，沒有任何檢查。
//
// 【5. 為什麼該用 empty() 而不是 size() == 0】
//   對 vector 兩者都是 O(1)，差別不大。但對 std::list，
//   C++11 之前的某些實作 size() 是 O(n)（要走一遍鏈結串列），
//   而 empty() 恆為 O(1)。養成用 empty() 的習慣，
//   換容器時就不會突然掉進效能陷阱。C++20 起 empty() 還加了
//   [[nodiscard]]，寫成 v.empty(); 這種無效敘述會被警告。
//
// 【概念補充 Concept Deep Dive】
//   ▸ vector 的內部其實只有三根指標
//       T* begin_;          // 資料起點
//       T* end_;            // 最後元素之後（= begin_ + size）
//       T* cap_end_;        // 配置區之後（= begin_ + capacity）
//     於是 size() 就是 end_ - begin_，capacity() 是 cap_end_ - begin_，
//     empty() 是 begin_ == end_。全部 O(1)，而且不需要額外欄位。
//     這也解釋了為什麼 vector 的 sizeof 通常是 24（3 × 8 bytes，
//     x86-64 上的實作定義值，本機實測相符）。
//   ▸ 為什麼範圍 for 對空容器天生安全
//     範圍 for 展開後就是 auto b = v.begin(); auto e = v.end();
//     for (; b != e; ++b)。空容器時 b == e，迴圈體零次執行。
//     不需要任何 empty() 檢查——這正是半開區間設計的直接紅利。
//   ▸ 一個常見的無號回繞陷阱
//     for (size_t i = 0; i <= v.size() - 1; ++i)
//     空容器時 v.size() - 1 是 size_t(0) - 1 = 18446744073709551615，
//     迴圈幾乎無限次執行並立刻越界。
//     用迭代器或範圍 for 就完全不會有這個問題。
//
// 【注意事項 Pay Attention】
//   1. *v.end() 是 UB；空容器時 *v.begin() 同樣是 UB。
//   2. v.front() / v.back() 對空容器是 UB，沒有任何檢查（不像 at() 會丟例外）。
//   3. 空容器的 begin() 不保證是 nullptr，別依賴這件事。
//   4. 判空請用 empty()，不要用 size() == 0，更不要用 size() - 1 做迴圈上界。
//   5. 空 vector 的 begin() 與 end() 可以合法比較、相減（結果為 0）。
//   6. 範圍 for 與所有 STL 演算法對空區間都天然安全，不必額外判空。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】空 vector 與半開區間
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. STL 為什麼採用半開區間 [begin, end) 而不是閉區間 [begin, end]？
//     答：三個理由。(1) 空區間可以自然表達：begin == end，
//         不需要「end 指向 begin 之前」這種不存在的位置；
//         (2) 長度就是 end - begin，不必 +1；
//         (3) 相鄰區間可無縫接合：[a,b) + [b,c) = [a,c)，
//         切割時不會重複或遺漏元素。
//     追問：那 end() 指向的位置有沒有真的配置記憶體？
//         → 不保證有。標準只要求 end() 是個合法可比較的迭代器值，
//           不要求它指向可存取的記憶體，所以絕不能解參考它。
//
// 🔥 Q2. 對一個空的 vector 呼叫 v.front() 會發生什麼事？
//     答：undefined behavior。front() 內部就是 *begin()，
//         而空容器的 begin() == end()，解參考它是 UB。
//         它不會丟例外、也不保證崩潰——可能安靜地讀到垃圾值繼續跑。
//     追問：那 at() 呢？
//         → at(0) 會做範圍檢查並丟出 std::out_of_range，這是它與
//           operator[] 的唯一差別。但 front()/back() 沒有對應的檢查版本。
//
// ⚠️ 陷阱. for (size_t i = 0; i <= v.size() - 1; ++i) 對空 vector
//          為什麼會災難性地出錯？
//     答：size() 回傳無號的 size_type。空容器時 size() - 1 不是 -1，
//         而是無號回繞成 18446744073709551615（64-bit 實測值）。
//         迴圈條件幾乎恆真，第一次迭代就 v[0] 越界存取。
//     為什麼會錯：腦中把 size() 當成有號整數，
//         以為 0 - 1 會得到 -1 然後迴圈不執行。
//         但無號算術沒有負數，減法會回繞到型別最大值。
//         這也是為什麼應該優先用迭代器或範圍 for——
//         半開區間讓空容器根本不需要特別處理。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】批次作業統計：資料可能一筆都沒有
//   情境：每小時跑一次的批次工作，讀進這個小時的訂單金額做統計。
//         離峰時段很可能一筆訂單都沒有——這是正常狀態，不是錯誤。
//   為什麼用本主題：這正是「空容器邊界」最常出事的地方。
//         平均值要除以 size()，空的時候會除以零；
//         最大值用 *max_element() 對空區間解參考是 UB。
//         正確做法是把空當成一等公民先處理掉，
//         而 sum 這種可以有自然單位元的運算則完全不需要判空。
// -----------------------------------------------------------------------------
struct BatchStats {
    bool   hasData = false;
    double sum     = 0.0;
    double average = 0.0;
    double maxValue = 0.0;
};

BatchStats summarize(const std::vector<double>& amounts) {
    BatchStats st;

    // accumulate 對空區間安全：直接回傳初始值 0.0，不必判空
    st.sum = std::accumulate(amounts.begin(), amounts.end(), 0.0);

    if (amounts.empty()) {
        return st;                 // 平均與最大值對空資料沒有定義，維持預設
    }

    st.hasData = true;
    st.average = st.sum / static_cast<double>(amounts.size());
    // max_element 對空區間會回傳 end()，解參考它是 UB → 必須先擋掉空的情況
    st.maxValue = *std::max_element(amounts.begin(), amounts.end());
    return st;
}

void report(const std::string& label, const std::vector<double>& amounts) {
    BatchStats st = summarize(amounts);
    std::cout << label << " 筆數=" << amounts.size()
              << ", 總額=" << st.sum;
    if (st.hasData) {
        std::cout << ", 平均=" << st.average << ", 最大=" << st.maxValue;
    } else {
        std::cout << ", 平均=N/A, 最大=N/A（本時段無資料，屬正常狀態）";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 一、空 vector 的 begin() 等於 end() ===" << std::endl;
    std::vector<int> v;
    std::cout << "begin() == end(): " << (v.begin() == v.end()) << std::endl;
    std::cout << "empty()         : " << v.empty() << std::endl;
    std::cout << "end() - begin() : " << (v.end() - v.begin()) << std::endl;

    std::cout << "\n=== 二、for 迴圈條件一開始就不成立，安全不執行 ===" << std::endl;
    for (auto it = v.begin(); it != v.end(); ++it) {
        std::cout << *it << " ";       // 不會執行
    }
    std::cout << "（傳統 for 迴圈未執行）" << std::endl;

    for (int x : v) {
        std::cout << x << " ";         // 同樣不會執行
        (void)x;
    }
    std::cout << "（範圍 for 也未執行）" << std::endl;

    std::cout << "\n=== 三、STL 演算法對空區間天然安全 ===" << std::endl;
    std::cout << "accumulate 空區間 = "
              << std::accumulate(v.begin(), v.end(), 0) << "（回傳初始值）" << std::endl;
    std::cout << "count 空區間      = "
              << std::count(v.begin(), v.end(), 42) << std::endl;
    std::cout << "find 空區間 == end() ? "
              << (std::find(v.begin(), v.end(), 42) == v.end()) << std::endl;
    std::cout << "max_element 空區間 == end() ? "
              << (std::max_element(v.begin(), v.end()) == v.end())
              << "  ← 回傳 end()，解參考它才是 UB" << std::endl;

    std::cout << "\n=== 四、有元素之後的對照 ===" << std::endl;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    std::cout << "push 三個後 begin() == end(): " << (v.begin() == v.end()) << std::endl;
    std::cout << "end() - begin() : " << (v.end() - v.begin()) << std::endl;
    std::cout << "內容: ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n=== 五、日常實務：批次統計要能處理「零筆資料」 ===" << std::endl;
    report("02:00 離峰時段:", {});
    report("12:00 尖峰時段:", {1200.0, 350.5, 980.0, 45.25, 2100.0});
    report("03:00 單筆訂單:", {88.8});

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作13.cpp" -o empty_iter
//
// 【本檔未附 LeetCode 範例的理由】
//   本檔的主題是「空區間為什麼不需要特例」這個設計原理。
//   LeetCode 題目的約束條件通常直接寫明 1 <= nums.length，
//   空輸入根本不在測資範圍內，強行套一題只會把重點模糊掉。
//   真正會被空容器咬到的是後端批次作業、報表統計這類
//   「資料量本來就可能為零」的情境，因此改以上面的批次統計範例呈現。

// === 預期輸出 ===
// === 一、空 vector 的 begin() 等於 end() ===
// begin() == end(): true
// empty()         : true
// end() - begin() : 0
//
// === 二、for 迴圈條件一開始就不成立，安全不執行 ===
// （傳統 for 迴圈未執行）
// （範圍 for 也未執行）
//
// === 三、STL 演算法對空區間天然安全 ===
// accumulate 空區間 = 0（回傳初始值）
// count 空區間      = 0
// find 空區間 == end() ? true
// max_element 空區間 == end() ? true  ← 回傳 end()，解參考它才是 UB
//
// === 四、有元素之後的對照 ===
// push 三個後 begin() == end(): false
// end() - begin() : 3
// 內容: 10 20 30
//
// === 五、日常實務：批次統計要能處理「零筆資料」 ===
// 02:00 離峰時段: 筆數=0, 總額=0, 平均=N/A, 最大=N/A（本時段無資料，屬正常狀態）
// 12:00 尖峰時段: 筆數=5, 總額=4675.75, 平均=935.15, 最大=2100
// 03:00 單筆訂單: 筆數=1, 總額=88.8, 平均=88.8, 最大=88.8
