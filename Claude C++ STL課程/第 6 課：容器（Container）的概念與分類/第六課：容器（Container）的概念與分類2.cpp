// =============================================================================
//  第六課 2 — std::vector：可動態成長的連續記憶體陣列
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T, class Allocator = std::allocator<T>> class vector;
//                                                       // <vector>,C++98 起
//
//   常用介面與複雜度：
//     size_type size()      const noexcept;   // O(1),目前有幾個元素
//     size_type capacity()  const noexcept;   // O(1),不重新配置最多能放幾個
//     void      reserve(size_type n);         // 預留容量,可能重新配置
//     void      shrink_to_fit();              // 請求歸還多餘容量(非強制)
//     void      push_back(const T&);          // 攤提(amortized) O(1)
//     void      pop_back();                   // O(1),不會縮小 capacity
//     reference operator[](size_type n);      // O(1),不做邊界檢查
//     reference at(size_type n);              // O(1),越界丟 std::out_of_range
//     iterator  insert(const_iterator, const T&);  // O(n),要搬移後面的元素
//     iterator  erase(const_iterator);             // O(n),要搬移後面的元素
//     T*        data() noexcept;              // 指向連續記憶體的首位址
//
//   標頭檔：#include <vector>
//
// 【詳細解釋 Explanation】
//
// 【1. vector 要解決什麼問題:長度到執行期才知道】
// std::array 很好,但它有一個無法迴避的限制:長度是模板參數,必須在編譯期確定。
// 可是真實世界的資料長度幾乎都是執行期才知道的 —— 讀進來的檔案有幾行?使用者
// 選了幾個項目?網路回傳幾筆記錄?這些都不可能寫死在型別裡。
//
// 在 C 語言,這件事要自己用 malloc/realloc/free 手工處理,而且必須自己記住
// 「目前有幾個」和「配置了多大」兩個數字,還要在每個 error path 上記得釋放。
// 這是 C 程式最主要的記憶體漏洞與 buffer overflow 來源。
//
// std::vector 把這整套流程包起來:自動配置、自動成長、自動釋放(RAII),而且
// **保證元素在記憶體中連續**,所以它同時具備兩個看似衝突的優點 ——
// 像 C 陣列一樣快(可以直接 memcpy、可以餵給只吃 T* 的 C API、對 CPU cache
// 極友善),又像高階語言的動態陣列一樣好用。
//
// 這也是為什麼 C++ 社群的共識是:**沒有特殊理由時,預設就用 vector**。
//
// 【2. size 與 capacity:兩個必須分清楚的數字】
// 這是初學者最常混淆、也是面試最愛問的一組概念:
//   * size()     = 目前**實際存在幾個元素**(已建構、可以合法存取的個數)
//   * capacity() = 目前**這塊記憶體最多能放幾個**元素而不必重新配置
//
// 永遠有 size() <= capacity()。兩者之間那段空間是「已經跟 heap 要來、但還沒
// 放東西」的預留區。vector 之所以要多要一些,正是為了讓後續的 push_back 不必
// 每次都重新配置記憶體。
//
// 本機實測(GCC 15.2.0 / libstdc++),連續 push_back 時 capacity 的變化:
//     size=1 → cap=1,  size=2 → cap=2,  size=3 → cap=4,
//     size=5 → cap=8,  size=9 → cap=16, size=17 → cap=32, size=33 → cap=64
// 也就是容量序列 0 → 1 → 2 → 4 → 8 → 16 → 32 → 64,**成長倍率為 2**。
// 注意這個倍率是**實作定義**的(見【概念補充 B】)。
//
// 【3. 為什麼 push_back 是「攤提 O(1)」而不是 O(1)】
// 單看一次 push_back,它可能有兩種命運:
//   (a) capacity 還有空位 → 就地建構一個元素,O(1),非常快。
//   (b) capacity 滿了     → 必須「重新配置」:跟 heap 要一塊更大的記憶體、
//       把**全部 n 個舊元素搬過去**、解構舊元素、釋放舊記憶體。這一次是 O(n)。
//
// 既然最壞情況是 O(n),為什麼還敢說它是 O(1)?關鍵在「幾何成長」:
//
//   假設倍率為 2,從空的 vector 開始做 n 次 push_back,重新配置只會發生在
//   容量 1、2、4、8、… 這些點上,搬移的元素總數是:
//       1 + 2 + 4 + 8 + … + n/2 + n  <  2n
//   總搬移成本是 O(n),平均分攤到 n 次 push_back,每次只有 O(1)。這就是
//   **攤提分析(amortized analysis)**:個別操作可能很貴,但長期平均是常數。
//
//   反例:如果每次只多要 1 個位置(線性成長),n 次 push_back 的搬移總量是
//       1 + 2 + 3 + … + n = n(n+1)/2 = O(n²)
//   —— 塞 100 萬筆資料會慢到不可接受。幾何成長不是最佳化技巧,而是讓
//   push_back 這個介面能夠成立的**必要條件**。
//
// 【4. 迭代器失效:vector 在真實專案裡的頭號 bug】
// 重新配置會把整塊記憶體換掉,舊位址上的資料整批搬走。因此:
//
//   **一旦發生重新配置,所有指向該 vector 元素的 iterator、pointer、reference
//     全部失效。** 繼續使用它們是 undefined behavior。
//
// 最經典的錯誤長這樣:
//     std::vector<int> v = {1,2,3};
//     int& first = v[0];        // 或 auto it = v.begin();
//     v.push_back(4);           // ← 這裡可能重新配置
//     std::cout << first;       // ← 若真的重新配置了,這是 UB
//
// 這個 bug 之所以特別惡毒,是因為它**不一定當場出事**:如果 capacity 剛好還有
// 空間,就不會重新配置,程式看起來完全正常;等到資料量變大、或換一台機器、
// 換一版編譯器,才突然壞掉。「測試都過了」在這裡完全不是安全的證據。
//
// 同樣的道理:insert() 與 erase() 也會讓**插入/刪除點之後**的迭代器失效
// (因為後面的元素被搬動了),就算沒有重新配置也一樣。所以在迴圈裡邊走邊刪
// 必須用 erase 的回傳值:it = v.erase(it);
//
// 【5. reserve():當你知道大概有多少筆】
// 如果事先知道大約要放幾筆,先呼叫 reserve(n) 可以一次要足記憶體,把 log n 次
// 重新配置壓成 1 次。這在讀檔、解析、批次處理時是很常見且很划算的最佳化。
//
//   注意 reserve 只改變 capacity,**不改變 size**,也不會建構任何元素。
//   reserve(100) 之後 v.size() 仍是 0,寫 v[0] 依然是 undefined behavior。
//   要真的產生元素,那是 resize() 的工作(resize 會建構/解構元素,改變 size)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局:vector 本體其實只有三個指標
//     libstdc++ 的 vector 簡化後是這樣:
//         T* _M_start;           // 資料起點
//         T* _M_finish;          // 最後一個元素的下一個位置 → size = finish - start
//         T* _M_end_of_storage;  // 配置區的結尾       → capacity = end_of_storage - start
//     所以本機實測 sizeof(std::vector<int>) == 24(3 × 8 bytes,**實作定義**),
//     而且**不論裡面裝 0 筆還是 100 萬筆,vector 物件本身永遠是 24 bytes** ——
//     元素全部在 heap 上,vector 物件只是那塊記憶體的「控制頭」。
//     這解釋了為什麼 size() 和 capacity() 都是 O(1):它們只是兩個指標相減。
//
// (B) 成長倍率 2 與 1.5 的真實取捨
//     標準只要求 push_back 是攤提 O(1),**沒有規定倍率**,因此倍率是實作定義:
//         libstdc++(GCC)、libc++(Clang)：2      ← 本機實測為 2
//         MSVC：約 1.5
//     為什麼有人選 1.5?因為倍率 2 有一個記憶體特性:先前釋放的所有區塊加起來
//     永遠不夠放下一次要的大小(1+2+4+…+2^(k-1) = 2^k − 1 < 2^k),所以
//     舊空間**永遠無法被重複利用**,配置位址會一路往前推。倍率小於黃金比例
//     (≈1.618)時,累積釋放的空間終究會超過下一次的需求,配置器就有機會
//     合併並重用這些空間,對長時間執行的程式較友善。
//     代價是倍率越小、重新配置越頻繁(搬移次數變多)。兩者都是合理的工程取捨,
//     這也正是標準刻意不寫死的原因。
//
// (C) 重新配置時到底發生什麼:為什麼 move 建構子該標 noexcept
//     重新配置的完整流程是:配置新空間 → 把舊元素逐一搬到新空間 → 解構舊元素
//     → 釋放舊記憶體。關鍵在中間那步:
//       * 若 T 的 move 建構子標了 noexcept,vector 會用 **move**(快,只搬指標)。
//       * 若沒標 noexcept,vector 只好用 **copy**(慢,深拷貝一份)。
//     原因是 push_back 必須提供 strong exception guarantee(要嘛成功、要嘛
//     vector 維持原狀)。搬到一半若拋例外,move 已經把舊物件掏空、無法還原;
//     copy 則因舊資料還在,可以安全回退。所以自訂型別的 move 建構子沒標
//     noexcept,會讓它在 vector 裡的效能默默掉一個數量級 —— 這是很多人踩過的坑。
//
// (D) 為什麼 vector<bool> 是個著名的「假容器」
//     標準規定 std::vector<bool> 是一個特化版本:它把每個 bool 壓成 **1 個 bit**
//     來省空間。省是省了,但代價很大:
//       * 一個 bit 沒有位址,所以 operator[] 回傳的不是 bool&,而是一個
//         **proxy 物件**(代理參考)。因此 auto x = v[0]; 拿到的是 proxy 不是 bool,
//         而 bool* p = &v[0]; 根本無法編譯。
//       * 它不滿足一般容器的需求,不能安全地餵給某些泛型演算法。
//     這被公認是標準庫的歷史設計失誤。需要一串布林值時,實務上請改用
//     std::vector<char>、std::vector<uint8_t> 或 std::bitset(大小固定時)。
//
// (E) 編譯器實際做了什麼
//     因為資料連續且 operator[] 是簡單的指標偏移,-O2 之下對 vector 的迴圈通常
//     能被完全 inline 並向量化(SIMD),產生的機器碼與手寫 C 指標迴圈幾乎相同。
//     vector 的效能優勢主要來自**連續記憶體帶來的 cache 命中率**,而不是介面本身。
//
// 【注意事項 Pay Attention】
//  1. 重新配置會讓所有 iterator / pointer / reference 失效,之後再使用它們是
//     undefined behavior。行為**不保證、也不可預測** —— 可能看起來正常、可能
//     讀到舊資料、也可能崩潰。絕不能拿「跑起來沒事」當作正確性的證據。
//  2. operator[] 不做邊界檢查,越界是 undefined behavior;index 來自外部輸入時
//     請用 at()(越界會丟 std::out_of_range)。
//  3. reserve(n) 只動 capacity,不動 size。reserve 後仍不可存取 v[0]。
//     要產生實際元素請用 resize()。
//  4. pop_back() 與 clear() **不會**釋放記憶體,capacity 維持不變。要真的把
//     記憶體還回去,呼叫 shrink_to_fit() —— 但標準明定它是**非強制性請求
//     (non-binding request)**,實作可以完全忽略。需要保證釋放的舊寫法是
//     std::vector<int>(v).swap(v)(建一個剛好大小的暫時物件再交換)。
//  5. 成長倍率(本機為 2)、sizeof(vector<int>)==24、
//     max_size()==2305843009213693951 都是**實作定義**的實測值,換平台或換
//     標準庫可能不同,不可寫死在程式邏輯裡。
//  6. std::vector<bool> 不是一般容器(見【概念補充 D】),需要布林陣列請改用
//     vector<char> 或 std::bitset。
//  7. 在迴圈中邊走訪邊 erase,必須用回傳值接續:it = v.erase(it); 否則 it 已
//     失效,再 ++it 是 undefined behavior。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::vector
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. size() 和 capacity() 差在哪?為什麼 push_back 敢說自己是 O(1)?
//     答：size() 是目前實際存在的元素個數,capacity() 是不必重新配置就能容納的
//         上限,恆有 size() <= capacity()。push_back 在容量足夠時是 O(1),容量
//         滿了則要重新配置並搬移全部元素,那一次是 O(n)。但因為容量是**幾何成長**
//         (本機實測倍率 2,屬實作定義),n 次 push_back 的搬移總量是 1+2+4+…+n
//         < 2n = O(n),平均下來每次 O(1) —— 這叫**攤提 O(1)**。
//     追問：如果每次只多配置一個位置會怎樣?
//         → 搬移總量變成 1+2+…+n = O(n²),塞百萬筆資料會慢到不可用。
//           幾何成長是 push_back 這個介面能成立的必要條件,不是最佳化技巧。
//
// 🔥 Q2. 什麼情況下 vector 的 iterator 會失效?
//     答：兩類情況。(a) 發生重新配置(push_back / insert / reserve / resize
//         導致容量不足)→ **所有** iterator、pointer、reference 全部失效。
//         (b) insert / erase 即使沒有重新配置,**插入或刪除點之後**的迭代器也
//         因元素被搬移而失效。所以在迴圈裡刪除必須寫 it = v.erase(it);。
//     追問：那 reserve() 之後再 push_back,迭代器還安全嗎?
//         → 只要總數不超過 reserve 的容量就不會重新配置,先前取得的迭代器仍
//           有效;一旦超過就全部失效。但仰賴這點寫程式很脆弱,不建議。
//
// ⚠️ 陷阱. 呼叫 clear() 之後,vector 佔用的記憶體就還給系統了吧?
//     答：沒有。clear() 只把 size 歸零(解構所有元素),**capacity 完全不變**,
//         那塊 heap 記憶體仍被這個 vector 持有。pop_back() 也一樣。想歸還要呼叫
//         shrink_to_fit(),但標準明定它是**非強制請求**,實作可以忽略;需要
//         保證釋放的作法是 std::vector<int>(v).swap(v)。
//     為什麼會錯：把 clear()「清空」理解成「釋放」。實際上保留 capacity 是刻意
//         設計 —— 清空後常常會再填入資料,留著記憶體可以省下重新配置的成本。
//
// ⚠️ 陷阱. 這段程式碼有什麼問題?
//         std::vector<int> v = {1,2,3};
//         int& first = v[0];
//         v.push_back(4);
//         std::cout << first;
//     答：push_back 可能觸發重新配置,使 first 這個 reference 失效,讀它是
//         undefined behavior。是否真的失效取決於當下 capacity 是否還有空間 ——
//         **不保證會出錯,但也不保證安全**。
//     為什麼會錯：多數人測一次發現「印出來是對的」就認定沒問題。實際上此例中
//         capacity 是否剛好足夠是實作決定的;資料量一變、換個標準庫版本就可能
//         崩潰。這正是 vector 最常見、也最難追的線上事故成因 —— 潛伏期很長。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element
//   題目：給一個陣列 nums 和一個值 val,原地(in-place)移除所有等於 val 的元素,
//         回傳剩餘元素個數;不需在意超出長度之後的內容。
//   為什麼用到本主題：這題正是 vector「刪除中間元素是 O(n)」的直接後果 ——
//     若對每個目標各呼叫一次 erase,總成本是 O(n²)。標準解法是 **erase-remove
//     idiom**:先用 std::remove 把要保留的元素往前搬(一次掃描 O(n)),它回傳
//     新的邏輯結尾,再用 erase 一次砍掉尾巴。這是 vector 最重要的實務慣用法。
//   複雜度：時間 O(n),空間 O(1)。
// -----------------------------------------------------------------------------
int removeElement(std::vector<int>& nums, int val) {
    // std::remove 不會真的縮短容器(它做不到,演算法只看得到迭代器),
    // 它只把「不等於 val」的元素依序往前搬,回傳新的邏輯結尾迭代器。
    auto newEnd = std::remove(nums.begin(), nums.end(), val);
    // 真正把尾巴刪掉、讓 size() 正確,必須由容器自己的 erase 完成。
    nums.erase(newEnd, nums.end());
    return static_cast<int>(nums.size());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】解析 access log,統計並挑出需要告警的 5xx 請求
//   情境：維運時最常做的事之一 —— 把一批 log 行讀進來,解析出狀態碼,
//     過濾出伺服器錯誤(5xx)以便觸發告警。
//   為什麼用到本主題：這是 vector 最典型的使用場景(長度執行期才知道)。
//     這裡同時示範一個很划算的最佳化:**事先 reserve**。已知大約有幾筆時,
//     一次要足記憶體可以把 log n 次重新配置壓成 1 次,同時也避免了過程中
//     所有迭代器被反覆作廢的問題。
// -----------------------------------------------------------------------------
struct LogEntry {
    std::string path;
    int         status;
};

// 從 "GET /api/users 200" 這種格式取出路徑與狀態碼
static LogEntry parseLine(const std::string& line) {
    // 格式:<method> <path> <status>
    size_t sp1 = line.find(' ');
    size_t sp2 = line.find(' ', sp1 + 1);
    LogEntry e;
    e.path   = line.substr(sp1 + 1, sp2 - sp1 - 1);
    e.status = std::stoi(line.substr(sp2 + 1));
    return e;
}

int main() {
    // ── 原始課堂示範:vector 的基本操作 ────────────────────────────────────
    std::cout << "=== std::vector ===" << std::endl;

    std::vector<int> vec;

    // 動態新增
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    std::cout << "大小: " << vec.size() << std::endl;
    std::cout << "容量: " << vec.capacity() << std::endl;

    // 繼續新增
    vec.push_back(40);
    vec.push_back(50);

    std::cout << "新增後大小: " << vec.size() << std::endl;
    std::cout << "新增後容量: " << vec.capacity() << std::endl;

    // 存取
    std::cout << "vec[2]: " << vec[2] << std::endl;
    std::cout << "front: " << vec.front() << std::endl;
    std::cout << "back: " << vec.back() << std::endl;

    // 刪除最後一個
    vec.pop_back();
    std::cout << "pop_back 後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // ── size vs capacity:pop_back / clear 不會釋放記憶體 ──────────────────
    std::cout << "\n=== size vs capacity ===" << std::endl;
    std::cout << "pop_back 後 size=" << vec.size()
              << " capacity=" << vec.capacity() << " (容量沒有跟著變小)" << std::endl;
    vec.clear();
    std::cout << "clear()  後 size=" << vec.size()
              << " capacity=" << vec.capacity() << " (clear 不釋放記憶體)" << std::endl;

    // ── 成長倍率實測:capacity 什麼時候變、變成多少 ────────────────────────
    std::cout << "\n=== 成長軌跡實測 (本機 libstdc++,倍率=2,屬實作定義) ===" << std::endl;
    std::vector<int> g;
    std::cout << "初始 capacity = " << g.capacity() << std::endl;
    size_t lastCap = g.capacity();
    for (int i = 1; i <= 40; ++i) {
        g.push_back(i);
        if (g.capacity() != lastCap) {
            lastCap = g.capacity();
            std::cout << "  size=" << g.size() << " 時重新配置 → capacity=" << lastCap << std::endl;
        }
    }

    // ── 重新配置的可觀察後果(不解參考失效的迭代器,只觀察位址是否改變)────
    std::cout << "\n=== 重新配置會讓迭代器失效 ===" << std::endl;
    std::vector<int> r = {1, 2, 3};
    r.shrink_to_fit();                       // 盡量讓 capacity 貼齊 size(非強制)
    const int* before = r.data();
    size_t capBefore  = r.capacity();
    r.push_back(4);                          // 容量若不足 → 重新配置
    const int* after  = r.data();
    std::cout << "push_back 前 capacity=" << capBefore
              << ",後 capacity=" << r.capacity() << std::endl;
    std::cout << "資料區塊有被搬移(即發生重新配置)? "
              << std::boolalpha << (before != after) << std::endl;
    std::cout << "→ 若為 true,先前取得的 iterator/pointer/reference 全部失效,"
                 "再使用它們是 UB(此處刻意不示範)" << std::endl;

    // ── reserve:一次要足,避免中途重新配置 ────────────────────────────────
    std::cout << "\n=== reserve 的效果 ===" << std::endl;
    std::vector<int> noReserve, withReserve;
    withReserve.reserve(1000);
    int reallocNo = 0, reallocYes = 0;
    size_t cap1 = noReserve.capacity(), cap2 = withReserve.capacity();
    for (int i = 0; i < 1000; ++i) {
        noReserve.push_back(i);
        if (noReserve.capacity() != cap1) { cap1 = noReserve.capacity(); ++reallocNo; }
        withReserve.push_back(i);
        if (withReserve.capacity() != cap2) { cap2 = withReserve.capacity(); ++reallocYes; }
    }
    std::cout << "填入 1000 筆,未 reserve 的重新配置次數: " << reallocNo << std::endl;
    std::cout << "填入 1000 筆,已 reserve 的重新配置次數: " << reallocYes << std::endl;
    std::cout << "注意 reserve 只改 capacity 不改 size" << std::endl;

    // ── LeetCode 27 ───────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 27. Remove Element ===" << std::endl;
    std::vector<int> nums = {0, 1, 2, 2, 3, 0, 4, 2};
    std::cout << "原始: ";
    for (int n : nums) std::cout << n << " ";
    std::cout << "\nval=2,erase-remove 後長度 = " << removeElement(nums, 2) << std::endl;
    std::cout << "結果: ";
    for (int n : nums) std::cout << n << " ";
    std::cout << std::endl;

    // ── 日常實務:解析 access log,挑出 5xx ────────────────────────────────
    std::cout << "\n=== 日常實務: access log 過濾 5xx ===" << std::endl;
    const std::vector<std::string> rawLines = {
        "GET /api/users 200",
        "POST /api/login 500",
        "GET /static/logo.png 304",
        "GET /api/orders 503",
        "DELETE /api/cart 404"
    };

    std::vector<LogEntry> entries;
    entries.reserve(rawLines.size());        // 已知筆數 → 一次要足,零次重新配置
    for (const std::string& line : rawLines) entries.push_back(parseLine(line));

    std::cout << "共解析 " << entries.size() << " 筆,capacity="
              << entries.capacity() << " (reserve 後不再成長)" << std::endl;

    std::cout << "需要告警的 5xx 請求:" << std::endl;
    for (const LogEntry& e : entries) {
        if (e.status >= 500 && e.status < 600) {
            std::cout << "  [ALERT] " << e.status << " " << e.path << std::endl;
        }
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類2.cpp" -o vector_demo

// === 預期輸出 ===
// === std::vector ===
// 大小: 3
// 容量: 4
// 新增後大小: 5
// 新增後容量: 8
// vec[2]: 30
// front: 10
// back: 50
// pop_back 後: 10 20 30 40
//
// === size vs capacity ===
// pop_back 後 size=4 capacity=8 (容量沒有跟著變小)
// clear()  後 size=0 capacity=8 (clear 不釋放記憶體)
//
// === 成長軌跡實測 (本機 libstdc++,倍率=2,屬實作定義) ===
// 初始 capacity = 0
//   size=1 時重新配置 → capacity=1
//   size=2 時重新配置 → capacity=2
//   size=3 時重新配置 → capacity=4
//   size=5 時重新配置 → capacity=8
//   size=9 時重新配置 → capacity=16
//   size=17 時重新配置 → capacity=32
//   size=33 時重新配置 → capacity=64
//
// === 重新配置會讓迭代器失效 ===
// push_back 前 capacity=3,後 capacity=6
// 資料區塊有被搬移(即發生重新配置)? true
// → 若為 true,先前取得的 iterator/pointer/reference 全部失效,再使用它們是 UB(此處刻意不示範)
//
// === reserve 的效果 ===
// 填入 1000 筆,未 reserve 的重新配置次數: 11
// 填入 1000 筆,已 reserve 的重新配置次數: 0
// 注意 reserve 只改 capacity 不改 size
//
// === LeetCode 27. Remove Element ===
// 原始: 0 1 2 2 3 0 4 2
// val=2,erase-remove 後長度 = 5
// 結果: 0 1 3 0 4
//
// === 日常實務: access log 過濾 5xx ===
// 共解析 5 筆,capacity=5 (reserve 後不再成長)
// 需要告警的 5xx 請求:
//   [ALERT] 500 /api/login
//   [ALERT] 503 /api/orders
