// =============================================================================
//  第 22 課 總結：deque 的宣告與初始化  —  本課的教科書
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：#include <deque>
//   類別：  template<class T, class Allocator = std::allocator<T>> class deque;
//           （deque = Double-Ended QUEue，唸作 "deck"）
//
//   ┌─ 建構與初始化的完整清單 ────────────────────────────────────────────┐
//   │  deque<int> d;                      (1) 預設        O(1)            │
//   │  deque<int> d(5);                   (2) n 個 T{}    O(n)  explicit  │
//   │  deque<int> d(4, 77);               (3) n 個 v      O(n)            │
//   │  deque<int> d = {1,2,3};            (4) init-list   O(n)  C++11     │
//   │  deque<int> d2(d1);                 (5) 複製        O(n)            │
//   │  deque<int> d2(std::move(d1));      (6) 移動        O(1)  C++11     │
//   │  deque<int> d(v.begin(), v.end());  (7) 迭代器範圍  O(n)            │
//   │  d.assign(...);                     (8) 重新指定    O(n)            │
//   │  deque d{1,2,3};                    (9) CTAD        C++17           │
//   └──────────────────────────────────────────────────────────────────────┘
//
//   標準版本（本機以 -pedantic-errors 逐一驗證，g++ 15.2.0）：
//     移動建構 / initializer_list → C++11
//     CTAD（免寫 <int>）          → C++17（C++14 下編譯失敗，已驗證）
//
// 【詳細解釋 Explanation】
//
// 【1. deque 的定位：介面像 vector，結構像「分段的 vector」】
// STL 三大序列容器的分工：
//     vector —— 連續記憶體，尾端 O(1)，隨機存取最快，中間/頭端 O(n)
//     deque  —— 分段記憶體，**頭尾都 O(1)**，隨機存取次快，中間 O(n)
//     list   —— 節點鏈結，任意位置 O(1) 插刪（但要先有迭代器），無隨機存取
// deque 存在的唯一理由就是那個粗體字：**頭端也要 O(1)**。
// 如果你的資料只從尾端進出，vector 永遠是更好的選擇（更快、更省、cache 更友善）。
//
// 【2. map + chunk：兩層結構如何同時做到「頭尾 O(1)」與「隨機存取」】
//
//     map（中央控制陣列，連續的 T** 指標陣列）
//     ┌──────┬──────┬──────┬──────┬──────┐
//     │ 空位 │  p1  │  p2  │  p3  │ 空位 │  ← 兩端刻意留空，供未來擴張
//     └──────┴──┬───┴──┬───┴──┬───┴──────┘
//               │      │      │
//               ▼      ▼      ▼
//            [chunk][chunk][chunk]           ← 每個 chunk 內部連續，chunk 之間不相鄰
//             ▲                   ▲
//           start               finish       ← 兩個迭代器標記有效範圍
//
//   為什麼頭端也能 O(1)：
//     push_front 只要在 start 所在的 chunk 前面還有空位就直接放；
//     沒空位就新配一個 chunk，把指標掛到 map 的前一格。
//     **完全不需要搬動任何既有元素** —— 這是與 vector 的根本差異。
//   為什麼還能隨機存取（d[i] 是 O(1)）：
//     因為每個 chunk 大小固定，位置可以直接算出來：
//         chunk 編號 = (start 偏移 + i) / 每chunk元素數
//         chunk 內位移 = (start 偏移 + i) % 每chunk元素數
//     兩次除法 + 兩次解參考 → O(1)，但常數比 vector 的「一次加法」大不少。
//   代價：
//     不連續 → **沒有 data()、沒有 capacity()、沒有 reserve()**。
//
// 【3. 小括號 vs 大括號：C++ 最惡名昭彰的初始化陷阱】
//     deque<int> d8(5, 10);   // 5 個 10        → size=5
//     deque<int> d9{5, 10};   // 元素 5 和 10   → size=2
//   規則只有一條，但很絕對：
//     **只要 initializer_list 建構子「能」匹配，大括號就一定選它。**
//   即使有其他「看起來更合適」的建構子也一樣。這是 C++11 為了讓大括號語意
//   可預測而定下的硬規則。
//   最惡毒的地方是它**跟元素型別有關**：
//     deque<int>    d{5, "x"};  // 編譯錯誤
//     deque<int>    d{5, 10};   // → 2 個元素（int 能組成 init-list）
//     deque<string> d{5, "x"};  // → 5 個 "x"（5 不能轉 string，init-list 匹配失敗
//                               //   → 退回 (size, value) 建構子）
//   同樣的括號寫法，換個元素型別語意就翻轉。本檔 main() 有實測驗證。
//
// 【4. assign：建構之後「整批換掉內容」】
//   assign 不是建構子，但它提供了與建構子平行的三種形式：
//       d.assign(n, v);              // 變成 n 個 v
//       d.assign(first, last);       // 變成迭代器範圍的內容
//       d.assign({1, 2, 3});         // 變成 init-list 的內容
//   它會先清掉原有元素再填入，等效於「重新初始化」但重用已配置的 chunk，
//   比「解構後重建」省一些配置成本。
//
// 【5. 容器的複製語意：深或淺由「元素型別」決定】
//   容器複製 = 對每個元素呼叫**元素型別的**複製建構子。容器本身不做決定：
//       deque<int>                → 複製 int 值             → 表現為深複製
//       deque<string>             → string 自己深複製緩衝區 → 深
//       deque<int*>               → 只複製「位址」          → **淺**，危險
//       deque<shared_ptr<T>>      → 引用計數 +1             → 共享但安全
//       deque<unique_ptr<T>>      → **不可複製**，編譯失敗  → 最安全
//   裸指標版本的三大 bug：意外別名、double free（未定義行為）、懸空指標。
//   詳見本課 2.cpp。
//
// 【概念補充 Concept Deep Dive】
//
// (A) chunk 大小完全是實作定義的（必背的「標明實作」範例）
//   標準對 chunk 大小**隻字未提**。本機 libstdc++ 15.2.0 實測規則：
//       chunk 位元組 = 512，每 chunk 元素數 = max(1, 512 / sizeof(T))
//   實測對照表：
//       sizeof(T)=1(char)   → 512 個/chunk
//       sizeof(T)=4(int)    → 128 個/chunk
//       sizeof(T)=8(double) →  64 個/chunk
//       sizeof(T)=256       →   2 個/chunk
//       sizeof(T)=600       →   1 個/chunk  ← 超過 512 就退化成一格一個
//   其他實作差異極大：MSVC STL 用 16 bytes，libc++ 用 4096 bytes。
//   → 任何提到「512」的說法都必須加上「在 libstdc++ 上」。
//
// (B) 空 deque 也會配置記憶體（vector / list 都不會）
//   本機用覆寫 operator new 攔截實測：
//       空 deque<int>  → **2 次配置，共 576 bytes**
//       空 vector<int> → 0 次配置
//       空 list<int>   → 0 次配置
//   576 = 512（先開一個 chunk）+ 64（map 初始 8 根指標 × 8 bytes）。
//   原因是 deque 的迭代器實作需要「隨時有一個合法的落點」讓 begin()/end()
//   指得到，所以連空的也先開一個 chunk。這是**實作細節、非標準保證**。
//   實務影響：如果你要開「幾十萬個大多是空的容器」（例如 vector<deque<int>>
//   當鄰接表），deque 的固定成本會非常可觀 —— 這種場合請改用 vector。
//   本機 sizeof 實測：deque<int>=80、vector<int>=24、list<int>=24 bytes。
//
// (C) 為什麼移動建構是 O(1) 而且 noexcept
//   移動只搬 map 指標與 start/finish 兩個迭代器，然後把來源重設為空狀態，
//   **完全不碰任何元素**。所以無論裡面有 3 個還是 300 萬個元素都一樣快。
//   noexcept 很重要：它讓 deque 放進 vector 時，vector 擴容能安全地用移動
//   而不是複製（vector 只在移動 noexcept 時才敢用移動，否則為了異常安全
//   會退回複製）。
//
// (D) 從迭代器範圍建構時的一個效能細節
//   deque(first, last) 若傳入的是 forward iterator 以上（vector/list/set 的
//   迭代器都是），實作可以先算出距離、一次配置好足夠的 chunk。
//   若傳入的是 input iterator（例如 istream_iterator，只能走一次），
//   就只能一個一個 push_back，過程中可能多次配置。這在讀大型輸入時有感。
//
// 【注意事項 Pay Attention】
// 1. deque **沒有** data() / capacity() / reserve()，但**有** shrink_to_fit()。
//    （本機以 SFINAE 編譯期偵測逐一確認，見 1.cpp）
// 2. 被移動後的來源處於「有效但未指定」狀態。libstdc++ 實測為空，
//    但**標準不保證**，程式不可依賴這點。
// 3. deque<int> d(5) 的元素是 value-initialized（int 得到 0），不是垃圾值。
// 4. (2) 是 explicit：deque<int> d = 5; 編譯失敗，必須寫 d(5)。
// 5. double free 是**未定義行為**，不是「一定 crash」。可能被 glibc 偵測並
//    abort，也可能靜默破壞 heap 讓程式在很久之後的別處崩潰。
// 6. 想要「n 個 v」一律用小括號；大括號只用在「就是列這幾個元素」。
// 7. 只從尾端進出的資料請用 vector。deque 的頭端 O(1) 不是免費的 ——
//    它換來的是更差的 cache 局部性、更大的固定成本與更慢的走訪。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 的宣告、初始化與底層結構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. deque 底層是什麼結構？為什麼它能做到「頭尾都 O(1) 且支援隨機存取」？
//     答：map（一個連續的指標陣列）+ 多個固定大小的 chunk。元素放在 chunk 裡，
//         chunk 之間不相鄰。頭尾插入只需在 map 兩端掛新 chunk，**不搬動既有元素**
//         → O(1)。隨機存取則因為 chunk 大小固定，可用除法/取餘直接算出
//         「第幾個 chunk、chunk 內第幾格」→ O(1)，但常數比 vector 大。
//     追問：那 deque 的隨機存取跟 vector 一樣快嗎？
//         → 不一樣。vector 是一次加法定址；deque 要一次除法、一次取餘、
//           兩次解參考，而且跨 chunk 走訪對 CPU 預取不友善。
//           複雜度同為 O(1)，常數差很多。
//
// 🔥 Q2. deque 少了哪些 vector 有的成員函式？為什麼？
//     答：data()、capacity()、reserve()。因為 deque **不保證連續記憶體**：
//         沒有單一緩衝區就沒有「容量」概念，也無法回傳一根涵蓋全部元素的指標。
//         但 shrink_to_fit() 是**有的**（它能歸還用不到的 chunk 與 map 空間）。
//     追問：那要把 deque 內容交給只吃 const int* 的 C API 怎麼辦？
//         → 先複製到連續容器：vector<int> v(d.begin(), d.end()); 再傳 v.data()。
//           沒有零成本的做法 —— 這正是選 deque 的代價。
//
// 🔥 Q3. deque<int> d(5, 10) 和 deque<int> d{5, 10} 差在哪？
//     答：(5,10) → 5 個 10（size=5）；{5,10} → 兩個元素 5 和 10（size=2）。
//         規則：只要 initializer_list 建構子能匹配，大括號就一定選它。
//     追問：deque<string> d{5, "x"} 呢？
//         → 5 個 "x"。因為 5 無法轉成 string，initializer_list<string> 匹配失敗，
//           才退回 (size, value) 建構子。**同樣的括號、換個元素型別語意就翻轉**，
//           這是它真正危險的原因。
//
// 🔥 Q4. deque 的複製建構是深複製嗎？
//     答：問法本身有陷阱。容器複製 = 對每個元素呼叫元素型別的複製建構子。
//         deque<int>/deque<string> 表現為深；deque<int*> 只複製位址 → 淺，
//         會導致意外別名與 double free（未定義行為）；deque<unique_ptr<T>>
//         則根本無法複製（編譯期就擋下來）。深淺由**元素型別**決定。
//     追問：那正確的做法是什麼？
//         → 能存值就存值；需要共享所有權用 shared_ptr；獨佔用 unique_ptr。
//           讓所有權寫進型別裡，而不是寫在文件或某個人的記憶裡。
//
// ⚠️ 陷阱. 「空的容器都不會配置記憶體，所以大量建立空 deque 是免費的」——錯在哪？
//     答：**空 deque 會配置**。本機攔截 operator new 實測：空 deque<int> 建構
//         就發生 2 次配置、共 576 bytes（512 的 chunk + 64 的 map），
//         而空 vector<int> 與空 list<int> 都是 0 次。
//         所以 vector<deque<int>> adj(100000) 這種鄰接表寫法，光是空容器
//         就吃掉約 57 MB；換成 vector<vector<int>> 則幾乎不花錢。
//     為什麼會錯：一般人把「空容器 = 沒東西 = 不用配置」當成通則，
//         那對 vector/list 成立，但 deque 的迭代器設計需要「隨時有個合法落點」，
//         所以連空的都得先開一個 chunk。這是實作細節而非標準保證，
//         但三大實作（libstdc++/libc++/MSVC）在這點上都會預先配置。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <vector>
#include <list>
#include <string>
#include <memory>
#include <type_traits>
using namespace std;

