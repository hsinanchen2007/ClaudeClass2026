// =============================================================================
//  第 10 課：vector 的宣告與初始化方式 5  —  複製建構與迭代器範圍建構
// =============================================================================
//
// 【主題資訊 Information】
//   vector(const vector& other);                            // (1) 複製建構
//   template<class InputIt>
//   vector(InputIt first, InputIt last,
//          const Allocator& alloc = Allocator());           // (2) 迭代器範圍建構
//
//   標頭檔　：<vector>（用到 std::array 需另外 #include <array>；
//             std::begin/std::end 對 C 陣列在 <iterator> 或多數容器標頭都可取得）
//   標準版本：(1)(2) 均自 C++98；std::begin/std::end 自由函式是 C++11
//   複雜度　：兩者皆 O(n)——都要逐一複製元素
//   範圍語意：[first, last)——左閉右開，last 本身不包含
//
// 【詳細解釋 Explanation】
//
// 【1. 複製建構：深拷貝，不是共享】
//   std::vector<int> copy1(original);      // 直接初始化
//   std::vector<int> copy2 = original;     // 複製初始化（結果相同）
//   兩者都執行「深拷貝」：新配置一塊記憶體，把每個元素複製過去。
//   所以修改 copy1 不會影響 original——這和 Java/Python 的參考語意完全不同，
//   是 C++ 值語意（value semantics）的核心體現。
//   代價是 O(n) 的時間與空間；若原物件之後不再需要，應該改用 move（見檔案 6）。
//
// 【2. 迭代器範圍建構：STL 通用的「跨容器轉換」機制】
//   vector(first, last) 不在乎迭代器來自哪裡，只要能走訪、能解參考即可。
//   這讓它成為 STL 裡最泛用的資料轉換入口：
//       vector<int> v(lst.begin(), lst.end());      // 從 list 轉 vector
//       vector<int> v(s.begin(), s.end());          // 從 set 轉 vector（自動排序好）
//       vector<char> v(str.begin(), str.end());     // 從 string 轉 vector<char>
//       vector<int> v(arr, arr + n);                // 從 C 陣列轉 vector
//   這也是「取子範圍」的標準做法：
//       vector<int> mid(v.begin() + 1, v.begin() + 4);   // 取索引 1、2、3
//
// 【3. 左閉右開 [first, last) 為什麼這樣設計？】
//   三個好處：
//     ① 元素個數直接是 last - first，不必 +1，減少 off-by-one。
//     ② 空範圍自然表示成 first == last，不需要特例。
//     ③ 可以用 end() 當終點——end() 指向「最後一個元素的下一格」，
//        它不可解參考，但可以當邊界，讓迴圈條件統一寫成 it != end。
//   這個慣例貫穿整個 STL，不只是 vector 建構子。
//
// 【4. 為什麼 C 陣列建議用 std::begin/std::end 而不是 arr、arr+n？】
//   std::vector<int> v(std::begin(arr), std::end(arr));   // 推薦
//   std::vector<int> v(arr, arr + 4);                     // 可行，但要自己數
//   前者由編譯器從陣列型別 int[4] 推導出長度，日後增減元素不必改數字；
//   後者把 4 寫死，改陣列卻忘了改 4 就是越界讀取（UB）。
//   ⚠️ 但 std::begin/std::end 只對「真正的陣列」有效——
//   陣列一旦退化成指標（例如傳進函式當參數）就失去長度資訊，無法使用。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 範圍建構的效能優化：
//     當迭代器是 random-access（vector、array、C 指標）時，
//     實作可以先算出 last - first，一次配置到位、只走訪一次。
//     若是 input iterator（例如 istream_iterator），無法預知長度，
//     只能邊讀邊 push_back，過程中可能多次重新配置。
//     這是「容器轉容器很快、串流讀取較慢」的原因。
//
//   ▸ 對 trivially-copyable 型別（int、double、POD struct），
//     libstdc++ 會把逐一複製優化成 memmove，速度接近 memcpy。
//     對 std::string 這種型別則必須逐一呼叫 copy constructor。
//
//   ▸ 型別不必完全相同，只要可轉換：
//     std::vector<double> d(intVec.begin(), intVec.end());   // int → double 逐一轉換
//     這比先建 vector<int> 再手動轉換簡潔得多。
//
// 【注意事項 Pay Attention】
//   1. 範圍是左閉右開：(v.begin(), v.begin()+3) 取的是索引 0、1、2 三個元素。
//   2. 迭代器算術必須自己確保不越界：v.begin() + 100 在 size 只有 5 時是 UB，
//      不會丟例外。取子範圍前請先檢查 size()。
//   3. first 與 last 必須來自同一個容器，且 first 必須能走到 last，
//      否則是 UB。混用兩個不同 vector 的迭代器是常見錯誤。
//   4. 兩個迭代器都是同型別的整數時要小心：vector<int> v(5, 10) 的 5 和 10
//      不會被當成迭代器（實作用 SFINAE 排除整數型別），會正確選中 (count, value)。
//      這是標準明文要求的行為，不必擔心。
//   5. 從 std::set 轉 vector 會保留 set 的排序結果；從 unordered_set 轉出來
//      的順序則是【實作定義】且可能隨版本改變，不可依賴。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】複製建構與範圍建構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector<int> b = a; 之後修改 b[0]，a 會變嗎？
//     答：不會。vector 的複製建構是深拷貝，b 有自己獨立的記憶體。
//         這是 C++ 值語意的展現，和 Java/Python 的「賦值即共享參考」相反。
//     追問：那成本是多少？→ O(n) 時間 + O(n) 空間。
//         若 a 之後不再需要，應改寫 auto b = std::move(a); 變成 O(1)。
//
// 🔥 Q2. 如何取出 vector 中間三個元素成為新 vector？
//     答：用迭代器範圍建構，注意左閉右開：
//         std::vector<int> mid(v.begin() + 1, v.begin() + 4);  // 索引 1、2、3
//         要先確認 v.size() >= 4，否則迭代器越界是 UB。
//     追問：為什麼 STL 一律用左閉右開？
//         → 元素個數 = last - first（不必 +1）、空範圍就是 first==last、
//         且能用不可解參考的 end() 當邊界，三個好處一次到位。
//
// ⚠️ 陷阱. int arr[4]; 傳進函式後，還能用 std::vector<int> v(std::begin(arr), std::end(arr)) 嗎？
//     答：不行。陣列當函式參數時會退化（decay）成指標 int*，
//         型別中的長度資訊 [4] 消失，std::end() 無從得知結尾在哪 → 編譯錯誤。
//         函式內只能用 v(arr, arr + n)，並額外傳入 n。
//     為什麼會錯：以為 std::end() 是執行期去「找」陣列結尾。
//         實際上它是編譯期模板，靠陣列型別 int[4] 推導長度；
//         型別一旦變成 int*，這個資訊就永遠拿不回來了。
// ═══════════════════════════════════════════════════════════════════════════

