// =============================================================================
//  summary.cpp  —  deque 的內部結構：map 中控器 + 分段連續 buffer
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔: #include <deque>
//   宣告:   template<class T, class Allocator = std::allocator<T>> class deque;
//   標準:   C++98 起;C++11 加入 emplace_front/emplace_back、shrink_to_fit
//
//   核心操作與複雜度:
//     push_front(v) / push_back(v)   攤銷 O(1)
//     pop_front()   / pop_back()     O(1)
//     operator[] / at(i)             O(1)（但比 vector 多一次間接定址）
//     insert(pos,v) / erase(pos)     O(n)（中間位置,需搬移較少的那一側）
//     size() / empty()               O(1)
//
//   ★ deque 沒有 data()。這不是「忘了加」,是因為元素不保證連續(見下)。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要 deque:vector 解不了的那個問題】
// vector 是一整塊連續記憶體 + 「尾端有空位」的設計,所以 push_back 攤銷 O(1)。
// 但 push_front 必須把既有 n 個元素全部往後搬一格 → O(n)。
// 對「兩端都要進出」的場景(滑動視窗、雙端佇列、undo/redo、排程佇列),
// vector 每次前端插入都是 O(n),整體退化成 O(n²)。
//
// 直覺的修法是「在前面也預留空位」,但單一連續區塊的前端一旦用完,
// 還是得整塊重新配置 + 搬移。deque 的答案是:**放棄「整體連續」這個要求**。
//
// 【2. 兩層結構:map(中控器) + 固定大小 buffer】
// deque 把元素切成許多**固定大小的 buffer**(又稱 chunk / node),
// 再用一個**指標陣列**(標準沒給名字,SGI STL 與 libstdc++ 內部稱為 map)
// 記錄每個 buffer 的位址:
//
//     map(指標陣列,本身是連續的)
//     ┌────┬────┬────┬────┬────┬────┐
//     │null│ p1 │ p2 │ p3 │null│null│
//     └────┴─┬──┴─┬──┴─┬──┴────┴────┘
//            │    │    │
//            ▼    ▼    ▼
//        [buffer1][buffer2][buffer3]   ← 每個 buffer 內部才是連續的
//         ← 前端                 後端 →
//
// 關鍵在於 map **從中間開始用**,兩端都留有空的指標槽:
//   * push_back  → 尾端 buffer 還有空位就直接放;滿了就配置新 buffer,
//                  把指標寫進 map 右邊的空槽。
//   * push_front → 對稱地往左邊長。
// 兩者都只動到「一個 buffer」與「map 的一個槽」,**既有元素從不搬移**。
//
// 【3. 為什麼是「攤銷 O(1)」而不是「嚴格 O(1)」】
// 大多數 push 只是在既有 buffer 內寫一個元素 = O(1)。
// 但兩種情況要多做事:
//   (a) 目前 buffer 滿了 → 配置一個新 buffer(固定大小,O(buffer 大小) = O(1) 常數)。
//   (b) map 的指標槽用完了 → 重新配置更大的 map,把**指標**複製過去。
// (b) 看起來像 vector 的擴容,但代價小一個數量級:搬的是 n/buffer_size 個
// **指標**,不是 n 個**元素**;而且元素本身原地不動 → 不會呼叫任何
// 建構/搬移建構子。攤還到每次操作上仍是 O(1)。
//
// 這也是 deque 與 vector 的一個實務差異:vector 擴容會**搬移元素**
// (可能呼叫 move ctor,對 non-noexcept move 型別甚至退化成 copy);
// deque 擴容只搬指標,元素位址穩定。
//
// 【4. operator[] 怎麼做到 O(1):兩次除法換來的隨機存取】
// 邏輯索引 i 要換算成實體位址,概念上是:
//     絕對位置   = (start 在其 buffer 內的偏移) + i
//     buffer 編號 = 絕對位置 / buffer_size
//     buffer 內偏移 = 絕對位置 % buffer_size
//     位址        = map[first_node + buffer 編號] + buffer 內偏移
// 全都是常數次算術 + **兩次記憶體存取**(先讀 map 拿到 buffer 指標,再讀元素)。
// 所以複雜度是 O(1),但常數比 vector 大 —— vector 只要 base + i*sizeof(T)
// 一次定址。這就是「deque 隨機存取比 vector 慢」的真正來源,
// 不是複雜度差異,是常數差異與 cache locality 差異。
//
// 【5. buffer 大小是「實作定義」——標準完全沒規定】
// 標準只規範複雜度與失效規則,**沒有**規定 buffer 多大、甚至沒規定要分段。
// libstdc++ 的策略(本機 g++ 15.2.0 實測,見程式輸出):
//     buffer 元素數 = (sizeof(T) < 512) ? (512 / sizeof(T)) : 1
// 也就是「一個 buffer 約 512 bytes,但至少放得下 1 個元素」。
//   int(4B)    → 128 個/段
//   double(8B) → 64 個/段
//   char(1B)   → 512 個/段
//   600B 的大型 struct → 1 個/段(此時 deque 退化成「每個元素一個配置」)
// MSVC 的策略不同(較小,歷史上是 16 bytes 為基準),所以**同一份程式碼在
// 不同標準庫上,deque 的分段行為與記憶體用量可能差很多**。任何依賴
// 「一段有幾個元素」的程式碼都是不可攜的。
//
// 【6. 為什麼 deque 沒有 data()】
// vector 保證元素連續,所以 `v.data()` 可以直接餵給 C API:
//     read(fd, v.data(), v.size());        // vector: OK
// deque 的元素散在多個 buffer,**根本不存在一個涵蓋全部元素的連續指標**,
// 因此標準不提供 data()。要傳給 C API 只能先複製到連續空間:
//     std::vector<T> flat(dq.begin(), dq.end());   // 複製一份
// 這是選 deque 之前必須確認的事:**只要下游需要連續緩衝區,就別用 deque**。
//
// 【概念補充 Concept Deep Dive】
//
// (A) deque 物件本身有多大
//   libstdc++ 的 deque 內含 map 指標、map 大小,以及 start / finish 兩個
//   迭代器,而每個 deque 迭代器是 4 個指標(cur/first/last/node)。
//   本機實測 sizeof(std::deque<int>) = 80 bytes,而 sizeof(std::vector<int>) = 24。
//   → deque 是個「重」容器,不適合大量小型 deque 物件的場景。
//   (此為 libstdc++ 實測值,非標準保證。)
//
// (B) 空的 deque 也會配置記憶體
//   為了讓 push_front / push_back 都能立刻運作,實作通常在建構時就先配好
//   map 與一個 buffer。所以「空 vector 不配置、空 deque 會配置」是常見差異
//   ——建立大量空容器時值得注意。(同樣是實作定義。)
//
// (C) 失效規則:deque 最違反直覺、也最常考的地方
//   標準對 deque 的規定是:
//     * 在**頭或尾**插入 → **所有 iterator 失效**,
//                          但**所有 reference / pointer 仍然有效**。
//     * 在**中間**插入   → iterator 與 reference / pointer **全部失效**。
//     * 在**頭或尾**刪除 → 只有**被刪元素**的 iterator/reference 失效
//                          (刪尾端時 end() 也失效)。
//     * 在**中間**刪除   → 全部失效。
//
//   「iterator 失效但 reference 有效」為什麼可能?
//   因為 push_front 不搬移任何既有元素 —— 元素在 buffer 中的位址完全沒變,
//   所以指向它的 T* / T& 依然指著同一個物件。失效的是 **iterator**,
//   因為 iterator 內部快取了 node(指向 map 某槽的指標),而 map 可能被
//   重新配置,那個快取就成了懸空指標。
//
//   對比 vector:擴容會把元素整批搬到新位置 → iterator 與 reference 一起失效。
//   這個「deque 的 reference 比 iterator 更耐用」是面試最愛的對照題。
//
// (D) deque 是 std::stack / std::queue 的預設底層容器
//   std::stack<T>  的預設是 std::deque<T>
//   std::queue<T>  的預設是 std::deque<T>
//   選 deque 而非 vector 的理由:queue 需要高效 pop_front(vector 做不到);
//   而 stack 用 deque 則可避免 vector 擴容時的整批搬移。
//   若確定只當 stack 用且在意 cache,可以顯式指定 std::stack<T, std::vector<T>>。
//
// 【注意事項 Pay Attention】
//  1. **沒有 data()**,元素不保證連續 → 不能整段丟給 C API / memcpy。
//  2. 頭尾插入使**所有 iterator 失效**;若在迴圈中一邊 push_front 一邊
//     持有 iterator,行為未定義。持有 **reference/pointer** 才是安全的。
//  3. 中間 insert/erase 使 iterator 與 reference **全部**失效。
//  4. buffer 大小是**實作定義**(libstdc++ = 512/sizeof(T),至少 1),
//     不要寫任何依賴這個數字的程式碼。
//  5. operator[] 不做邊界檢查(UB);要檢查請用 at()(丟 std::out_of_range)。
//  6. 大型元素型別會讓 deque 退化成「每元素一次配置」,記憶體開銷變高。
//  7. 需要連續、需要極致 cache 效率、或只在尾端進出 → 選 vector;
//     兩端都要進出、或不能忍受擴容搬移元素 → 選 deque。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 的內部結構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. deque 內部長什麼樣?為什麼 push_front 和 push_back 都是攤銷 O(1)?
//     答：兩層結構 —— 一個指標陣列(map/中控器)指向多個固定大小的 buffer,
//         每個 buffer 內部連續。map 從中間開始用,兩端都留空槽,
//         所以往前或往後長都只是「在某個 buffer 內寫一格」或「配一個新
//         buffer + 填一個 map 槽」,**既有元素從不搬移**。
//     追問：那 map 本身滿了怎麼辦?→ 重新配置更大的 map,但只複製
//         n/buffer_size 個**指標**,元素原地不動、不呼叫任何建構子,
//         攤還後仍是 O(1);這比 vector 擴容搬移 n 個元素便宜得多。
//
// 🔥 Q2. deque 的 operator[] 是 O(1),為什麼實務上還是比 vector 慢?
//     答：複雜度相同,差在常數與 cache。vector 是 base + i*sizeof(T) 一次定址;
//         deque 要先算 buffer 編號與段內偏移(除法/位移),讀 map 拿到 buffer
//         指標,再讀元素 —— **兩次記憶體存取**。而且元素分段存放,
//         線性掃描的 prefetch 友善度也不如 vector。
//     追問：那遍歷 deque 該怎麼寫比較快?→ 用 range-for / iterator,
//         讓實作在段內用指標連續前進;用 `for(i=0;i<size();++i) dq[i]`
//         等於每次都從頭重算定址。
//
// 🔥 Q3. deque 為什麼沒有 data()?
//     答：因為元素分散在多個 buffer,不存在一個涵蓋全部元素的連續區段,
//         data() 沒有任何合理的回傳值。vector / string / array 有 data()
//         正是因為標準保證它們連續。
//     追問：那要傳給只吃連續緩衝區的 C API 怎麼辦?→ 只能複製出來:
//         `std::vector<T> flat(dq.begin(), dq.end());`。
//         如果這個複製無法接受,代表當初就不該選 deque。
//
// ⚠️ 陷阱. 「容器一改動,iterator 和 reference 就一起失效」——對 deque 錯在哪?
//     答：對 deque 的**頭尾插入**,標準明確規定:**所有 iterator 失效,
//         但 reference 與 pointer 仍然有效**。因為 push_front/push_back
//         不搬移既有元素,元素位址沒變;失效的是 iterator 內部快取的
//         node 指標(map 可能被重配)。
//     為什麼會錯：多數人拿 vector 的模型套用到所有序列容器 —— vector 擴容
//         會整批搬移元素,所以 iterator 與 reference 必然一起死。
//         deque 不搬元素,兩者因此**脫鉤**。
//         注意這個豁免**只限頭尾**:中間 insert/erase 仍然全部失效。
//
// ⚠️ 陷阱2. 「deque 的 buffer 是 512 bytes,一段放 128 個 int」——這句話錯在哪?
//     答：錯在把**實作細節當成標準規定**。標準完全沒有規定 buffer 大小,
//         甚至沒規定必須分段。512/sizeof(T) 是 libstdc++ 的選擇(本機實測),
//         MSVC 用的是另一套較小的規則。
//     為什麼會錯：這個數字在教材與部落格裡被大量複述,久了就被當成標準。
//         正確說法是:「libstdc++ 實測為 512 bytes/段,至少 1 個元素;
//         標準未規定,不可攜。」
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <vector>
#include <string>
#include <algorithm>

