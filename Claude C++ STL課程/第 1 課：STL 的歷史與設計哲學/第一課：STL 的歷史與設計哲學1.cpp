// =============================================================================
//  第一課：STL 的歷史與設計哲學 1  —  STL 三大元件的第一次見面
// =============================================================================
//
// 【主題資訊 Information】
//   本檔示範 STL 的三個核心元件如何協同工作：
//     容器   std::vector<T>            <vector>      連續記憶體的動態陣列
//     演算法 std::sort(first,last)     <algorithm>   O(N log N)，IntroSort
//     演算法 std::find(first,last,val) <algorithm>   O(N)，線性掃描
//   標準版本：三者 C++98 即有；本檔用來接 iterator 的 `auto` 是 C++11。
//   回傳型別：std::find 回傳 iterator，找不到時回傳 last（本例即 v.end()）。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼這 20 行是整個 STL 的縮影】
//   這段程式碼沒有出現任何一個 new / delete / malloc / free，也沒有一個手寫
//   迴圈去做排序或搜尋。但它做完了在 C 語言中要寫五十行的三件事：
//     ① 建立一個會自動成長、離開作用域自動釋放的動態陣列
//     ② 排序
//     ③ 搜尋
//   關鍵在於這三件事「彼此不認識」。std::sort 不知道 vector 是什麼，
//   std::find 也不知道。它們只知道「有一對 iterator」。
//
// 【2. begin() / end() 為什麼是半開區間 [begin, end)】
//   end() 指向「最後一個元素的再下一個位置」，不是最後一個元素。
//   這個設計讓三件事變得非常漂亮：
//     ① 區間長度 = end - begin，不必 +1，天然不會 off-by-one。
//     ② 空區間的表示法很自然：begin == end。若改用閉區間 [first, last]，
//        空區間得寫成 last < first，型別上很彆扭（可能要指到越界一格）。
//     ③ 「找不到」有現成的哨兵值：回傳 end() 即可，不必額外發明常數
//        （對比 std::string::find 就必須發明 npos）。
//
// 【3. 為什麼 std::find 回傳 iterator 而不是 index】
//   index 這個概念只對「可隨機存取」的容器有意義。std::list 是雙向鏈結串列，
//   它沒有「第 3 個元素的位址算式」，但它有 iterator。
//   回傳 iterator，同一個 std::find 就能同時服務 vector、list、set、deque、
//   原生陣列、甚至 istream。這就是正交性（orthogonality）的代價與收益：
//   呼叫端要多寫一次 `it != v.end()`，換來演算法完全不必為容器改寫。
//
// 【4. 判斷「有沒有找到」只有一種正確寫法】
//     if (it != v.end())        ← 唯一正確
//   不可以寫 if (it)：iterator 不是指標（對 vector 剛好常是，對 list 不是），
//   也不保證能轉成 bool。也不可以先 *it 再判斷：對 end() 解參考是 UB。
//
// 【概念補充 Concept Deep Dive】
//
// (A) sort 要求「隨機存取 iterator」，find 不用
//     std::sort 內部是 IntroSort（QuickSort + HeapSort + InsertionSort 混合），
//     需要 `it + n`、`it2 - it1` 這類跳躍運算，所以只接受 random access
//     iterator。這正是 std::list 不能用 std::sort 的原因（list 只有
//     bidirectional iterator），list 必須用成員函式 list::sort。
//     std::find 只需要「往前走一格」，連 input iterator 都能用。
//     這套 iterator 分級，就是 STL 用來在編譯期擋掉錯誤組合的機制。
//
// (B) 這段程式碼幾乎沒有執行期抽象成本
//     std::sort 的比較子在編譯期就被具體化（template 展開），編譯器看得到
//     `a < b` 的實際內容，可以 inline。C 的 qsort 收的是
//     `int(*)(const void*, const void*)` 函式指標，每比較一次就是一次無法
//     inline 的間接呼叫，還要透過 memcpy 搬資料。
//     這是「泛型不犧牲效能」最具體的證據。
//
// (C) vector 在本例的記憶體行為（libstdc++，實作定義）
//     `{5, 2, 8, 1, 9}` 走的是 initializer_list 建構子，長度已知，
//     所以只配置一次，capacity 剛好等於 5，不會經歷成長。
//     若改成連續 push_back 五次，libstdc++ 的容量序列會是 1→2→4→8
//     （成長倍率 2×，實作定義；MSVC 是 1.5×。標準只要求攤還 O(1)）。
//
// 【注意事項 Pay Attention】
//   1. std::find 是線性掃描 O(N)。這裡雖然已經 sort 過，find 也不會變快——
//      它根本不知道資料有序。有序資料要用 std::binary_search /
//      std::lower_bound（O(log N)）。這是最常見的誤解之一。
//   2. 對 v.end() 解參考是未定義行為：不保證崩潰，也可能靜默讀到垃圾值。
//   3. sort 之後，原本的 iterator 指向的「位置」仍有效，但那個位置上的
//      「值」已經被換掉。sort 不會使 iterator 失效，卻會使它們失去意義。
//   4. std::find 用 operator== 比較，std::sort 用 operator<。自訂型別要提供
//      對應運算子，或改用帶比較子的重載版本。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】STL 三大元件與 iterator 慣用法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::find 找不到時回傳什麼？為什麼不回傳 -1 或 nullptr？
//     答：回傳傳進去的 last（本例是 v.end()）。因為 find 是泛型演算法，
//         它只拿到 [first, last) 這對 iterator，根本不知道容器是誰、
//         有沒有 index 這個概念。回傳 last 是唯一「一定存在、一定合法、
//         一定可比較」的哨兵值，而且不需要額外發明常數。
//     追問：那 std::string::find 為什麼就發明了 npos？
//         → 因為它是成員函式、回傳 size_type（index），
//           index 空間裡沒有天然的界外值，只好指定最大值當哨兵。
//
// 🔥 Q2. std::list 為什麼不能用 std::sort？
//     答：std::sort 要求 random access iterator（需要 it+n、it2-it1），
//         而 list 只提供 bidirectional iterator，編譯期就會被擋下來。
//         list 必須用成員函式 list::sort，那是對節點做指標接合的 merge sort。
//     追問：那為什麼 list::sort 不會使 iterator 失效？
//         → 因為它搬的是節點之間的連結，節點本身沒有移動，
//           iterator 指向的還是同一個節點。
//
// ⚠️ 陷阱. 這段程式碼已經先 sort 過了，所以後面的 std::find 是 O(log N)——對嗎？
//     答：錯。std::find 永遠是 O(N) 線性掃描。它拿到的只是一對 iterator，
//         標準沒有、也不可能讓它知道區間是否有序。要吃到有序的好處，
//         必須明確改用 std::binary_search（回傳 bool）或
//         std::lower_bound（回傳位置），那才是 O(log N)。
//     為什麼會錯：把「資料的狀態」和「演算法的知識」混為一談。
//         人腦看得到資料已排序，但 find 的函式簽章裡沒有任何一個位置
//         能承載「這段區間有序」這個資訊，編譯器自然也無從最佳化。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 217. Contains Duplicate
//   題目：給定整數陣列，若有任何值出現超過一次回傳 true，否則回傳 false。
//   為什麼用到本主題：這題正是「不用自己寫排序、不用自己寫搜尋」的最小範例——
//     先 std::sort 把相同的值排到相鄰，再用 std::adjacent_find 找相鄰重複。
//     兩個 STL 演算法接在一起，中間完全靠 iterator 溝通。
//   複雜度：O(N log N) 時間（排序主導）、O(1) 額外空間（就地排序）。
// -----------------------------------------------------------------------------
bool containsDuplicate(std::vector<int> nums) {   // 傳值：允許就地排序，不動呼叫端
    std::sort(nums.begin(), nums.end());
    return std::adjacent_find(nums.begin(), nums.end()) != nums.end();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】防火牆允許清單（allow-list）比對
//   情境：服務啟動時載入一份「允許連入的埠號」清單，每個連線進來時檢查。
//   為什麼用到本主題：這就是 std::find 的原型用途——在一個沒有特別結構的
//     序列裡問「這個值在不在裡面」，並用 `it != end()` 判斷結果。
//   實務提醒：清單若很大且查詢頻繁，應改用 std::unordered_set（平均 O(1)）；
//     這裡是小清單（通常十來個埠），線性掃描反而 cache 友善、實際更快。
// -----------------------------------------------------------------------------
bool isPortAllowed(const std::vector<int>& allowList, int port) {
    auto it = std::find(allowList.begin(), allowList.end(), port);
    return it != allowList.end();
}

int main() {
    std::cout << std::boolalpha;

    std::cout << "=== 原始示範：容器 + 排序 + 搜尋 ===\n";

    // 不用自己寫動態陣列 - 用 vector
    std::vector<int> numbers = {5, 2, 8, 1, 9};

    // 不用自己寫排序 - 用 sort
    std::sort(numbers.begin(), numbers.end());

    std::cout << "排序後: ";
    for (int n : numbers) std::cout << n << " ";
    std::cout << "\n";

    // 不用自己寫搜尋 - 用 find
    auto it = std::find(numbers.begin(), numbers.end(), 8);

    if (it != numbers.end()) {
        std::cout << "找到了: " << *it << std::endl;
        // it 是 iterator；減去 begin() 才得到 index（只有隨機存取容器能這樣算）
        std::cout << "它的索引是: " << (it - numbers.begin()) << "\n";
    }

    std::cout << "\n=== 找不到的情況 ===\n";
    auto miss = std::find(numbers.begin(), numbers.end(), 100);
    std::cout << "找 100 的結果: "
              << (miss == numbers.end() ? "沒找到（it == end()）" : "找到了") << "\n";

    std::cout << "\n=== 對比：find 是線性，binary_search 才吃到有序 ===\n";
    // 資料已排序，這時才有資格用 O(log N) 的版本
    std::cout << "std::binary_search(8) = "
              << std::binary_search(numbers.begin(), numbers.end(), 8) << "\n";
    std::cout << "std::binary_search(7) = "
              << std::binary_search(numbers.begin(), numbers.end(), 7) << "\n";

    std::cout << "\n=== 同一個 find 也能用在原生陣列上（正交性）===\n";
    int raw[] = {11, 22, 33, 44};
    // 原生陣列的「iterator」就是指標，STL 演算法照吃不誤
    int* rit = std::find(std::begin(raw), std::end(raw), 33);
    std::cout << "在原生陣列找 33: "
              << (rit != std::end(raw) ? "找到" : "沒找到")
              << "，索引 " << (rit - std::begin(raw)) << "\n";

    std::cout << "\n=== LeetCode 217 Contains Duplicate ===\n";
    std::cout << "[1,2,3,1]     -> " << containsDuplicate({1, 2, 3, 1}) << "\n";
    std::cout << "[1,2,3,4]     -> " << containsDuplicate({1, 2, 3, 4}) << "\n";
    std::cout << "[1,1,1,3,3,4] -> " << containsDuplicate({1, 1, 1, 3, 3, 4}) << "\n";

    std::cout << "\n=== 日常實務：埠號允許清單 ===\n";
    const std::vector<int> allow = {22, 80, 443, 8080};
    for (int port : {80, 443, 3306}) {
        std::cout << "port " << port << " -> "
                  << (isPortAllowed(allow, port) ? "允許" : "拒絕") << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第一課：STL 的歷史與設計哲學1.cpp -o demo1

// === 預期輸出 ===
// === 原始示範：容器 + 排序 + 搜尋 ===
// 排序後: 1 2 5 8 9
// 找到了: 8
// 它的索引是: 3
//
// === 找不到的情況 ===
// 找 100 的結果: 沒找到（it == end()）
//
// === 對比：find 是線性，binary_search 才吃到有序 ===
// std::binary_search(8) = true
// std::binary_search(7) = false
//
// === 同一個 find 也能用在原生陣列上（正交性）===
// 在原生陣列找 33: 找到，索引 2
//
// === LeetCode 217 Contains Duplicate ===
// [1,2,3,1]     -> true
// [1,2,3,4]     -> false
// [1,1,1,3,3,4] -> true
//
// === 日常實務：埠號允許清單 ===
// port 80 -> 允許
// port 443 -> 允許
// port 3306 -> 拒絕
