// =============================================================================
//  01_input_iterator.cpp  —  InputIterator
// =============================================================================
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼這個 iterator 存在?                              │
//  └────────────────────────────────────────────────────────────┘
//
//  STL 在設計時遇到一個基本問題:
//
//    「我寫了一個 std::find / std::accumulate / std::count,要怎麼讓它
//     既能吃 vector、也能吃從鍵盤一字一字讀進來的 std::cin?」
//
//  鍵盤輸入、socket、磁帶機、generator function 這類「資料來源」有共通限制:
//    * 「沒有索引」(沒有 it[5] 這種隨機跳)
//    * 「讀過就消失」(讀過的字元不會再回來)
//    * 「不能備份重走」(沒辦法存起來等等再讀第二遍)
//
//  STL 把「最寬鬆」的讀取需求抽象為 InputIterator:能 ++、能 *it 讀、
//  能 == 比較 (用來測 EOF)。把演算法的需求壓到這個等級,代表它的
//  「適用來源」範圍最大 — vector / list / set / istream_iterator / 自家來源
//  全部能用同一份程式碼。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、特性 (LegacyInputIterator)                             │
//  └────────────────────────────────────────────────────────────┘
//
//   * 「只能讀」+「單次走訪 (single-pass)」。
//   * 支援的操作:
//       *it          讀取目前元素 (右值)
//       it->m        若元素是物件,取成員 (等同於 (*it).m)
//       ++it / it++  前進 (走過去後就不能回頭、也不能再讀同一個元素)
//       it == it2    比較 (主要用來判斷是否已到 end)
//       it != it2    同上
//   * 不保證 it 走過去後再走一次還能讀到同樣的東西。
//   * 演算法範例:std::find, std::accumulate, std::count, std::for_each,
//                std::copy (來源端) — 這些底層只需要 InputIterator 即可。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、底層機制 — istream_iterator 是怎麼運作的?              │
//  └────────────────────────────────────────────────────────────┘
//
//  std::istream_iterator<T> 是最典型的 InputIterator,概念上的實作:
//
//      template <class T>
//      class istream_iterator {
//          std::istream* in_;       // null 代表「end iterator」
//          T value_;                // 當前讀到的值
//
//          istream_iterator() : in_(nullptr) {}             // end
//          istream_iterator(std::istream& s) : in_(&s) {    // begin
//              read_next();
//          }
//          void read_next() {
//              if (in_ && !(*in_ >> value_)) in_ = nullptr; // EOF → 變 end
//          }
//          const T& operator*() const { return value_; }
//          istream_iterator& operator++() { read_next(); return *this; }
//          bool operator==(const istream_iterator& o) const { return in_ == o.in_; }
//      };
//
//  關鍵觀察:
//   * 「end iterator」就是「in_ 是 nullptr」 — 沒辦法用「指到尾」這種概念,
//     因為串流沒有固定大小,只能用「無資料時就等於 end」表示。
//   * 解參考拿到的是「上一次 ++ 時讀進來的快取值」,不是當下重新讀 stream。
//     這就是為什麼「同一個 it 連續解參考兩次」拿到的是同一個值,
//     但「拷貝 it 後分別 ++」 — 兩條路看到的順序就會不一致 (multi-pass 沒保證)。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、常見來源                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * std::istream_iterator<T>      ← 最典型 (格式化讀,跳空白)
//   * std::istreambuf_iterator<C>   ← 純 byte 串流 (不跳空白,見 08b)
//   * 由 generator / coroutine 產生的「one-shot 序列」
//   * Forward / Bidirectional / RandomAccess iterator 都「向上相容」InputIterator
//     (因為強的 iterator 可當弱的用)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、何時使用                                               │
//  └────────────────────────────────────────────────────────────┘
//
//  * 寫泛型演算法或函式時,參數型別接 InputIterator 表示
//    「我只讀一遍、不會偷偷拷副本走第二遍」。這對呼叫者是最友善的 —
//    呼叫端可以塞最便宜的來源 (例如直接從 stream 接過來)。
//  * 範例:std::find / std::count / std::accumulate 全都只需 InputIterator。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 六、Pitfalls (陷阱)                                        │
//  └────────────────────────────────────────────────────────────┘
//
//   1. 不要對 InputIterator 做兩次走訪:
//        std::istringstream iss("1 2 3");
//        auto it = std::istream_iterator<int>(iss);
//        std::accumulate(it, std::istream_iterator<int>(), 0);
//        std::accumulate(it, std::istream_iterator<int>(), 0);  // ← 第二次什麼都讀不到
//      不一定會 crash,但行為不保證。
//   2. 不能 it += n、不能 it1 - it2 (那是 RandomAccess 才有的)。
//      想推進 n 步請用 std::advance(it, n) — 它對 InputIterator 退化成 O(n) 迴圈。
//   3. ++ 和 -- 後不要再回頭讀。對某些實作 (檔案 stream 等) 會直接拿到下一筆。
//   4. 別指望「對同一個位置呼叫兩次 *it 都得到同樣值」 — 規格只保證在
//      「兩次 *it 中間沒有 ++」的情況下會一致。
//   5. Most Vexing Parse (僅針對 istream_iterator):
//      std::vector<int> v(std::istream_iterator<int>(iss),  // ← 被當成函式宣告!
//                         std::istream_iterator<int>());
//      要用 {} 列表初始化、或多包一層括號才安全 (詳見 08_stream_iterators.cpp)。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 七、參考連結 (References)                                  │
//  └────────────────────────────────────────────────────────────┘
//
//    https://en.cppreference.com/w/cpp/named_req/InputIterator     — 規格定義
//    https://en.cppreference.com/w/cpp/iterator/istream_iterator    — 典型實例
//    https://en.cppreference.com/w/cpp/iterator/istreambuf_iterator — byte 串流
//    https://cplusplus.com/reference/iterator/InputIterator/        — 簡明說明
// =============================================================================

