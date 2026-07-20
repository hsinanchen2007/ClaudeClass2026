// =============================================================================
//  第 19 課：vector 與原始陣列的互操作 8  —  用「指標區間」建構 vector（部分複製）
// =============================================================================
//
// 【主題資訊 Information】
//   template <class InputIt>
//   vector(InputIt first, InputIt last,
//          const Allocator& alloc = Allocator());          // 區間建構子（C++98 起）
//
//   標頭檔：<vector>
//   複雜度：對 forward iterator 以上為 O(last - first)，且只配置一次記憶體
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「原始指標」可以當成 iterator 用】
//   STL 的演算法與容器並不要求你傳入某個 class 型別的 iterator，它們要求的是
//   「符合 iterator 需求的東西」——能 *it 取值、++it 前進、it1 != it2 比較。
//   內建指標 T* 天生就滿足 random access iterator 的全部要求，所以
//       std::vector<int> v(arr + 2, arr + 6);
//   完全合法。這是 STL 泛型設計最漂亮的一個成果：C 語言的指標不需要任何包裝，
//   就直接是 STL 生態系的一等公民。C++17 之前靠 std::iterator_traits<T*> 的
//   偏特化提供 value_type/difference_type，C++20 之後則由 concept 直接描述。
//
// 【2. 半開區間 [first, last) 的意義】
//   arr + 2 是「起點」，arr + 6 是「終點的下一個位置」，所以取到的是索引 2、3、4、5
//   共 4 個元素，而不是 5 個。半開區間的三個好處：
//     (a) 元素個數 = last - first，不需要 +1，不會有 off-by-one。
//     (b) 空區間自然表示成 first == last，不需要特例。
//     (c) 可以連續切分：[a,b) 與 [b,c) 接起來剛好是 [a,c)，中間不重疊不遺漏。
//
// 【3. 為什麼是「複製」而不是「參照」】
//   區間建構子會把 [first, last) 的內容逐一複製到 vector 自己配置的新記憶體。
//   完成後 vector 與原陣列是兩塊完全獨立的記憶體，改動 v 不會影響 arr。
//   如果你要的是「不複製、只觀察」，C++20 的 std::span<int> 才是正確工具。
//
// 【4. 只配置一次：iterator category 的威力】
//   對 forward iterator 以上（指標屬於 random access），實作可以先用
//   std::distance(first, last) 算出元素個數，一次配置好記憶體再逐一複製。
//   若傳入的是 input iterator（例如 istream_iterator），事先算不出長度，
//   只能邊讀邊 push_back，於是會發生多次重新配置。同一個建構子、
//   兩種完全不同的效能表現，差別就在 iterator category 的 tag dispatch。
//
// 【概念補充 Concept Deep Dive】
//   ● 記憶體佈局
//       arr:  [10][20][30][40][50][60][70][80]   ← 堆疊上的連續 8 個 int
//                      ↑first        ↑last
//       v:    heap 上另一塊 4 個 int 的空間 [30][40][50][60]
//     v 的 size()==4；capacity() 在本機 libstdc++ 實測也是 4（剛好配置所需大小，
//     不做額外預留）——這是實作定義的行為，不是標準保證。
//
//   ● 越界的界線在哪裡
//       arr + 8 是合法的「尾後指標」，可以拿來比較與相減，但不可解參考。
//       arr + 9 連「形成」這個指標本身就是 UB，即使你完全不去讀它。
//
//   ● 為什麼不寫 v.assign(arr+2, arr+6)
//       建構子適用於「物件還不存在」的場合，assign 適用於「物件已存在、要換內容」。
//       建構子少一次「先預設建構再丟棄」的成本，語意上也更精確。
//
// 【注意事項 Pay Attention】
//   1. first 與 last 必須指向「同一個陣列物件」，否則兩者相減是 UB，
//      即使兩塊記憶體在位址上剛好相鄰也一樣。
//   2. last 必須可以從 first 往前走到達；寫反成 (arr+6, arr+2) 是 UB，
//      不可依賴任何特定結果（不保證崩潰，也不保證任何固定輸出）。
//   3. 對於 std::vector<int> v(5, 0) 這種「兩個同型別整數」的呼叫，
//      標準有特別規定：當型別不是 iterator 時要退回 (size_type, const T&) 版本。
//   4. 陣列一旦退化成指標（傳進函式）就失去長度資訊，此時 std::size(arr) 會編不過，
//      必須另外傳長度。這正是原始陣列最大的弱點。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】用指標區間建構 vector
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector<int> v(arr + 2, arr + 6) 會得到幾個元素？為什麼？
//     答：4 個（索引 2、3、4、5）。STL 一律使用半開區間 [first, last)，
//         last 指向「最後一個元素的下一格」，不包含在內。
//         元素個數直接等於 last - first，不需要 +1。
//     追問：那 arr + 8（陣列尾後）合法嗎？→ 合法，可比較、可相減，但不可解參考；
//         arr + 9 則連形成該指標本身就是 UB。
//
// 🔥 Q2. 為什麼原始指標可以直接當 iterator 傳給 STL？
//     答：STL 對 iterator 的要求是行為（可解參考、可遞增、可比較），不是繼承某個基底類別。
//         內建指標天生滿足 random access iterator 的全部要求，
//         再透過 std::iterator_traits<T*> 的偏特化取得 value_type 等型別資訊。
//     追問：那 vector 的 iterator 一定是原始指標嗎？→ 不一定。libstdc++ 在
//         _GLIBCXX_DEBUG 模式下會用一個 class 包裝以便做邊界檢查，
//         所以不可以假設 v.begin() 能隱式轉成 int*，要取指標請用 v.data()。
//
// ⚠️ 陷阱. std::vector<int> v(5, 0) 和 std::vector<int> v(arr, arr + 5)
//         都是「兩個參數」，編譯器怎麼分辨要建 5 個 0、還是複製一段區間？
//     答：靠重載決議加上標準的特別規定。當兩個參數都是整數型別、不可能是 iterator 時，
//         標準要求實作必須選擇 (size_type count, const T& value) 版本。
//         實作端典型做法是用 tag dispatch 或 SFINAE 判斷型別是否為 integral。
//     為什麼會錯：多數人以為「模板更泛化就更優先」，於是預期 v(5, 0) 會
//         把 5 和 0 當成 iterator 而編譯失敗。實際上這個歧義在 C++98 就被發現，
//         標準明文規定必須退回 count/value 版本，這也是 std::vector<int> v{5, 0}
//         （大括號，得到 {5,0} 兩個元素）和 v(5, 0)（得到 5 個 0）差這麼多的原因。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <iterator>   // std::begin / std::end / std::size

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：已排序陣列就地去重，回傳新長度 k，且前 k 個元素必須是去重後的結果。
//   為什麼用到本主題：判題器只看「前 k 個」，這正是半開區間 [begin, begin + k)
//     的概念。下面 main() 會用區間建構子把這一段切成獨立 vector，
//     示範「以迭代器／指標區間建構」在真實題目裡怎麼用。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    if (nums.empty()) return 0;
    std::size_t k = 1;                       // [0, k) 是已確定去重的區段
    for (std::size_t i = 1; i < nums.size(); ++i) {
        if (nums[i] != nums[k - 1]) {
            nums[k] = nums[i];
            ++k;
        }
    }
    return static_cast<int>(k);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】從固定大小的感測器取樣緩衝區，切出「有效樣本」那一段
