// =============================================================================
//  第一課：STL 的歷史與設計哲學 2  —  一個 std::sort，處理所有型別
// =============================================================================
//
// 【主題資訊 Information】
//   template<class RandomIt> void sort(RandomIt first, RandomIt last);           // (1)
//   template<class RandomIt, class Compare>
//   void sort(RandomIt first, RandomIt last, Compare comp);                      // (2)
//   標頭檔：<algorithm>
//   標準版本：C++98 起。C++11 起要求最壞 O(N log N)（C++98 只要求「平均」）。
//   複雜度：O(N log N) 次比較，就地排序（不需額外 O(N) 空間）。
//   要求：random access iterator；(1) 用 operator<，(2) 用你給的 comp。
//   ⚠️ std::sort 不穩定（相等元素的相對順序不保證）。要穩定請用 std::stable_sort。
//
// 【詳細解釋 Explanation】
//
// 【1. 「一套程式碼處理多型別」到底是怎麼做到的】
//   std::sort 是 function template。當你寫下
//       std::sort(ints.begin(), ints.end());
//   編譯器做的事叫「具現化（instantiation）」：把 RandomIt 代換成
//   `std::vector<int>::iterator`，產生一份專門排 int 的機器碼。
//   下一行 std::sort(doubles...) 又產生另一份專門排 double 的機器碼。
//   所以最終二進位檔裡其實有三份不同的 sort——這叫 code bloat，是泛型的成本。
//   換來的是：每一份都是為該型別特化、可以完全 inline 的最佳碼。
//   這正是 STL 的核心取捨：用編譯時間與程式碼體積，換執行期零抽象成本。
//
// 【2. 為什麼 C++ 的泛型不需要繼承或共同基底類別】
//   Java 早期用 Object + 向下轉型，需要裝箱、需要執行期型別檢查。
//   C++ 的 template 走的是「結構化型別（structural typing）」——
//   俗稱 duck typing 的編譯期版本：
//     std::sort 對 T 的唯一要求是「T 之間能用 < 比較」「T 可移動賦值」。
//   T 不必繼承任何東西、不必實作任何 interface。
//   int 這種內建型別根本不可能繼承任何類別，卻照樣能被 std::sort 處理，
//   這在「必須有共同基底」的語言裡是做不到的。
//
// 【3. std::string 為什麼能直接排序，排出來又是什麼順序】
//   std::string 有 operator<，它做的是「字典序（lexicographical）比較」，
//   逐字元比較 char 的值。注意這是「位元組序」不是「語意序」：
//     * 大寫字母的 ASCII 值小於小寫（'Z'=90 < 'a'=97），
//       所以 "Zebra" 會排在 "apple" 前面。
//     * 數字字串 "10" < "9"，因為第一個字元 '1' < '9'。
//     * 中文（UTF-8）比的是位元組，不是筆畫也不是注音。
//   要語意排序必須自己提供比較子，或借助 locale / ICU。
//
// 【4. 浮點數排序的一個地雷】
//   std::sort 要求比較子構成「嚴格弱序（strict weak ordering）」。
//   double 只要不含 NaN 就滿足。但只要區間裡有一個 NaN，
//   `NaN < x` 和 `x < NaN` 都是 false，嚴格弱序被破壞，
//   std::sort 的行為就是未定義的——實務上可能越界寫入。
//   有 NaN 風險的資料要先過濾（std::isnan）或用自訂比較子處理。
//
// 【概念補充 Concept Deep Dive】
//
// (A) IntroSort：std::sort 內部到底是什麼
//     libstdc++ 的 std::sort 是 David Musser 的 IntroSort（1997），
//     三種演算法混用，各自補對方的短處：
//       ① QuickSort     —— 主力，平均最快、cache 友善
//       ② HeapSort      —— 當遞迴深度超過 2*log2(N) 時切換，
//                          把 QuickSort 最壞的 O(N²) 硬壓回 O(N log N)
//       ③ InsertionSort —— 區間縮到十幾個元素以下時改用，
//                          小陣列上它的常數項最小
//     這個「深度守衛」正是 C++11 能把複雜度保證從「平均」升級成
//     「最壞 O(N log N)」的原因。
//
// (B) 為什麼 std::sort 通常比 C 的 qsort 快
//     不是演算法比較好，是「比較子能不能 inline」的差別：
//       qsort    : void qsort(void*, size_t, size_t,
//                             int(*cmp)(const void*, const void*));
//                  cmp 是執行期函式指標 → 每次比較一次間接呼叫，不能 inline；
//                  元素搬移只能透過 memcpy（它不知道型別）。
//       std::sort: Compare 是 template 參數 → 編譯期就知道實體，
//                  lambda / operator< 直接被 inline 進迴圈；
//                  元素搬移用該型別的 move 建構子。
//     同一台機器、相同資料，std::sort 常見快 2～3 倍。
//
// (C) 為什麼 sort 只吃 random access iterator
//     IntroSort 需要「取中位數」「跳到區間中點」「計算區間長度」，
//     這些都需要 it + n / it2 - it1，只有 random access iterator 提供。
//     std::list 只有 bidirectional iterator，所以 std::sort(list...)
//     會在編譯期失敗（訊息通常很長，因為錯誤發生在 template 內部深處）。
//
// 【注意事項 Pay Attention】
//   1. std::sort 不穩定。相等元素的相對順序可能被打亂。
//      需要保持原順序（例如「先依部門排、部門內維持原本的到職順序」）
//      必須用 std::stable_sort（代價：可能配置 O(N) 暫存記憶體）。
//   2. 比較子必須是嚴格弱序。最常見的錯誤是寫 `return a <= b;`——
//      這會讓 comp(a, a) 為 true，違反非自反性，是未定義行為；
//      在 libstdc++ 的 _GLIBCXX_DEBUG 模式下會直接 assert。
//   3. sort 會改變「reference / pointer 所觀察到的值」，
//      但不會使 vector 的 iterator 失效（沒有重新配置記憶體）。
//   4. 對已排序或幾乎排序的資料，std::sort 沒有特別最佳化；
//      若資料常常近乎有序，需要另尋策略（例如 Timsort 類的做法）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::sort 與泛型編程
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 同一個 std::sort 能排 int、double、string，這在編譯後是「一份」
//        還是「三份」程式碼？
//     答：三份。template 在編譯期為每個實際用到的型別各具現化一份專屬碼。
//         好處是每份都能完全 inline、零抽象成本；代價是程式碼膨脹
//         （code bloat）與編譯時間變長。這是 C++ 泛型與 Java 泛型
//         （型別抹除，只有一份）最根本的分野。
//     追問：那 template 具現化出來的重複符號，連結時為什麼不會衝突？
//         → 它們被標記為 weak symbol / COMDAT，連結器會自動摺疊成一份。
//
// 🔥 Q2. std::sort 和 std::stable_sort 差在哪？各自的複雜度與代價？
//     答：std::sort 不保證相等元素的相對順序，O(N log N)、O(log N) 堆疊空間。
//         std::stable_sort 保證相對順序不變；記憶體充足時是 O(N log N)
//         並使用 O(N) 暫存空間，記憶體不足時退化為 O(N log²N) 的就地版本。
//     追問：什麼情境非要 stable 不可？
//         → 多鍵排序時「先排次要鍵、再排主要鍵」的做法，
//           完全依賴穩定性才能保住次要鍵的順序。
//
// ⚠️ 陷阱. 自訂比較子寫成 `[](int a, int b){ return a <= b; }` 有什麼問題？
//         它看起來只是「允許相等」而已，測試小資料也都跑對。
//     答：這是未定義行為。std::sort 要求比較子構成嚴格弱序，
//         其中一條是「非自反性」：comp(x, x) 必須為 false。
//         用 <= 的話 comp(x, x) 為 true，排序內部的邊界判斷會失效，
//         可能讀寫到區間之外——實務上就是隨機崩潰或記憶體損毀。
//     為什麼會錯：把比較子理解成「回答 a 是否應該排在 b 前面或同位」，
//         但標準要的是嚴格的「小於」語意。相等的情況必須回傳 false，
//         由演算法自己處理，而不是由你「允許」它相等。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cctype>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 179. Largest Number
//   題目：給一組非負整數，重新排列順序後拼接，組成可能的最大數字（以字串回傳）。
//         例如 [3,30,34,5,9] -> "9534330"。
//   為什麼用到本主題：這題不能用預設的 operator<（純字典序會排錯），
//     必須提供自訂比較子 `a+b > b+a`。它完美示範本課主題：
//     同一個 std::sort，換一個比較子就變成完全不同的排序語意——
//     「演算法」與「排序準則」是正交的兩件事。
//   複雜度：O(N log N * L)，L 是數字的字串長度（每次比較都要拼接字串）。
// -----------------------------------------------------------------------------
std::string largestNumber(std::vector<int> nums) {
    std::vector<std::string> s;
    s.reserve(nums.size());
    for (int n : nums) s.push_back(std::to_string(n));

    // 關鍵：若 a+b 拼起來比 b+a 大，a 就該排前面
    std::sort(s.begin(), s.end(), [](const std::string& a, const std::string& b) {
        return a + b > b + a;
    });

    // 全是 0 的特例：避免輸出 "000"
    if (!s.empty() && s[0] == "0") return "0";

    std::string res;
    for (const auto& t : s) res += t;
    return res;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】依「檔案大小」排序目錄清單（帶次要鍵）
//   情境：寫一支類似 `du -s | sort -rn` 的小工具，列出佔空間最大的檔案。
//         大小相同時再依檔名字典序排，讓輸出可重現（不同機器結果一致）。
//   為什麼用到本主題：真實世界的排序幾乎都是「自訂比較子 + 多鍵」，
//     而不是課本上的 sort(v.begin(), v.end())。這裡示範怎麼把多個鍵
//     組合進一個仍然合法的嚴格弱序比較子——注意主鍵相等時才比次鍵，
//     且每一層回傳的都是嚴格 < 或 >，絕不寫 <=。
// -----------------------------------------------------------------------------
struct FileEntry {
    std::string name;
    long long   bytes;
};

// 給 sort / stable_sort 穩定性示範用：key 是排序鍵，id 記錄原始順序
struct Record {
    int key;
    int id;
};

void sortBySizeDesc(std::vector<FileEntry>& files) {
    std::sort(files.begin(), files.end(), [](const FileEntry& a, const FileEntry& b) {
        if (a.bytes != b.bytes) return a.bytes > b.bytes;  // 主鍵：大小由大到小
        return a.name < b.name;                            // 次鍵：檔名字典序
    });
}

int main() {
    std::cout << "=== 原始示範：同一個 sort，處理所有型別 ===\n";

    // 同一個 sort，處理所有型別
    std::vector<int> ints = {3, 1, 4, 1, 5};
    std::vector<double> doubles = {3.14, 1.41, 2.72};
    std::vector<std::string> strings = {"cherry", "apple", "banana"};

    std::sort(ints.begin(), ints.end());
    std::sort(doubles.begin(), doubles.end());
    std::sort(strings.begin(), strings.end());

    std::cout << "排序後的整數: ";
    for (int n : ints) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "排序後的浮點數: ";
    for (double d : doubles) std::cout << d << " ";
    std::cout << std::endl;

    std::cout << "排序後的字串: ";
    for (const auto& s : strings) std::cout << s << " ";
    std::cout << std::endl;

    std::cout << "\n=== 字串排序是「位元組序」不是「語意序」===\n";
    std::vector<std::string> tricky = {"apple", "Zebra", "10", "9", "Apple"};
    std::sort(tricky.begin(), tricky.end());
    std::cout << "預設 operator< : ";
    for (const auto& s : tricky) std::cout << s << " ";
    std::cout << "\n";
    std::cout << "注意：'Z'(90) < 'a'(97) 所以大寫排前面；\"10\" < \"9\" 因為 '1' < '9'\n";

    std::cout << "\n=== 換一個比較子，同一個 sort 就變成另一種語意 ===\n";
    // 不分大小寫排序：只是換 comp，sort 本身一個字都沒改
    std::vector<std::string> ci = {"apple", "Zebra", "Apple", "banana"};
    std::sort(ci.begin(), ci.end(), [](const std::string& a, const std::string& b) {
        return std::lexicographical_compare(
            a.begin(), a.end(), b.begin(), b.end(),
            [](unsigned char x, unsigned char y) {
                return std::tolower(x) < std::tolower(y);
            });
    });
    std::cout << "不分大小寫: ";
    for (const auto& s : ci) std::cout << s << " ";
    std::cout << "\n";

    std::cout << "\n=== sort vs stable_sort：相等元素的順序 ===\n";
    // 40 筆記錄，key 只有 0~3 四種 → 大量元素在比較子眼中「相等」。
    // 只比 key，完全不看 id；於是 id 的排列就暴露了穩定性差異。
    // ⚠️ 元素少（十來個）時 libstdc++ 會退化成 insertion sort，
    //    std::sort 剛好也保持了順序——那是巧合，不是保證。要看出差異得夠多筆。
    std::vector<Record> r1;
    for (int i = 0; i < 40; ++i) r1.push_back({i % 4, i});
    std::vector<Record> r2 = r1;
    auto byKey = [](const Record& a, const Record& b) { return a.key < b.key; };
    std::sort(r1.begin(), r1.end(), byKey);
    std::stable_sort(r2.begin(), r2.end(), byKey);

    std::cout << "sort        中 key==0 的 id 順序: ";
    for (const auto& r : r1) if (r.key == 0) std::cout << r.id << " ";
    std::cout << "\n";
    std::cout << "stable_sort 中 key==0 的 id 順序: ";
    for (const auto& r : r2) if (r.key == 0) std::cout << r.id << " ";
    std::cout << "\n";
    std::cout << "只有 stable_sort 保證 id 遞增（= 維持原始相對順序）。\n";
    std::cout << "std::sort 打亂後的確切排列是實作定義的，換編譯器/版本可能不同。\n";

    std::cout << "\n=== LeetCode 179 Largest Number ===\n";
    std::cout << "[10,2]        -> " << largestNumber({10, 2}) << "\n";
    std::cout << "[3,30,34,5,9] -> " << largestNumber({3, 30, 34, 5, 9}) << "\n";
    std::cout << "[0,0]         -> " << largestNumber({0, 0}) << "\n";

    std::cout << "\n=== 日常實務：依檔案大小排序（大小相同再比檔名）===\n";
    std::vector<FileEntry> files = {
        {"access.log",  1048576},
        {"error.log",     20480},
        {"app.log",     1048576},
        {"debug.log",      4096},
    };
    sortBySizeDesc(files);
    for (const auto& f : files) {
        std::cout << f.bytes << "\t" << f.name << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第一課：STL 的歷史與設計哲學2.cpp -o demo2

// === 預期輸出 ===
// === 原始示範：同一個 sort，處理所有型別 ===
// 排序後的整數: 1 1 3 4 5
// 排序後的浮點數: 1.41 2.72 3.14
// 排序後的字串: apple banana cherry
//
// === 字串排序是「位元組序」不是「語意序」===
// 預設 operator< : 10 9 Apple Zebra apple
// 注意：'Z'(90) < 'a'(97) 所以大寫排前面；"10" < "9" 因為 '1' < '9'
//
// === 換一個比較子，同一個 sort 就變成另一種語意 ===
// 不分大小寫: apple Apple banana Zebra
//
// === sort vs stable_sort：相等元素的順序 ===
// sort        中 key==0 的 id 順序: 8 16 20 12 24 28 32 4 36 0
// stable_sort 中 key==0 的 id 順序: 0 4 8 12 16 20 24 28 32 36
// 只有 stable_sort 保證 id 遞增（= 維持原始相對順序）。
// std::sort 打亂後的確切排列是實作定義的，換編譯器/版本可能不同。
//
// === LeetCode 179 Largest Number ===
// [10,2]        -> 210
// [3,30,34,5,9] -> 9534330
// [0,0]         -> 0
//
// === 日常實務：依檔案大小排序（大小相同再比檔名）===
// 1048576	access.log
// 1048576	app.log
// 20480	error.log
// 4096	debug.log