// -----------------------------------------------------------------------------
// 工具:印出 deque 內容
// -----------------------------------------------------------------------------
template <typename T>
void print(const std::string& label, const std::deque<T>& dq) {
    std::cout << "  " << label << " [" << dq.size() << "]:";
    for (const auto& v : dq) std::cout << " " << v;   // 前置空白 → 不留行尾空白
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 探針:實測 libstdc++ 的 buffer 大小
//   作法:連續 push_back,觀察相鄰元素的位址何時「不再相差 sizeof(T)」——
//         那就是跨過了 buffer 邊界。回傳「單段最多連續幾個元素」。
//   ★ 這個數值是 libstdc++ 實作細節,標準未規定,其他標準庫會不同。
//   ★ 只印「幾個元素」這種穩定值,不印位址(位址每次執行都不同)。
// -----------------------------------------------------------------------------
template <typename T>
std::size_t probeBufferElems() {
    std::deque<T> dq;
    dq.push_back(T{});
    const char* prev = reinterpret_cast<const char*>(&dq.back());
    std::size_t run = 1, best = 1;
    for (int i = 0; i < 2000; ++i) {
        dq.push_back(T{});
        const char* cur = reinterpret_cast<const char*>(&dq.back());
        if (cur == prev + sizeof(T)) { ++run; best = std::max(best, run); }
        else                          { run = 1; }
        prev = cur;
    }
    return best;
}

struct Big { char pad[600]; };   // 刻意大於 512 bytes,觀察退化情形

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 641. Design Circular Deque
//   題目：設計一個固定容量的循環雙端佇列,支援前後插入/刪除與查詢。
//   為什麼用到本主題：這題的 API(insertFront/insertLast/deleteFront/
//     deleteLast)正是 deque 的語意本體。原題要求自己用陣列做循環緩衝區,
//     這裡用 std::deque 實作,正好凸顯「deque 就是替你把兩端 O(1) 做好的容器」;
//     差別在原題容量固定,所以要自己擋住超量插入。
// -----------------------------------------------------------------------------
class MyCircularDeque {
    std::deque<int> dq_;
    std::size_t cap_;
public:
    explicit MyCircularDeque(int k) : cap_(static_cast<std::size_t>(k)) {}
    bool insertFront(int value) {
        if (isFull()) return false;
        dq_.push_front(value);          // O(1),不搬移既有元素
        return true;
    }
    bool insertLast(int value) {
        if (isFull()) return false;
        dq_.push_back(value);
        return true;
    }
    bool deleteFront() { if (isEmpty()) return false; dq_.pop_front(); return true; }
    bool deleteLast()  { if (isEmpty()) return false; dq_.pop_back();  return true; }
    int  getFront() const { return isEmpty() ? -1 : dq_.front(); }
    int  getRear()  const { return isEmpty() ? -1 : dq_.back();  }
    bool isEmpty()  const { return dq_.empty(); }
    bool isFull()   const { return dq_.size() == cap_; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 239. Sliding Window Maximum
//   題目：給定陣列與視窗大小 k,回傳每個視窗的最大值。
//   為什麼用到本主題：經典的「單調遞減佇列」——需要在**尾端**彈出比新元素
//     小的候選,又要在**頭端**彈出滑出視窗的舊索引。兩端都要 O(1) 進出,
//     正是 deque 的分段連續結構所提供的能力(vector 的 pop_front 是 O(n))。
//   複雜度：每個索引最多進出 deque 各一次 → O(n)。
// -----------------------------------------------------------------------------
std::vector<int> maxSlidingWindow(const std::vector<int>& nums, int k) {
    std::deque<std::size_t> idx;        // 存索引,保持對應值單調遞減
    std::vector<int> out;
    for (std::size_t i = 0; i < nums.size(); ++i) {
        // 頭端:把已滑出視窗的索引丟掉
        if (!idx.empty() && idx.front() + static_cast<std::size_t>(k) <= i)
            idx.pop_front();
        // 尾端:把不可能再成為最大值的候選丟掉
        while (!idx.empty() && nums[idx.back()] <= nums[i])
            idx.pop_back();
        idx.push_back(i);
        if (i + 1 >= static_cast<std::size_t>(k))
            out.push_back(nums[idx.front()]);   // 頭端永遠是視窗最大值
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】固定長度的事件時間窗(rate limiting / 最近 N 秒統計)
//   情境：API gateway 要判斷「同一個 client 在最近 60 秒內是否超過 N 次請求」。
//   為什麼用 deque：新請求從**尾端**進來,過期的請求從**頭端**淘汰,
//     兩端都是 O(1)。用 vector 的話 erase(begin()) 是 O(n),
//     高 QPS 下會變成瓶頸。
// -----------------------------------------------------------------------------
class SlidingWindowLimiter {
    std::deque<long> stamps_;    // 單調遞增的時間戳(秒)
    long   window_;
    std::size_t limit_;
public:
    SlidingWindowLimiter(long windowSec, std::size_t maxReq)
        : window_(windowSec), limit_(maxReq) {}

    // 回傳 true = 允許;false = 超過限制
    bool allow(long nowSec) {
        // 頭端淘汰:所有早於 (now - window) 的紀錄都過期了
        while (!stamps_.empty() && stamps_.front() <= nowSec - window_)
            stamps_.pop_front();
        if (stamps_.size() >= limit_) return false;
        stamps_.push_back(nowSec);   // 尾端加入
        return true;
    }
    std::size_t activeCount() const { return stamps_.size(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】編輯器的 undo/redo 歷史(有上限的雙端歷史堆疊)
//   情境：文字編輯器保留最近 N 個操作可復原;超過上限就從**最舊的一端**丟棄。
//   為什麼用 deque：新操作 push_back、復原 pop_back(堆疊語意),
//     而「歷史滿了丟掉最舊的」需要 pop_front —— 只有 deque 兩者都 O(1)。
// -----------------------------------------------------------------------------
class UndoHistory {
    std::deque<std::string> undo_, redo_;
    std::size_t cap_;
public:
    explicit UndoHistory(std::size_t cap) : cap_(cap) {}
    void record(const std::string& action) {
        undo_.push_back(action);
        if (undo_.size() > cap_) undo_.pop_front();   // 丟掉最舊的
        redo_.clear();                                // 新操作使 redo 失效
    }
    std::string undo() {
        if (undo_.empty()) return "(無可復原)";
        std::string a = undo_.back(); undo_.pop_back();
        redo_.push_back(a);
        return a;
    }
    std::string redo() {
        if (redo_.empty()) return "(無可重做)";
        std::string a = redo_.back(); redo_.pop_back();
        undo_.push_back(a);
        return a;
    }
    std::size_t undoCount() const { return undo_.size(); }
};

int main() {
    // =========================================================================
    std::cout << "=== 1. 雙端插入:deque 的核心能力 ===\n";
    // =========================================================================
    std::deque<int> dq;
    dq.push_back(10);
    dq.push_back(20);
    dq.push_back(30);
    dq.push_front(5);      // vector 做不到 O(1)
    dq.push_front(1);
    print("push_back×3 + push_front×2", dq);

    // =========================================================================
    std::cout << "\n=== 2. 隨機存取與雙端刪除 ===\n";
    // =========================================================================
    std::cout << "  dq[0]=" << dq[0] << " dq[2]=" << dq[2] << " dq[4]=" << dq[4] << "\n";
    dq.pop_front();
    dq.pop_back();
    print("pop_front + pop_back      ", dq);
    std::cout << "  front=" << dq.front() << " back=" << dq.back() << "\n";

    // =========================================================================
    std::cout << "\n=== 3. 實測 libstdc++ 的 buffer 大小(非標準保證) ===\n";
    // =========================================================================
    // 觀察「單段最多能連續放幾個元素」→ 反推每段的位元組數。
    std::cout << "  int    (sizeof=" << sizeof(int)    << "): 單段 "
              << probeBufferElems<int>()    << " 個 → "
              << probeBufferElems<int>()    * sizeof(int)    << " bytes/段\n";
    std::cout << "  double (sizeof=" << sizeof(double) << "): 單段 "
              << probeBufferElems<double>() << " 個 → "
              << probeBufferElems<double>() * sizeof(double) << " bytes/段\n";
    std::cout << "  char   (sizeof=" << sizeof(char)   << "): 單段 "
              << probeBufferElems<char>()   << " 個 → "
              << probeBufferElems<char>()   * sizeof(char)   << " bytes/段\n";
    std::cout << "  Big    (sizeof=" << sizeof(Big)    << "): 單段 "
              << probeBufferElems<Big>()    << " 個 → 超過 512B 時退化為 1 個/段\n";
    std::cout << "  → 規則 = (sizeof(T) < 512) ? 512/sizeof(T) : 1\n";
    std::cout << "  ★ 此為 libstdc++ 實測,標準未規定,MSVC 不同,不可依賴\n";

    // =========================================================================
    std::cout << "\n=== 4. 失效規則:頭尾插入後 reference 仍有效 ===\n";
    // =========================================================================
    std::deque<int> d2 = {100, 200, 300};
    int* p200 = &d2[1];                 // 指向值 200 的元素
    std::cout << "  push_front 前:*p200=" << *p200 << "\n";
    for (int i = 0; i < 1000; ++i) d2.push_front(i);   // 大量前端插入,必然跨 buffer
    std::cout << "  push_front 1000 次後:*p200=" << *p200
              << " → reference 仍指向同一個元素(標準保證)\n";
    std::cout << "  它的邏輯索引已從 1 變成 " << (1 + 1000)
              << ",驗證 d2[1001]=" << d2[1001] << "\n";
    std::cout << "  ★ 但所有 iterator 已失效 —— 不可在此時使用先前取得的 iterator\n";

    // =========================================================================
    std::cout << "\n=== 5. deque 沒有 data():要給 C API 得先攤平 ===\n";
    // =========================================================================
    std::deque<int> d3 = {7, 8, 9};
    std::vector<int> flat(d3.begin(), d3.end());   // 唯一辦法:複製到連續空間
    std::cout << "  攤平成 vector 後 data()[0]=" << flat.data()[0]
              << " size=" << flat.size() << "\n";

    // =========================================================================
    std::cout << "\n=== 6. LeetCode 641. Design Circular Deque ===\n";
    // =========================================================================
    MyCircularDeque cd(3);
    std::cout << "  insertLast(1)  = " << cd.insertLast(1)  << "\n";   // 1
    std::cout << "  insertLast(2)  = " << cd.insertLast(2)  << "\n";   // 1
    std::cout << "  insertFront(3) = " << cd.insertFront(3) << "\n";   // 1
    std::cout << "  insertFront(4) = " << cd.insertFront(4) << "\n";   // 0 (滿)
    std::cout << "  getRear()      = " << cd.getRear()      << "\n";   // 2
    std::cout << "  isFull()       = " << cd.isFull()       << "\n";   // 1
    std::cout << "  deleteLast()   = " << cd.deleteLast()   << "\n";   // 1
    std::cout << "  insertFront(4) = " << cd.insertFront(4) << "\n";   // 1
    std::cout << "  getFront()     = " << cd.getFront()     << "\n";   // 4

    // =========================================================================
    std::cout << "\n=== 7. LeetCode 239. Sliding Window Maximum ===\n";
    // =========================================================================
    std::vector<int> nums = {1, 3, -1, -3, 5, 3, 6, 7};
    std::cout << "  nums=[1,3,-1,-3,5,3,6,7] k=3 →";
    for (int v : maxSlidingWindow(nums, 3)) std::cout << " " << v;
    std::cout << "\n";

    // =========================================================================
    std::cout << "\n=== 8. 實務:滑動視窗限流器(最近 10 秒最多 3 次) ===\n";
    // =========================================================================
    SlidingWindowLimiter limiter(10, 3);
    const long times[] = {100, 101, 102, 103, 111, 112};
    for (long t : times) {
        bool ok = limiter.allow(t);
        std::cout << "  t=" << t << " → " << (ok ? "允許" : "拒絕(超限)")
                  << "  視窗內筆數=" << limiter.activeCount() << "\n";
    }

    // =========================================================================
    std::cout << "\n=== 9. 實務:有上限的 undo/redo 歷史(上限 3) ===\n";
    // =========================================================================
    UndoHistory hist(3);
    hist.record("輸入 Hello");
    hist.record("設為粗體");
    hist.record("插入圖片");
    hist.record("刪除段落");        // 超過上限 → 最舊的「輸入 Hello」被丟棄
    std::cout << "  歷史筆數(上限3) = " << hist.undoCount() << "\n";
    std::cout << "  undo → " << hist.undo() << "\n";
    std::cout << "  undo → " << hist.undo() << "\n";
    std::cout << "  redo → " << hist.redo() << "\n";

    // =========================================================================
    std::cout << "\n=== 重點整理 ===\n";
    // =========================================================================
    std::cout << "  1. map(指標陣列) + 固定大小 buffer 的兩層分段連續結構\n";
    std::cout << "  2. 兩端插入都不搬移既有元素 → 攤銷 O(1)\n";
    std::cout << "  3. operator[] 是 O(1),但需兩次記憶體存取,常數大於 vector\n";
    std::cout << "  4. 頭尾插入:iterator 全失效,但 reference/pointer 仍有效\n";
    std::cout << "  5. 中間 insert/erase:iterator 與 reference 全部失效\n";
    std::cout << "  6. 沒有 data(),元素不保證連續,不能直接餵 C API\n";
    std::cout << "  7. buffer 大小是實作定義(libstdc++ 512B/段),不可依賴\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 3. 實測 libstdc++ 的 buffer 大小(非標準保證) ===

// === 預期輸出 ===
// === 1. 雙端插入:deque 的核心能力 ===
//   push_back×3 + push_front×2 [5]: 1 5 10 20 30
//
// === 2. 隨機存取與雙端刪除 ===
//   dq[0]=1 dq[2]=10 dq[4]=30
//   pop_front + pop_back       [3]: 5 10 20
//   front=5 back=20
//
//   int    (sizeof=4): 單段 128 個 → 512 bytes/段
//   double (sizeof=8): 單段 64 個 → 512 bytes/段
//   char   (sizeof=1): 單段 512 個 → 512 bytes/段
//   Big    (sizeof=600): 單段 1 個 → 超過 512B 時退化為 1 個/段
//   → 規則 = (sizeof(T) < 512) ? 512/sizeof(T) : 1
//   ★ 此為 libstdc++ 實測,標準未規定,MSVC 不同,不可依賴
//
// === 4. 失效規則:頭尾插入後 reference 仍有效 ===
//   push_front 前:*p200=200
//   push_front 1000 次後:*p200=200 → reference 仍指向同一個元素(標準保證)
//   它的邏輯索引已從 1 變成 1001,驗證 d2[1001]=200
//   ★ 但所有 iterator 已失效 —— 不可在此時使用先前取得的 iterator
//
// === 5. deque 沒有 data():要給 C API 得先攤平 ===
//   攤平成 vector 後 data()[0]=7 size=3
//
// === 6. LeetCode 641. Design Circular Deque ===
//   insertLast(1)  = 1
//   insertLast(2)  = 1
//   insertFront(3) = 1
//   insertFront(4) = 0
//   getRear()      = 2
//   isFull()       = 1
//   deleteLast()   = 1
//   insertFront(4) = 1
//   getFront()     = 4
//
// === 7. LeetCode 239. Sliding Window Maximum ===
//   nums=[1,3,-1,-3,5,3,6,7] k=3 → 3 3 5 5 6 7
//
// === 8. 實務:滑動視窗限流器(最近 10 秒最多 3 次) ===
//   t=100 → 允許  視窗內筆數=1
//   t=101 → 允許  視窗內筆數=2
//   t=102 → 允許  視窗內筆數=3
//   t=103 → 拒絕(超限)  視窗內筆數=3
//   t=111 → 允許  視窗內筆數=2
//   t=112 → 允許  視窗內筆數=2
//
// === 9. 實務:有上限的 undo/redo 歷史(上限 3) ===
//   歷史筆數(上限3) = 3
//   undo → 刪除段落
//   undo → 插入圖片
//   redo → 插入圖片
//
// === 重點整理 ===
//   1. map(指標陣列) + 固定大小 buffer 的兩層分段連續結構
//   2. 兩端插入都不搬移既有元素 → 攤銷 O(1)
//   3. operator[] 是 O(1),但需兩次記憶體存取,常數大於 vector
//   4. 頭尾插入:iterator 全失效,但 reference/pointer 仍有效
//   5. 中間 insert/erase:iterator 與 reference 全部失效
//   6. 沒有 data(),元素不保證連續,不能直接餵 C API
//   7. buffer 大小是實作定義(libstdc++ 512B/段),不可依賴