//   情境：韌體 / 驅動程式常見的介面是「給我一塊固定大小的陣列，我填多少回報多少」。
//         緩衝區宣告成 256 格，但這次只填了 5 格；後面 251 格是舊資料。
//         我們必須只取 [buf, buf + n) 這一段，絕不能整塊 256 格拿去用。
// -----------------------------------------------------------------------------
int read_sensor_samples(double* buf, int cap) {
    static const double kFakeReading[] = {23.5, 23.7, 24.1, 23.9, 24.4};
    const int n = static_cast<int>(std::size(kFakeReading));
    if (cap < n) return -1;                  // 緩衝區太小
    for (int i = 0; i < n; ++i) buf[i] = kFakeReading[i];
    return n;                                // 回報實際寫入筆數
}

int main() {
    std::cout << "=== 1. 區間建構：只複製索引 2~5 ===\n";
    int arr[] = {10, 20, 30, 40, 50, 60, 70, 80};

    // 只複製索引 2~5 的元素（30, 40, 50, 60）
    // 半開區間：arr+2 是起點（含），arr+6 是終點（不含）→ 6-2 = 4 個元素
    std::vector<int> v(arr + 2, arr + 6);

    std::cout << "原始陣列 arr : ";
    for (int x : arr) std::cout << x << " ";
    std::cout << "(共 " << std::size(arr) << " 個)\n";

    std::cout << "部分複製 v   : ";
    for (int x : v) std::cout << x << " ";
    std::cout << "(共 " << v.size() << " 個)\n";
    std::cout << "v.size()     = " << v.size() << "\n";
    std::cout << "v.capacity() = " << v.capacity()
              << "（本機 libstdc++ 剛好配置所需大小，非標準保證）\n";

    std::cout << "\n=== 2. 證明是「複製」而非「共用記憶體」 ===\n";
    v[0] = 999;
    std::cout << "改 v[0] = 999 後，arr[2] 仍是 " << arr[2]
              << "（兩塊記憶體互不相干）\n";

    std::cout << "\n=== 3. 全複製 vs 空區間 ===\n";
    std::vector<int> all(std::begin(arr), std::end(arr));
    std::vector<int> none(arr + 3, arr + 3);   // first == last → 合法的空區間
    std::cout << "全複製 size = " << all.size() << "\n";
    std::cout << "空區間 size = " << none.size()
              << "、empty() = " << std::boolalpha << none.empty() << "\n";

    std::cout << "\n=== 4. 易混淆：() 與 {} 的差別 ===\n";
    std::vector<int> byCount(5, 0);    // 5 個 0
    std::vector<int> byList{5, 0};     // 兩個元素：5 和 0
    std::cout << "vector<int> v(5, 0) → size=" << byCount.size() << " 內容：";
    for (int x : byCount) std::cout << x << " ";
    std::cout << "\nvector<int> v{5, 0} → size=" << byList.size() << " 內容：";
    for (int x : byList) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== LeetCode 26. Remove Duplicates from Sorted Array ===\n";
    std::vector<int> nums = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    int k = removeDuplicates(nums);
    std::cout << "回傳 k = " << k << "\n";
    // 用區間建構子把「有效的前 k 個」切成獨立的 vector——本課主題的實際應用
    std::vector<int> answer(nums.begin(), nums.begin() + k);
    std::cout << "前 k 個元素：";
    for (int x : answer) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "\n=== 日常實務：只取緩衝區的有效樣本 ===\n";
    std::vector<double> buffer(256, -1.0);          // 固定大小緩衝區
    int n = read_sensor_samples(buffer.data(), static_cast<int>(buffer.size()));
    std::cout << "緩衝區容量 = " << buffer.size() << "，本次實際寫入 = " << n << "\n";
    if (n > 0) {
        // 關鍵：只取 [begin, begin + n)，不是整塊 256 格
        std::vector<double> samples(buffer.begin(), buffer.begin() + n);
        std::cout << "有效樣本：";
        for (double d : samples) std::cout << d << " ";
        std::cout << "(共 " << samples.size() << " 筆)\n";
        std::cout << "若誤用整塊緩衝區，會多讀到 "
                  << buffer.size() - samples.size() << " 筆殘留值 -1\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：vector 與原始陣列的互操作8.cpp" -o demo8

// === 預期輸出 ===
// === 1. 區間建構：只複製索引 2~5 ===
// 原始陣列 arr : 10 20 30 40 50 60 70 80 (共 8 個)
// 部分複製 v   : 30 40 50 60 (共 4 個)
// v.size()     = 4
// v.capacity() = 4（本機 libstdc++ 剛好配置所需大小，非標準保證）
//
// === 2. 證明是「複製」而非「共用記憶體」 ===
// 改 v[0] = 999 後，arr[2] 仍是 30（兩塊記憶體互不相干）
//
// === 3. 全複製 vs 空區間 ===
// 全複製 size = 8
// 空區間 size = 0、empty() = true
//
// === 4. 易混淆：() 與 {} 的差別 ===
// vector<int> v(5, 0) → size=5 內容：0 0 0 0 0
// vector<int> v{5, 0} → size=2 內容：5 0
//
// === LeetCode 26. Remove Duplicates from Sorted Array ===
// 回傳 k = 5
// 前 k 個元素：0 1 2 3 4
//
// === 日常實務：只取緩衝區的有效樣本 ===
// 緩衝區容量 = 256，本次實際寫入 = 5
// 有效樣本：23.5 23.7 24.1 23.9 24.4 (共 5 筆)
// 若誤用整塊緩衝區，會多讀到 251 筆殘留值 -1