#include <array>
#include <iostream>
#include <list>
#include <set>
#include <string>
#include <vector>

// 小工具：印出容器內容
template <typename Container>
void dump(const std::string& tag, const Container& c) {
    std::cout << "  " << tag << " (size=" << c.size() << "): ";
    for (const auto& x : c) std::cout << x << " ";
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1929. Concatenation of Array
//   題目：給定長度 n 的陣列 nums，回傳長度 2n 的 ans，
//         其中 ans[i] == nums[i] 且 ans[i + n] == nums[i]（即 nums 接自己一次）。
//   為什麼用到本主題：這題本質就是「用既有範圍建構新 vector」。
//     最直接的寫法是先用複製建構造出第一份，再用 insert 把同一個範圍接上去——
//     兩個動作都吃 [first, last) 迭代器範圍，正是本檔主題。
//   複雜度：時間 O(n)、額外空間 O(n)。
//   注意：insert 的來源是 nums（另一個容器），不是 ans 自己——
//     對自己 insert 會因為可能的重新配置導致迭代器失效，是危險寫法。
// -----------------------------------------------------------------------------
std::vector<int> getConcatenation(const std::vector<int>& nums) {
    std::vector<int> ans(nums);                   // ① 複製建構，先拿到第一份
    ans.reserve(nums.size() * 2);                 //    預留總長度，避免中途重新配置
    ans.insert(ans.end(), nums.begin(), nums.end());  // ② 用範圍再接一份
    return ans;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把舊有 C API 回傳的裸陣列轉成 vector
//   情境：對接一個 C 寫的感測器驅動程式，它的介面長這樣：
//         int read_samples(int* out_buf, int max_count);   // 回傳實際寫入筆數
//   為什麼用範圍建構：C API 給的是「指標 + 長度」，
//     這正是迭代器範圍的原始形式（指標就是最原始的 random-access iterator）。
//     用 vector(buf, buf + n) 一行就完成轉換，之後就能享有
//     自動記憶體管理、size() 查詢、range-for、STL 演算法等全部好處。
//   ★ 關鍵：只取 [buf, buf + n)，n 是「實際寫入筆數」而非緩衝區容量——
//     用錯就會把未初始化的尾端垃圾一起讀進來。
// -----------------------------------------------------------------------------
namespace legacy {
// 模擬一個 C 風格 API：把資料寫進呼叫端提供的緩衝區，回傳實際寫入筆數
int read_samples(int* outBuf, int maxCount) {
    const int fakeData[] = {102, 98, 105, 99, 101};
    int n = static_cast<int>(sizeof(fakeData) / sizeof(fakeData[0]));
    if (n > maxCount) n = maxCount;
    for (int i = 0; i < n; ++i) outBuf[i] = fakeData[i];
    return n;                                     // 只有前 n 筆有效
}
}  // namespace legacy

std::vector<int> readSensorSamples() {
    int buf[16];                                  // 未初始化的固定緩衝區
    int n = legacy::read_samples(buf, 16);        // 實際只寫入 n 筆
    return std::vector<int>(buf, buf + n);        // ★ 只取有效的前 n 筆
}

int main() {
    std::cout << "=== 複製建構：深拷貝，兩者互不影響 ===\n";
    std::vector<int> original = {1, 2, 3, 4, 5};
    std::vector<int> copy1(original);             // 直接初始化
    std::vector<int> copy2 = original;            // 複製初始化
    dump("original", original);
    dump("copy1(original)", copy1);
    dump("copy2 = original", copy2);

    copy1[0] = 999;                               // 只改 copy1
    std::cout << "  修改 copy1[0] = 999 之後：\n";
    dump("original（未受影響）", original);
    dump("copy1（已改變）", copy1);
    std::cout << "  → 深拷貝，不是共享參考。\n";

    std::cout << "\n=== 迭代器範圍建構：左閉右開 [first, last) ===\n";
    std::vector<int> partial(original.begin() + 1, original.begin() + 4);
    dump("partial(begin+1, begin+4)", partial);
    std::cout << "  → 取索引 1、2、3 共三個元素；索引 4 不含（右開）。\n";
    std::cout << "  元素個數 = last - first = 4 - 1 = " << (4 - 1) << "\n";

    std::vector<int> whole(original.begin(), original.end());
    dump("whole(begin, end)", whole);

    std::vector<int> none(original.begin(), original.begin());
    std::cout << "  空範圍 (begin, begin) -> size=" << none.size()
              << "（first == last 自然表示空範圍）\n";

    std::cout << "\n=== 從 C 風格陣列建構 ===\n";
    int arr[] = {10, 20, 30, 40};
    std::vector<int> fromArray(std::begin(arr), std::end(arr));   // 長度自動推導
    dump("fromArray(std::begin, std::end)", fromArray);
    std::vector<int> fromArray2(arr, arr + 4);                    // 手動指定長度
    dump("fromArray2(arr, arr + 4)", fromArray2);
    std::cout << "  → 前者由型別 int[4] 推導長度，增減元素時不必改程式碼（推薦）。\n";

    std::cout << "\n=== 從 std::array 建構 ===\n";
    std::array<int, 3> stdArr = {100, 200, 300};
    std::vector<int> fromStdArray(stdArr.begin(), stdArr.end());
    dump("fromStdArray", fromStdArray);

    std::cout << "\n=== 範圍建構是跨容器轉換的通用入口 ===\n";
    std::list<int> lst = {7, 8, 9};
    std::vector<int> fromList(lst.begin(), lst.end());
    dump("從 std::list 轉來", fromList);

    std::set<int> st = {50, 10, 30, 10};          // set 會排序且去重
    std::vector<int> fromSet(st.begin(), st.end());
    dump("從 std::set 轉來（已排序去重）", fromSet);

    std::string str = "Hello";
    std::vector<char> fromString(str.begin(), str.end());
    std::cout << "  從 std::string 轉 vector<char> (size=" << fromString.size() << "): ";
    for (char ch : fromString) std::cout << ch << " ";
    std::cout << "\n";

    std::cout << "\n=== 元素型別可自動轉換 ===\n";
    std::vector<double> asDouble(original.begin(), original.end());
    std::cout << "  vector<int> -> vector<double>: ";
    for (double d : asDouble) std::cout << d << " ";
    std::cout << "（逐一做 int → double 轉換）\n";

    std::cout << "\n=== LeetCode 1929. Concatenation of Array ===\n";
    dump("nums = {1,2,1}   -> ans", getConcatenation({1, 2, 1}));
    dump("nums = {1,3,2,1} -> ans", getConcatenation({1, 3, 2, 1}));

    std::cout << "\n=== 日常實務：C API 裸陣列 -> vector ===\n";
    std::vector<int> samples = readSensorSamples();
    dump("感測器讀數", samples);
    std::cout << "  → 緩衝區宣告了 16 格，但只取 API 回報的有效筆數 "
              << samples.size() << " 筆，\n"
              << "    避免把未初始化的尾端垃圾一起讀進來。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：vector 的宣告與初始化方式5.cpp" -o lesson10_5

// === 預期輸出 ===
// === 複製建構：深拷貝，兩者互不影響 ===
//   original (size=5): 1 2 3 4 5
//   copy1(original) (size=5): 1 2 3 4 5
//   copy2 = original (size=5): 1 2 3 4 5
//   修改 copy1[0] = 999 之後：
//   original（未受影響） (size=5): 1 2 3 4 5
//   copy1（已改變） (size=5): 999 2 3 4 5
//   → 深拷貝，不是共享參考。
//
// === 迭代器範圍建構：左閉右開 [first, last) ===
//   partial(begin+1, begin+4) (size=3): 2 3 4
//   → 取索引 1、2、3 共三個元素；索引 4 不含（右開）。
//   元素個數 = last - first = 4 - 1 = 3
//   whole(begin, end) (size=5): 1 2 3 4 5
//   空範圍 (begin, begin) -> size=0（first == last 自然表示空範圍）
//
// === 從 C 風格陣列建構 ===
//   fromArray(std::begin, std::end) (size=4): 10 20 30 40
//   fromArray2(arr, arr + 4) (size=4): 10 20 30 40
//   → 前者由型別 int[4] 推導長度，增減元素時不必改程式碼（推薦）。
//
// === 從 std::array 建構 ===
//   fromStdArray (size=3): 100 200 300
//
// === 範圍建構是跨容器轉換的通用入口 ===
//   從 std::list 轉來 (size=3): 7 8 9
//   從 std::set 轉來（已排序去重） (size=3): 10 30 50
//   從 std::string 轉 vector<char> (size=5): H e l l o
//
// === 元素型別可自動轉換 ===
//   vector<int> -> vector<double>: 1 2 3 4 5 （逐一做 int → double 轉換）
//
// === LeetCode 1929. Concatenation of Array ===
//   nums = {1,2,1}   -> ans (size=6): 1 2 1 1 2 1
//   nums = {1,3,2,1} -> ans (size=8): 1 3 2 1 1 3 2 1
//
// === 日常實務：C API 裸陣列 -> vector ===
//   感測器讀數 (size=5): 102 98 105 99 101
//   → 緩衝區宣告了 16 格，但只取 API 回報的有效筆數 5 筆，
//     避免把未初始化的尾端垃圾一起讀進來。
