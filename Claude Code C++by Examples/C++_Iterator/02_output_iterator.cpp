// =============================================================================
//  02_output_iterator.cpp  —  OutputIterator
// =============================================================================
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼這個 iterator 存在?                              │
//  └────────────────────────────────────────────────────────────┘
//
//  STL 把「輸出端」和「輸入端」拆成兩種 iterator,目的是讓演算法
//  能對應到「沒辦法回頭讀、只能寫」的目的端 — 例如:
//    * std::cout / std::ofstream  (字寫出去就送出,不能回頭改)
//    * 網路 socket / 序列埠       (寫了就出去)
//    * 資料壓縮器的輸入接口        (寫進去後內部已 hash / encode)
//    * 容器的 push_back / insert  (沒有「下一個位置」存在,要靠寫入時動態長)
//
//  這些目的端有共通特性:
//    * 一個位置只能「寫」一次
//    * 寫完後要前進到下一個位置 (++)
//    * 不能讀回來
//    * 不一定有「終點」(stream 沒有 end,寫到不寫為止)
//
//  STL 抽象出 OutputIterator 介面,讓 std::copy / std::transform / std::merge
//  等演算法可以「無感地」把資料寫到上述任何目的端。一次寫好的 std::copy
//  既能寫到 vector、也能寫到 cout、也能寫到 file — 這就是泛型的價值。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、特性 (LegacyOutputIterator)                            │
//  └────────────────────────────────────────────────────────────┘
//
//   * 「只能寫」+「單次走訪 (single-pass)」。
//   * 支援的操作:
//       *it = value   寫入 (注意:解參考的目的就是「指派給它」,不是「讀」)
//       ++it / it++   前進
//   * 「不要求可比較相等 (==)」、「不能讀」、「不能往回走」、「不保證可重走」。
//   * 演算法範例:std::copy, std::transform, std::generate, std::fill_n,
//                std::merge (目的端) — 這些演算法的「寫出位置」只需 OutputIterator。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、底層機制 — *it = x 是「整體寫入動作」                  │
//  └────────────────────────────────────────────────────────────┘
//
//  別把 *it = x 想成「先解參考拿出位置、再用 = 寫進去」兩步驟。
//  對 OutputIterator 而言,這是「一個動作」 — 標準只規定整段表達式的效果。
//
//  例如 ostream_iterator<int>(cout, ", ") 的內部實作概念上是:
//
//      class ostream_iterator {
//          ostream* os; const char* delim;
//          ostream_iterator& operator*()  { return *this; }   // *it 回傳自己
//          ostream_iterator& operator=(int x) {                // 真正動作在這
//              (*os) << x;
//              if (delim) (*os) << delim;
//              return *this;
//          }
//          ostream_iterator& operator++()    { return *this; }  // ++ 是 no-op!
//          ostream_iterator& operator++(int) { return *this; }
//      };
//
//  注意 ++ 通常是「什麼都不做」,真正的「前進」發生在寫入 stream 的當下。
//  這也說明了為什麼 OutputIterator「不能讀」 — 因為 *it 根本不是「位置」,
//  而是「寫入接口」(就是 iterator 自己)。
//
//  back_insert_iterator 也很類似:
//
//      class back_insert_iterator {
//          Container* c;
//          back_insert_iterator& operator*()  { return *this; }
//          back_insert_iterator& operator=(const T& x) { c->push_back(x); return *this; }
//          back_insert_iterator& operator++()    { return *this; }
//          back_insert_iterator& operator++(int) { return *this; }
//      };
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、常見來源 (典型 OutputIterator)                         │
//  └────────────────────────────────────────────────────────────┘
//
//   * std::ostream_iterator<T>      ─ 寫到 stream (帶分隔符)
//   * std::back_insert_iterator     ─ std::back_inserter(c) → push_back
//   * std::front_insert_iterator    ─ std::front_inserter(c) → push_front (list/deque)
//   * std::insert_iterator          ─ std::inserter(c, pos) → 通用 insert
//   * std::ostreambuf_iterator      ─ 直接寫 streambuf (最低階,最快)
//   * 一般 forward/random_access iterator 「指向非 const 元素」時,
//     也滿足 OutputIterator (例如 vector<int>::iterator)。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、何時使用 / 何時不要                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  使用:
//   * 寫泛型函式接收「寫出端」時,參數型別就用 OutputIterator,
//     呼叫端要寫到什麼容器或 stream 都行。
//   * std::copy(src.begin(), src.end(), back_inserter(dst))
//     是「append 一段資料到容器」最 idiomatic 的寫法。
//
//  不要:
//   * 把 OutputIterator 當「儲存的迭代器」傳來傳去 — 它只能寫一次,
//     用完即丟。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 六、Pitfalls (常見陷阱)                                    │
//  └────────────────────────────────────────────────────────────┘
//
//   1. 給 std::copy 的 destination 一定要「容量夠」或用 inserter 系列,
//      否則會寫到無效記憶體 (UB)。例如:
//          std::vector<int> dst;                // 空!
//          std::copy(v.begin(), v.end(), dst.begin());  // UB!
//      正確:
//          std::copy(v.begin(), v.end(), std::back_inserter(dst));
//      或先 dst.resize(v.size()) 再 std::copy(..., dst.begin())。
//   2. 對同一個 OutputIterator 寫兩次也是 UB (規範就是「每位置寫一次」)。
//   3. 不能比較相等 — 沒有 end() 概念,演算法靠來源端的 [first, last) 結束。
//   4. 對 trivially_copyable + RandomAccess 的 dst,std::copy 可能特化成
//      memmove,看起來似乎「兩遍走也沒事」 — 但語意上仍是「每位置一次」。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 七、參考連結 (References)                                  │
//  └────────────────────────────────────────────────────────────┘
//
//    https://en.cppreference.com/w/cpp/named_req/OutputIterator       — 規格定義
//    https://en.cppreference.com/w/cpp/iterator/ostream_iterator       — 典型實例
//    https://en.cppreference.com/w/cpp/iterator/back_insert_iterator   — 另一典型
//    https://en.cppreference.com/w/cpp/iterator/front_insert_iterator  — 頭部插入
//    https://en.cppreference.com/w/cpp/iterator/insert_iterator        — 任意位置
//    https://cplusplus.com/reference/iterator/OutputIterator/          — 簡明
// =============================================================================