void print(const string& label, const deque<int>& dq) {
    cout << "  " << label << ": ";
    for (int val : dq) cout << val << " ";
    cout << "(size=" << dq.size() << ")" << endl;
}

// SFINAE 偵測：編譯期檢查某成員函式是否存在
template <typename T, typename = void> struct has_data : false_type {};
template <typename T> struct has_data<T, void_t<decltype(declval<T&>().data())>> : true_type {};
template <typename T, typename = void> struct has_capacity : false_type {};
template <typename T> struct has_capacity<T, void_t<decltype(declval<T&>().capacity())>> : true_type {};
template <typename T, typename = void> struct has_reserve : false_type {};
template <typename T> struct has_reserve<T, void_t<decltype(declval<T&>().reserve(1))>> : true_type {};
template <typename T, typename = void> struct has_shrink : false_type {};
template <typename T> struct has_shrink<T, void_t<decltype(declval<T&>().shrink_to_fit())>> : true_type {};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 933. Number of Recent Calls
//   題目：實作 RecentCounter，每次 ping(t) 回傳「過去 3000 毫秒內」的請求數
//         （即區間 [t-3000, t] 內的請求數）。t 保證嚴格遞增。
//   為什麼用到本主題：這是最乾淨的「滑動視窗佇列」——新請求從**尾端**進來，
//         過期請求從**頭端**淘汰。兩端都要 O(1)，正是 deque 的定位。
//         而且容器是空的開始、動態成長，用不到 reserve —— 剛好避開 deque 的限制。
//   複雜度：每次 ping 攤還 O(1)（每個元素最多進出各一次）。
// -----------------------------------------------------------------------------
class RecentCounter {
    deque<int> q_;   // 建構時預設建構即可，這題不需要預先指定容量
public:
    RecentCounter() = default;