/*
補充筆記：input_iterator
  - input_iterator 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - input iterator 只保證單向讀取，常見於 stream；讀過的值可能不能再回頭讀。
  - 演算法若只需要 input iterator，就表示它不會多次走訪同一元素。
  - 不要假設 input iterator 可複製後各自獨立前進；stream iterator 複製後仍共享同一輸入來源。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】InputIterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. InputIterator 有什麼限制？為什麼說它是 single-pass？
//     答：只保證「讀一次、往前走一次」。iterator 一旦遞增，先前取得的值與副本就不保證仍
//         有效，所以同一個序列不能走第二遍。典型例子是 istream_iterator——資料從 stream
//         取出後就消耗掉了，無法回頭。
//     追問：那 std::find 對 InputIterator 可以用嗎？（可以，它只走一遍；但 std::sort 不行，
//         排序需要 random access）
//
// 🔥 Q2. Input 和 Forward iterator 的關鍵差別是什麼？
//     答：multi-pass guarantee。Forward 可以複製一份 iterator，兩份各自走訪會得到相同的
//         序列，也能記住某個位置稍後再回來用；Input 沒有這個保證，複製出來的副本在原
//         iterator 遞增後即失去意義。可讀寫與否是次要差別，multi-pass 才是分界線。
//     追問：哪些演算法需要 multi-pass？（任何需要走兩遍或回頭比對的，例如
//         std::adjacent_find、std::search、std::unique）
//
// ⚠️ 陷阱. InputIterator 的 it++ 回傳值可以直接拿來用嗎？
//     答：不保證。InputIterator 的 post-increment 只要求回傳可解參考一次的東西，標準並未
//         要求它像 forward iterator 那樣回傳一個功能完整的 iterator 副本。
//     為什麼會錯：大家習慣了 vector 的 iterator（random access）什麼都能做，就以為
//         *it++ 這類寫法在任何 iterator 上都同樣安全。對 input iterator 應該把它當成
//         「一次性讀取」看待。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>
#include <sstream>
#include <numeric>
#include <vector>
#include <algorithm>
#include <string>

int main() {
    // -----------------------------------------------------------------------
    // 範例 1:用 istream_iterator 從字串流讀整數累加
    //
    //   istream_iterator<int> 是經典的 InputIterator:
    //     begin: std::istream_iterator<int>(iss)
    //     end  : 預設建構的 std::istream_iterator<int>() 代表 EOF
    //
    //   std::accumulate 只需要 InputIterator → 完美對應。
    // -----------------------------------------------------------------------
    std::istringstream iss("3 1 4 1 5 9 2 6 5 3");
    int sum = std::accumulate(std::istream_iterator<int>(iss),
                              std::istream_iterator<int>(),
                              0);
    std::cout << "sum = " << sum << " (期望 39)\n";

    // -----------------------------------------------------------------------
    // 範例 2:拿 InputIterator 來建構 vector
    //   vector(InputIt first, InputIt last) 這個 ctor 也是「只需 InputIterator」
    //   ── 所以可直接用 istream_iterator 把 stream 倒進 vector。
    // -----------------------------------------------------------------------
    std::istringstream iss2("apple banana cherry");
    std::vector<std::string> words{
        std::istream_iterator<std::string>(iss2),
        std::istream_iterator<std::string>()
    };
    std::cout << "words: ";
    for (auto& w : words) std::cout << w << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1480_running_sum();
    void leetcode_136_single_number();
    void practical_count_words_from_stream();
    void leetcode_485_max_consecutive_ones();
    void practical_stream_avg_temperature();
    leetcode_1480_running_sum();
    leetcode_136_single_number();
    practical_count_words_from_stream();
    leetcode_485_max_consecutive_ones();
    practical_stream_avg_temperature();

    // -----------------------------------------------------------------------
    // 課程知識補充:為什麼 InputIterator 「這麼弱」?
    //
    //   * 設計初衷就是「能從未知串流讀資料」— 例如鍵盤輸入、socket、tape。
    //     這些來源「沒有索引」、「讀過就消失」、「不能回頭」。
    //   * 因此 STL 對它的要求很低:能 ++、能 *it (讀)、能 == 比較 (用來測 EOF)。
    //   * 把演算法寫到「只需 InputIterator」表示它「對輸入來源最寬容」—
    //     可以接 vector、可以接 list、也可以直接接 istream_iterator。這就是
    //     C++ 泛型程式碼的價值。
    //
    //   * 實作面:給 InputIterator 後不要做這些事情:
    //         - 不要拷貝 it 後拿副本走第二次 (multi-pass 沒保證)
    //         - 不要 it1 - it2 (沒有 difference)
    //         - 不要 it += n (沒有 random access)
    //
    //   * 為什麼 std::distance(input_it_begin, input_it_end) 還是有效?
    //     因為它退化成 while-loop 邊走邊數,代價 O(n)。
    //     對 RandomAccess iterator 它走 O(1) 直接相減 — tag dispatch 自動選。
    // -----------------------------------------------------------------------

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:為什麼 InputIterator 是「single-pass」?跟 ForwardIterator 的「multi-pass」差在哪?
    //    A:single-pass 表示資料來源不允許「拷貝 it 後再走第二次」 — 例如
    //      istream 讀過去的字元就消失了,不可能再讀回來。Forward 則保證「拷貝後
    //      兩條獨立的副本走訪相同序列會得到相同結果」,因此 std::adjacent_find /
    //      std::search / std::unique 這類需要回頭比對的演算法才能成立。
    //
    //  Q2:為什麼 istream_iterator 的「end iterator」用「預設建構」表示?
    //    A:串流沒有固定大小,不存在「指到尾」這個概念。STL 規範用「預設建構」
    //      做為 end-of-stream sentinel,當實作偵測到 EOF (operator>> 失敗) 後,
    //      內部 stream 指標設成 nullptr → 與預設建構的 it 比較相等 → 演算法停止。
    //      這是 InputIterator 設計上「比較 ==」的真正用途。
    //
    //  Q3:為什麼說 std::accumulate / std::find 接受 InputIterator 是「對呼叫端最寬容」?
    //    A:演算法需求等級越低,可接受的來源就越多。寫成 InputIterator 表示「我只讀
    //      一遍、不偷偷拷副本回頭走第二次」 — 呼叫端可以丟 vector、list、set,
    //      也可以直接接 istream_iterator (從鍵盤、socket 串流而來)。同一份程式碼
    //      就能應付所有可讀來源,這就是泛型程式設計的價值。
    //

    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1480: 一維陣列的累加和 (Running Sum of 1d Array)
// ----------------------------------------------------------------
// 題目:給陣列 nums,回傳 result 使得 result[i] = nums[0] + nums[1] + ... + nums[i]。
//      例如 nums = [1, 2, 3, 4] → [1, 3, 6, 10]。
//
// 為什麼這題對 InputIterator 完美:
//   * 整個演算法只需「從頭到尾讀一次」即可累加 — 完美對應 InputIterator 等級。
//   * 不需要回頭、不需要多次走訪。
//   * STL 提供現成 std::partial_sum,就是吃 InputIterator + OutputIterator,
//     直接一行解決。
//
// 解法核心:
//   * 累加器 acc = 0,逐元素 acc += *it,寫進輸出。
//   * 用 std::partial_sum 等同 (但更短)。
//
// 複雜度:時間 O(n);空間 O(n) (回傳新陣列)。
void leetcode_1480_running_sum() {
    auto running_sum = [](const std::vector<int>& nums) {
        std::vector<int> out;
        out.reserve(nums.size());
        int acc = 0;
        // 即使這裡用的是 vector::const_iterator (random_access),
        // 但「逐一讀過、不回頭」這個用法只需要 InputIterator 的能力。
        for (auto it = nums.begin(); it != nums.end(); ++it) {
            acc += *it;
            out.push_back(acc);
        }
        return out;
    };

    std::vector<int> nums = {1, 2, 3, 4};
    auto rs = running_sum(nums);
    std::cout << "LC1480 running sum: ";
    for (int x : rs) std::cout << x << ' ';   // 1 3 6 10
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 136: Single Number
// ----------------------------------------------------------------
// 題目:陣列裡每個數字都出現兩次,只有一個出現一次,找出它。
//      限制:O(n) 時間、O(1) 額外空間。
//
// 為什麼這題對 InputIterator 「完美」?
//   * 整個演算法只需要從頭到尾「走過一次」,每個元素只看一次。
//   * 不需要回頭、不需要記住前面的元素 (XOR 自動累積)。
//   * 所以 std::accumulate 用 InputIterator 等級的迭代器即可。
//
// 解法核心 (XOR trick):
//   * a ^ a = 0、a ^ 0 = a、XOR 滿足交換律與結合律。
//   * 把陣列全部 XOR 起來,出現兩次的兩兩抵消為 0,
//     剩下的就是那個「只出現一次」的數。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_136_single_number() {
    auto single_number = [](const std::vector<int>& v) {
        // accumulate 的初始值 0 ^ x = x,所以從 0 開始 XOR 一路到底
        return std::accumulate(v.begin(), v.end(), 0,
                               [](int acc, int x){ return acc ^ x; });
    };
    std::vector<int> sn = {4, 1, 2, 1, 2};
    std::cout << "LC136 single number = " << single_number(sn)
              << " (期望 4)\n";
}

// ----------------------------------------------------------------
// 實務範例:從文字流統計關鍵字出現次數
// ----------------------------------------------------------------
// 場景:處理 log 檔或 socket 串流,要算「ERROR」出現幾次。
//      資料來源是 stream,不該全部讀進記憶體;用 istream_iterator
//      搭 std::count,「邊讀邊數」,memory footprint 維持 O(1)。
//
// 重點:
//   * std::count 只要 InputIterator,所以可以直接吃 istream_iterator。
//   * 對檔案處理或無限串流,這比「先讀進 vector 再 count」省記憶體。
void practical_count_words_from_stream() {
    std::istringstream stream(
        "INFO startup INFO ready ERROR timeout WARN retry "
        "ERROR auth INFO done ERROR oom"
    );
    auto n = std::count(std::istream_iterator<std::string>(stream),
                        std::istream_iterator<std::string>(),
                        std::string("ERROR"));
    std::cout << "ERROR 出現次數 = " << n << " (期望 3)\n";
}

// ----------------------------------------------------------------
// LeetCode 485: Max Consecutive Ones (連續 1 的最大長度)
// ----------------------------------------------------------------
// 題目:給一個 0/1 陣列,回傳「連續 1」的最大長度。
//
// 為什麼這題對 InputIterator 完美:
//   * 只需要「從頭到尾走一次」,維護兩個變數 cur (當前連續長度) 與 best (歷史最大)。
//   * 不需要拷貝 iterator、不需要回頭。
//   * 改用 std::for_each (吃 InputIterator) 也能寫 — 真實場景甚至可直接吃 istream_iterator
//     處理一條「持續來的二進位串流」。
//
// 解法核心:邊掃描邊維護 cur,遇 0 歸零,遇 1 累加。
// 複雜度:時間 O(n)、空間 O(1)。
void leetcode_485_max_consecutive_ones() {
    std::vector<int> nums{1, 1, 0, 1, 1, 1, 0, 1};
    int cur = 0, best = 0;
    // 故意用最弱的 InputIterator 寫法:逐元素讀過去
    for (auto it = nums.begin(); it != nums.end(); ++it) {
        cur = (*it == 1) ? cur + 1 : 0;
        if (cur > best) best = cur;
    }
    std::cout << "LC485 max consecutive ones = " << best << " (期望 3)\n";
}

// ----------------------------------------------------------------
// 實務範例:溫度感測器串流的即時平均值
// ----------------------------------------------------------------
// 場景:IoT 裝置每秒從 socket 收到一筆 double 溫度,要計算「即時累計平均」。
// 因為資料來自串流、不能整批存進記憶體,只能逐筆讀過 — 完美契合 InputIterator。
// 用 istream_iterator<double> 包住輸入流,搭配 accumulate 即可邊讀邊累加。
void practical_stream_avg_temperature() {
    std::istringstream stream("22.5 23.1 22.8 24.0 23.5");
    std::istream_iterator<double> it(stream), end;
    double sum = 0.0;
    int count = 0;
    while (it != end) {
        sum += *it;
        ++count;
        ++it;
    }
    double avg = (count > 0) ? sum / count : 0.0;
    std::cout << "感測器平均溫度 = " << avg << " (期望 ~23.18)\n";
}

// === 預期輸出 (Expected output) ===
// sum = 39 (期望 39)
// words: apple banana cherry
// LC1480 running sum: 1 3 6 10
// LC136 single number = 4 (期望 4)
// ERROR 出現次數 = 3 (期望 3)
// LC485 max consecutive ones = 3 (期望 3)
// 感測器平均溫度 = 23.18 (期望 ~23.18)