/*
補充筆記：output_iterator
  - output_iterator 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - output iterator 只負責寫入，解參考後賦值是核心操作，不保證能讀回剛寫的值。
  - back_inserter 是 output iterator，可讓 copy/transform 自動 push_back。
  - 使用 output iterator 時，目的地容量或插入策略必須明確，否則容易寫越界。
*/
#include <iostream>
#include <iterator>
#include <vector>
#include <list>
#include <algorithm>
#include <set>
#include <string>
#include <sstream>

int main() {
    // -----------------------------------------------------------------------
    // 範例 1:ostream_iterator — 把 vector 印成 "1, 2, 3, 4, 5"
    //   ostream_iterator<int>(cout, ", ") 每次 *it = x 就會做 cout << x << ", "
    // -----------------------------------------------------------------------
    std::vector<int> v = {1, 2, 3, 4, 5};
    std::copy(v.begin(), v.end(), std::ostream_iterator<int>(std::cout, ", "));
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 範例 2:back_inserter — 把元素 append 到 dst
    //   一開始 dst 是空的,但 back_inserter 把 push_back 包成 OutputIterator,
    //   所以「邊寫邊長」是安全的。
    // -----------------------------------------------------------------------
    std::vector<int> dst;
    std::copy(v.begin(), v.end(), std::back_inserter(dst));
    std::cout << "dst.size() = " << dst.size() << " (期望 5)\n";

    // -----------------------------------------------------------------------
    // 範例 3:generate_n + ostream_iterator,產生 5 個遞增整數直接印
    // -----------------------------------------------------------------------
    int seed = 0;
    std::generate_n(std::ostream_iterator<int>(std::cout, " "),
                    5,
                    [&]{ return ++seed; });        // 1 2 3 4 5
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_88_merge_sorted_array();
    void leetcode_1929_concatenation_of_array();
    void practical_log_filter_to_file_or_stream();
    void leetcode_905_sort_array_by_parity();
    void practical_csv_export();
    leetcode_88_merge_sorted_array();
    leetcode_1929_concatenation_of_array();
    practical_log_filter_to_file_or_stream();
    leetcode_905_sort_array_by_parity();
    practical_csv_export();

    // -----------------------------------------------------------------------
    // 課程知識補充:「*it = x」是「整體寫入」、為什麼 OutputIterator 不需 ==
    //   * OutputIterator 的契約規定:每往前走一步「最多寫一次」、寫完才 ++。
    //   * 換句話說,下面的寫法是 UB:
    //         *out++; *out = 5;        // 已經 ++ 過去了還回頭寫
    //         *out = 5; *out = 6;      // 同一個位置寫兩次
    //   * 安全慣用語:每個位置「寫一次接一個 ++」:
    //         *out = x; ++out;     或者     *out++ = x;
    //   * 為什麼要這麼嚴格? ostream_iterator 寫到 stream,
    //     根本沒有「回頭修改」的概念 — 字早就送出去了。
    //
    //   * OutputIterator「不要求」可比較 (==),因為「寫」沒有「終點」概念。
    //     終點通常是「等到不需要再寫了」,由演算法外部 (來源端的 [first, last))
    //     控制。這也解釋為什麼 std::copy 的簽章是
    //         copy(InputIt first, InputIt last, OutputIt dst)
    //     ── 只看來源何時結束,目的端不必有 end。
    // -----------------------------------------------------------------------

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:OutputIterator 為什麼不要求支援 == 比較?
    //    A:寫出端沒有「終點」概念,串流可以一直寫下去。演算法的終止條件由「來源端」
    //      的 [first, last) 控制,而不是目的端。例如 std::copy 是「來源走完就停」,
    //      不關心 dst 走到哪。因此規格只要求 *it=x 與 ++it,沒有 == / !=。
    //
    //  Q2:back_insert_iterator 算是 OutputIterator 的特例嗎?
    //    A:對。back_insert_iterator / front_insert_iterator / insert_iterator 三者
    //      都是 OutputIterator 的 adaptor — 它們把容器的 push_back / push_front /
    //      insert 包裝成「*it = x」介面,讓演算法能無感地把資料倒進「會自動長大」
    //      的容器,完全避開「dst 容量不足」的 UB。
    //
    //  Q3:為什麼對同一個 OutputIterator 寫兩次 (*it = a; *it = b;) 是 UB?
    //    A:OutputIterator 的契約規定「每往前走一步,最多寫一次」,寫完才能 ++。
    //      ostream_iterator 寫進 stream 後字元已送出,根本沒辦法回頭改;
    //      back_insert_iterator 連續寫兩次會 push_back 兩次造成意料外結果。
    //      安全慣用語固定為:*out = x; ++out;  或者  *out++ = x;
    //

    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 88: 合併兩個有序陣列 (Merge Sorted Array)
// ----------------------------------------------------------------
// 題目:給兩個已排序整數陣列 nums1 和 nums2,合併後仍保持排序。
//      原題要求 in-place 寫進 nums1,但這裡示範「合併到第三個容器」的版本,
//      重點在示範 std::merge 的「目的端只需要 OutputIterator」。
//
// 為什麼這題對 OutputIterator 適合:
//   * std::merge 簽章:
//       merge(InputIt1 first1, InputIt1 last1,
//             InputIt2 first2, InputIt2 last2,
//             OutputIt d_first);
//   * 目的端不必預先配置容量 → 用 std::back_inserter 即可,
//     不必擔心 dst 是否事先 reserve。
//
// 解法核心:
//   * std::merge 比較兩端最小值,把較小者寫入 dst,然後該端往前。
//   * 直到一端走完,把另一端剩下全部寫入。
//
// 複雜度:時間 O(m + n);空間 O(m + n) (新陣列 merged)。
void leetcode_88_merge_sorted_array() {
    std::vector<int> a = {1, 3, 5, 7};
    std::vector<int> b = {2, 4, 6, 8};
    std::vector<int> merged;
    // back_inserter(merged):把 push_back 包成 OutputIterator
    std::merge(a.begin(), a.end(),
               b.begin(), b.end(),
               std::back_inserter(merged));
    std::cout << "LC88 merged: ";
    for (int x : merged) std::cout << x << ' ';    // 1 2 3 4 5 6 7 8
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1929: 連接兩次的陣列 (Concatenation of Array)
// ----------------------------------------------------------------
// 題目:給陣列 nums,回傳 ans = nums + nums (把 nums 接在自己後面變兩倍長度)。
//
// 為什麼這題對 OutputIterator 適合:
//   * 用 back_inserter 把同一個來源寫兩次到目的端 — 經典「Output 端」範例。
//   * std::copy 的 destination 必須是 OutputIterator,
//     而 back_inserter 把 push_back 包成 OutputIterator 介面,
//     讓「目的容器邊長邊寫」這件事在演算法層級無感地完成。
//
// 解法核心:
//   * std::copy 一次寫第一份、再 std::copy 一次寫第二份,目的端都是 back_inserter。
//   * 事先 reserve(2n) 避免重新配置 (小優化)。
//
// 複雜度:時間 O(n);空間 O(n) (答案陣列 2n)。
void leetcode_1929_concatenation_of_array() {
    auto get_concat = [](const std::vector<int>& nums) {
        std::vector<int> ans;
        ans.reserve(nums.size() * 2);                          // 小優化
        std::copy(nums.begin(), nums.end(), std::back_inserter(ans));
        std::copy(nums.begin(), nums.end(), std::back_inserter(ans));
        return ans;
    };
    std::vector<int> base = {1, 2, 1};
    auto ans = get_concat(base);
    std::cout << "LC1929 concat: ";
    for (int x : ans) std::cout << x << ' ';                   // 1 2 1 1 2 1
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:把符合條件的 log 篩到「任何輸出端」
// ----------------------------------------------------------------
// 場景:程式有大量 log,想把 ERROR 等級的篩出來;但「篩出來要送哪去」
//      可能是「另一個 vector 暫存」、「直接印到 stderr」、「寫到檔案」。
//      用泛型函式接 OutputIterator,呼叫端決定輸出端。
//
// 重點:
//   * std::copy_if 的 dst 是 OutputIterator,什麼都能塞。
//   * 同一份 filter_errors 函式,既能寫到 vector,也能寫到 ostream_iterator。
//   * 這就是「把目的端抽象為 OutputIterator」的實際好處。
template <class OutIt>
OutIt filter_errors(const std::vector<std::string>& logs, OutIt out) {
    return std::copy_if(logs.begin(), logs.end(), out,
                        [](const std::string& line){
                            return line.find("ERROR") != std::string::npos;
                        });
}

void practical_log_filter_to_file_or_stream() {
    std::vector<std::string> logs = {
        "2026-05-04 10:00:01 INFO  startup",
        "2026-05-04 10:00:02 ERROR connect timeout",
        "2026-05-04 10:00:03 WARN  retry",
        "2026-05-04 10:00:04 ERROR auth failed",
    };

    // 用法 1:篩到另一個 vector
    std::vector<std::string> errors;
    filter_errors(logs, std::back_inserter(errors));
    std::cout << "錯誤 log 共 " << errors.size() << " 條 (期望 2)\n";

    // 用法 2:篩到 ostream_iterator (例如直接送 cout)
    std::cout << "錯誤 log 直接 dump:\n";
    filter_errors(logs, std::ostream_iterator<std::string>(std::cout, "\n"));

    // 用法 3:篩到 stringstream (常見:組成 email body)
    std::ostringstream oss;
    filter_errors(logs, std::ostream_iterator<std::string>(oss, " | "));
    std::cout << "組成 email: " << oss.str() << '\n';
}

// ----------------------------------------------------------------
// LeetCode 905: Sort Array By Parity (按奇偶排序)
// ----------------------------------------------------------------
// 題目:給整數陣列 nums,重排使所有偶數排在前面、奇數在後 (各自順序不要求)。
//
// 為什麼這題對 OutputIterator 完美:
//   * 用兩次 std::copy_if:第一次把「偶數」寫到 dst (用 back_inserter),
//     第二次把「奇數」接著寫到 dst,兩次的目的端都是 OutputIterator。
//   * 寫法簡潔,核心邏輯就是「依條件分類後依序寫入」 — 經典 OutputIterator 套路。
//
// 解法核心:std::copy_if 兩遍,先 even 後 odd。
// 複雜度:時間 O(n)、空間 O(n)。
void leetcode_905_sort_array_by_parity() {
    std::vector<int> nums{3, 1, 2, 4, 7, 8};
    std::vector<int> result;
    result.reserve(nums.size());
    // 先寫偶數
    std::copy_if(nums.begin(), nums.end(), std::back_inserter(result),
                 [](int x){ return x % 2 == 0; });
    // 再寫奇數
    std::copy_if(nums.begin(), nums.end(), std::back_inserter(result),
                 [](int x){ return x % 2 != 0; });
    std::cout << "LC905 sort by parity: ";
    for (int x : result) std::cout << x << ' ';
    std::cout << '\n';
    // 預期輸出: 2 4 8 3 1 7
}

// ----------------------------------------------------------------
// 實務範例:把資料以 CSV 格式輸出到任意串流
// ----------------------------------------------------------------
// 場景:後端常把 vector<int> 匯出成 CSV 寫到檔案 / HTTP response / log。
// 用 ostream_iterator<int>(stream, ",") 把分隔符直接內建到 OutputIterator,
// std::copy 一行寫完。同樣的 export_csv 可吃 cout、ofstream、ostringstream。
template <class OutStream>
void export_csv(const std::vector<int>& data, OutStream& os) {
    std::copy(data.begin(), data.end(),
              std::ostream_iterator<int>(os, ","));
}

void practical_csv_export() {
    std::vector<int> data{10, 20, 30, 40};
    std::ostringstream oss;
    export_csv(data, oss);
    std::cout << "CSV: " << oss.str() << " (期望 10,20,30,40,)\n";
    // 同一函式也可直接吐到 cout:
    std::cout << "Inline CSV: ";
    export_csv(data, std::cout);
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// 1, 2, 3, 4, 5,
// dst.size() = 5 (期望 5)
// 1 2 3 4 5
// LC88 merged: 1 2 3 4 5 6 7 8
// LC1929 concat: 1 2 1 1 2 1
// 錯誤 log 共 2 條 (期望 2)
// 錯誤 log 直接 dump:
// 2026-05-04 10:00:02 ERROR connect timeout
// 2026-05-04 10:00:04 ERROR auth failed
// 組成 email: 2026-05-04 10:00:02 ERROR connect timeout | 2026-05-04 10:00:04 ERROR auth failed |