    int ping(int t) {
        q_.push_back(t);                       // 尾端進：O(1)
        while (!q_.empty() && q_.front() < t - 3000) {
            q_.pop_front();                    // 頭端出：O(1)，vector 這裡是 O(n)
        }
        return static_cast<int>(q_.size());
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 641. Design Circular Deque
//   題目：設計固定容量的雙端佇列，支援頭尾插入/刪除、取頭尾、判空判滿。
//   為什麼用到本主題：這題逼你面對「deque 沒有 capacity()」這件事 ——
//         容量上限必須自己用成員變數維護。同時四種端點操作全是 O(1)，
//         正好把 deque 的核心能力用滿。
//   複雜度：所有操作 O(1)。
// -----------------------------------------------------------------------------
class MyCircularDeque {
    deque<int> dq_;
    size_t     cap_;
public:
    explicit MyCircularDeque(int k) : cap_(static_cast<size_t>(k)) {}
    bool insertFront(int v) { if (isFull()) return false; dq_.push_front(v); return true; }
    bool insertLast(int v)  { if (isFull()) return false; dq_.push_back(v);  return true; }
    bool deleteFront()      { if (isEmpty()) return false; dq_.pop_front();  return true; }
    bool deleteLast()       { if (isEmpty()) return false; dq_.pop_back();   return true; }
    int  getFront() const   { return isEmpty() ? -1 : dq_.front(); }
    int  getRear()  const   { return isEmpty() ? -1 : dq_.back(); }
    bool isEmpty()  const   { return dq_.empty(); }
    bool isFull()   const   { return dq_.size() >= cap_; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 1】應用程式的「最近開啟檔案」清單（MRU list）
//   情境：編輯器要記住使用者最近開過的 10 個檔案，顯示在「檔案」選單裡。
//         最新開的排最前面；超過 10 個就把最舊的擠掉；重複開同一個檔案時
//         要把它移到最前面而不是重複列出。
//   為什麼用 deque：新項目從頭端進（push_front，O(1)）、舊項目從尾端淘汰
//         （pop_back，O(1)）。用 vector 的話 insert(begin()) 是 O(n)。
//   注意：這裡用「建構時指定上限」的模式 —— 正是本課建構知識的實際落點。
// -----------------------------------------------------------------------------
class RecentFiles {
    deque<string> files_;
    size_t        limit_;
public:
    explicit RecentFiles(size_t limit) : limit_(limit) {}

    void open(const string& path) {
        // 若已存在就先移除（避免重複列出）
        for (auto it = files_.begin(); it != files_.end(); ++it) {
            if (*it == path) { files_.erase(it); break; }
        }
        files_.push_front(path);               // 最新的排最前：O(1)
        if (files_.size() > limit_) {
            files_.pop_back();                 // 淘汰最舊的：O(1)
        }
    }
    void dump() const {
        cout << "    選單:";
        for (const auto& f : files_) cout << " " << f;
        cout << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從設定檔文字建立初始白名單（迭代器範圍建構的實際用途）
//   情境：伺服器啟動時讀取一行以逗號分隔的 IP 白名單設定，
//         轉成容器供後續查詢。這是「從既有序列一次建構容器」的典型場合。
//   為什麼用到本主題：示範建構子 (7) 迭代器範圍 與 (8) assign 的實際用法 ——
//         先把字串切成 vector，再用 range 建構子一次灌進 deque，
//         之後熱更新設定時用 assign 整批換掉，不必重建物件。
// -----------------------------------------------------------------------------
vector<string> splitByComma(const string& line) {
    vector<string> out;
    size_t start = 0;
    while (start <= line.size()) {
        size_t comma = line.find(',', start);
        if (comma == string::npos) {
            out.push_back(line.substr(start));
            break;
        }
        out.push_back(line.substr(start, comma - start));
        start = comma + 1;
    }
    return out;
}

int main() {
    cout << "========== 一、九種初始化方式 ==========" << endl;

    deque<int> d1;
    print("1. 預設建構    ", d1);

    deque<int> d2(5);
    print("2. 五個預設值  ", d2);

    deque<int> d3(4, 77);
    print("3. 四個 77     ", d3);

    deque<int> d4 = {10, 20, 30, 40, 50};
    print("4. 初始化列表  ", d4);

    deque<int> d5(d4);
    d5[0] = 999;
    print("5a. 修改 d5    ", d5);
    print("5b. d4 不受影響", d4);

    deque<int> d6(move(d5));
    print("6a. d6 移動自d5", d6);
    print("6b. d5 被掏空  ", d5);   // libstdc++ 實測；標準只保證「有效但未指定」

    vector<int> vec = {100, 200, 300};
    deque<int> d7(vec.begin(), vec.end());
    print("7. 從 vector   ", d7);

    d7.assign({7, 8, 9, 10});
    print("8. assign 後   ", d7);

    deque d10{1, 2, 3};              // (9) CTAD：C++17 起可省略 <int>
    print("9. CTAD 推導   ", d10);

    cout << "\n========== 二、小括號 vs 大括號（最惡名昭彰的陷阱）==========" << endl;
    deque<int> d8(5, 10);
    deque<int> d9{5, 10};
    print("d8 = deque<int>(5,10)", d8);
    print("d9 = deque<int>{5,10}", d9);

    // 換成 string，同樣的大括號語意就翻轉
    deque<string> s1{5, "x"};        // 5 不能轉 string → init-list 匹配失敗 → 退回 (n,v)
    cout << "  d = deque<string>{5,\"x\"} → size=" << s1.size() << "，內容:";
    for (const auto& s : s1) cout << " " << s;
    cout << endl;
    cout << "  ★ 同樣寫 {5, ...}：int 版得到 2 個元素，string 版得到 5 個！" << endl;

    cout << "\n========== 三、deque 沒有哪些 vector 的成員（SFINAE 編譯期實測）==========" << endl;
    cout << boolalpha;
    cout << "                    data()   capacity()  reserve()  shrink_to_fit()" << endl;
    cout << "  deque<int>  :     "
         << has_data<deque<int>>::value      << "    "
         << has_capacity<deque<int>>::value  << "       "
         << has_reserve<deque<int>>::value   << "      "
         << has_shrink<deque<int>>::value    << endl;
    cout << "  vector<int> :     "
         << has_data<vector<int>>::value     << "     "
         << has_capacity<vector<int>>::value << "        "
         << has_reserve<vector<int>>::value  << "       "
         << has_shrink<vector<int>>::value   << endl;
    cout << "  ★ deque 不連續 → 沒有 data/capacity/reserve，但有 shrink_to_fit" << endl;

    cout << "\n========== 四、容器本體大小與元素型別的關係 ==========" << endl;
    cout << "  sizeof(deque<int>)  = " << sizeof(deque<int>)  << " bytes" << endl;
    cout << "  sizeof(vector<int>) = " << sizeof(vector<int>) << " bytes" << endl;
    cout << "  sizeof(list<int>)   = " << sizeof(list<int>)   << " bytes" << endl;
    cout << "  （皆為本機 libstdc++ 15.2.0 x86-64 實測，實作定義）" << endl;

    cout << "\n========== 五、複製語意由「元素型別」決定 ==========" << endl;
    {
        // (a) 存值 → 表現為深複製
        deque<int> a = {1, 2, 3};
        deque<int> b = a;
        b[0] = 999;
        cout << "  deque<int>  : a[0]=" << a[0] << " b[0]=" << b[0]
             << "  → 互不影響（深）" << endl;

        // (b) 存裸指標 → 淺複製，兩邊共用同一塊 heap
        deque<int*> p1;
        p1.push_back(new int(42));
        deque<int*> p2 = p1;
        *p2[0] = 777;
        cout << "  deque<int*> : *p1[0]=" << *p1[0]
             << "  → 被 p2 改到了（淺，且有 double free 風險）" << endl;
        for (int* p : p1) delete p;     // 全程只能 delete 一次

        // (c) shared_ptr → 共享但安全
        deque<shared_ptr<int>> s2;
        s2.push_back(make_shared<int>(42));
        deque<shared_ptr<int>> s3 = s2;
        cout << "  shared_ptr  : use_count=" << s2[0].use_count()
             << "  → 共享，引用計數自動管理生命週期" << endl;

        // (d) unique_ptr → 根本不能複製
        deque<unique_ptr<int>> u1;
        u1.push_back(make_unique<int>(42));
        // deque<unique_ptr<int>> u2 = u1;   // ← 編譯失敗（複製建構子被 deleted）
        deque<unique_ptr<int>> u2 = move(u1);
        cout << "  unique_ptr  : 複製會編譯失敗；移動後 u1.size()=" << u1.size()
             << " u2.size()=" << u2.size() << endl;
        cout << "  ★ 容器不決定深淺，元素型別才決定" << endl;
    }

    cout << "\n========== 六、LeetCode 933. Number of Recent Calls ==========" << endl;
    {
        RecentCounter rc;
        const int pings[] = {1, 100, 3001, 3002};
        for (int t : pings) {
            cout << "  ping(" << t << ") → " << rc.ping(t) << endl;
        }
        cout << "  （t=3001 時 t-3000=1，1 未小於 1 故保留；t=3002 時淘汰掉 1）" << endl;
    }

    cout << "\n========== 七、LeetCode 641. Design Circular Deque ==========" << endl;
    {
        MyCircularDeque cd(3);
        cout << "  insertLast(1)  → " << cd.insertLast(1)  << endl;
        cout << "  insertLast(2)  → " << cd.insertLast(2)  << endl;
        cout << "  insertFront(3) → " << cd.insertFront(3) << endl;
        cout << "  insertFront(4) → " << cd.insertFront(4) << "  (已滿)" << endl;
        cout << "  getRear()      → " << cd.getRear()      << endl;
        cout << "  isFull()       → " << cd.isFull()       << endl;
        cout << "  deleteLast()   → " << cd.deleteLast()   << endl;
        cout << "  getFront()     → " << cd.getFront()     << endl;
    }

    cout << "\n========== 八、日常實務：最近開啟檔案清單 ==========" << endl;
    {
        RecentFiles mru(4);
        const char* opened[] = {"main.cpp", "utils.h", "README.md",
                                "Makefile", "test.cpp", "utils.h"};
        for (const char* f : opened) {
            cout << "  開啟 " << f << endl;
            mru.open(f);
            mru.dump();
        }
        cout << "  ★ 重開 utils.h 時它被移到最前面，而不是重複列出" << endl;
    }

    cout << "\n========== 九、日常實務：從設定檔建立白名單 ==========" << endl;
    {
        const string line = "10.0.0.1,10.0.0.2,192.168.1.100,127.0.0.1";
        vector<string> parts = splitByComma(line);

        // 建構子 (7)：從迭代器範圍一次建好
        deque<string> allowlist(parts.begin(), parts.end());
        cout << "  設定字串: " << line << endl;
        cout << "  白名單 (" << allowlist.size() << " 筆):";
        for (const auto& ip : allowlist) cout << " " << ip;
        cout << endl;

        // 熱更新：用 assign 整批換掉，物件本身不重建
        vector<string> updated = splitByComma("10.0.0.9,172.16.0.1");
        allowlist.assign(updated.begin(), updated.end());
        cout << "  熱更新後 (" << allowlist.size() << " 筆):";
        for (const auto& ip : allowlist) cout << " " << ip;
        cout << endl;
        cout << "  ★ 新增管理者 IP 用 push_front，撤銷最舊的用 pop_back，都是 O(1)" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ========== 一、九種初始化方式 ==========
//   1. 預設建構    : (size=0)
//   2. 五個預設值  : 0 0 0 0 0 (size=5)
//   3. 四個 77     : 77 77 77 77 (size=4)
//   4. 初始化列表  : 10 20 30 40 50 (size=5)
//   5a. 修改 d5    : 999 20 30 40 50 (size=5)
//   5b. d4 不受影響: 10 20 30 40 50 (size=5)
//   6a. d6 移動自d5: 999 20 30 40 50 (size=5)
//   6b. d5 被掏空  : (size=0)
//   7. 從 vector   : 100 200 300 (size=3)
//   8. assign 後   : 7 8 9 10 (size=4)
//   9. CTAD 推導   : 1 2 3 (size=3)
//
// ========== 二、小括號 vs 大括號（最惡名昭彰的陷阱）==========
//   d8 = deque<int>(5,10): 10 10 10 10 10 (size=5)
//   d9 = deque<int>{5,10}: 5 10 (size=2)
//   d = deque<string>{5,"x"} → size=5，內容: x x x x x
//   ★ 同樣寫 {5, ...}：int 版得到 2 個元素，string 版得到 5 個！
//
// ========== 三、deque 沒有哪些 vector 的成員（SFINAE 編譯期實測）==========
//                     data()   capacity()  reserve()  shrink_to_fit()
//   deque<int>  :     false    false       false      true
//   vector<int> :     true     true        true       true
//   ★ deque 不連續 → 沒有 data/capacity/reserve，但有 shrink_to_fit
//
// ========== 四、容器本體大小與元素型別的關係 ==========
//   sizeof(deque<int>)  = 80 bytes
//   sizeof(vector<int>) = 24 bytes
//   sizeof(list<int>)   = 24 bytes
//   （皆為本機 libstdc++ 15.2.0 x86-64 實測，實作定義）
//
// ========== 五、複製語意由「元素型別」決定 ==========
//   deque<int>  : a[0]=1 b[0]=999  → 互不影響（深）
//   deque<int*> : *p1[0]=777  → 被 p2 改到了（淺，且有 double free 風險）
//   shared_ptr  : use_count=2  → 共享，引用計數自動管理生命週期
//   unique_ptr  : 複製會編譯失敗；移動後 u1.size()=0 u2.size()=1
//   ★ 容器不決定深淺，元素型別才決定
//
// ========== 六、LeetCode 933. Number of Recent Calls ==========
//   ping(1) → 1
//   ping(100) → 2
//   ping(3001) → 3
//   ping(3002) → 3
//   （t=3001 時 t-3000=1，1 未小於 1 故保留；t=3002 時淘汰掉 1）
//
// ========== 七、LeetCode 641. Design Circular Deque ==========
//   insertLast(1)  → true
//   insertLast(2)  → true
//   insertFront(3) → true
//   insertFront(4) → false  (已滿)
//   getRear()      → 2
//   isFull()       → true
//   deleteLast()   → true
//   getFront()     → 3
//
// ========== 八、日常實務：最近開啟檔案清單 ==========
//   開啟 main.cpp
//     選單: main.cpp
//   開啟 utils.h
//     選單: utils.h main.cpp
//   開啟 README.md
//     選單: README.md utils.h main.cpp
//   開啟 Makefile
//     選單: Makefile README.md utils.h main.cpp
//   開啟 test.cpp
//     選單: test.cpp Makefile README.md utils.h
//   開啟 utils.h
//     選單: utils.h test.cpp Makefile README.md
//   ★ 重開 utils.h 時它被移到最前面，而不是重複列出
//
// ========== 九、日常實務：從設定檔建立白名單 ==========
//   設定字串: 10.0.0.1,10.0.0.2,192.168.1.100,127.0.0.1
//   白名單 (4 筆): 10.0.0.1 10.0.0.2 192.168.1.100 127.0.0.1
//   熱更新後 (2 筆): 10.0.0.9 172.16.0.1
//   ★ 新增管理者 IP 用 push_front，撤銷最舊的用 pop_back，都是 O(1)
