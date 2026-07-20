// =============================================================================
//  第六課 3 — std::deque：兩端都能 O(1) 進出的分段式容器
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T, class Allocator = std::allocator<T>> class deque;
//                                                       // <deque>,C++98 起
//   deque = "double-ended queue"(雙端佇列),讀作 "deck"。
//
//   常用介面與複雜度：
//     void      push_back(const T&);          // 攤提 O(1)
//     void      push_front(const T&);         // 攤提 O(1)  ← vector 沒有這個
//     void      pop_back();                   // O(1)
//     void      pop_front();                  // O(1)      ← vector 沒有這個
//     reference operator[](size_type n);      // O(1) 隨機存取(但常數比 vector 大)
//     reference at(size_type n);              // O(1),越界丟 std::out_of_range
//     size_type size()      const noexcept;   // O(1)
//     iterator  insert(const_iterator, const T&);  // 中間插入 O(n)
//     iterator  erase(const_iterator);             // 中間刪除 O(n)
//     // 注意:deque **沒有** capacity()、reserve()、data() —— 見【詳細解釋 2】
//
//   標頭檔：#include <deque>
//
// 【詳細解釋 Explanation】
//
// 【1. deque 要解決什麼問題:vector 的頭部是個死角】
// vector 的資料是一整塊連續記憶體,起點固定。想在**最前面**插入一個元素,就得
// 把現有的 n 個元素全部往後挪一格 —— 那是 O(n)。所以 vector 根本沒有提供
// push_front:不是忘了做,而是做出來也一定慢,標準庫刻意不給你這個誘惑。
//
// 但「兩端都要進出」是非常常見的需求:
//   * 佇列(queue):尾端進、前端出。
//   * 滑動視窗:右邊納入新資料、左邊淘汰舊資料。
//   * undo/redo、瀏覽器上一頁/下一頁、固定長度的最近 N 筆記錄。
//
// deque 就是為此而生:**兩端的插入與刪除都是攤提 O(1)**,同時又保留了 vector
// 最重要的那個能力 —— **O(1) 的隨機存取 d[i]**。這是它和 list 的關鍵分野:
// list 兩端進出也很快,但 list 沒有 operator[]。
//
// 【2. 代價:deque 不是連續記憶體】
// 天下沒有白吃的午餐。要讓「頭部也能 O(1) 成長」,就不可能維持單一連續區塊
// (因為往前長會撞到記憶體區塊的開頭)。deque 的解法是**分段配置**:
//
//   把資料切成許多固定大小的**區塊(chunk / buffer)**,另外用一個指標陣列
//   (稱為 map,和 std::map 無關)記錄每個區塊的位址:
//
//        map(指標陣列)
//        ┌────┬────┬────┬────┐
//        │ p0 │ p1 │ p2 │ p3 │
//        └─┬──┴─┬──┴─┬──┴─┬──┘
//          ▼    ▼    ▼    ▼
//        [chunk][chunk][chunk][chunk]     ← 每塊內部連續,塊與塊之間不連續
//
//   往前長 → 在 map 前端補一個新區塊;往後長 → 在 map 後端補一個新區塊。
//   兩邊都不需要搬動任何既有元素,所以都是 O(1)。
//
// 這帶來三個直接後果:
//   (a) **沒有 data()**:不存在「指向全部元素的單一指標」,所以 deque 無法
//       直接餵給只吃 T* 的 C API。這是 vector 與 deque 最實際的差別之一。
//   (b) **沒有 capacity() / reserve()**:容量不是單一數字,而是「目前有幾個
//       區塊、前後各還剩幾格」,對外沒有意義,所以標準乾脆不提供這個介面。
//   (c) **走訪比 vector 慢**:每次 ++it 都要檢查「是否走到本區塊結尾、需不需要
//       跳到下一個區塊」,而 vector 的 ++it 只是指標加一。加上分段配置對 CPU
//       cache 預取較不友善,所以在**單純掃過所有元素**這件事上,vector 通常
//       明顯較快。deque 的隨機存取 d[i] 也要先算「在第幾塊、塊內第幾格」
//       (一次除法/位移 + 一次額外的指標解參考),常數比 vector 大。
//
// 【3. 最微妙、也最常被考的一點:reference 有效但 iterator 失效】
// 這是 deque 獨有的性質,務必分清楚。在**兩端插入**(push_back / push_front)時:
//
//   * 指向既有元素的 **pointer 與 reference 仍然有效** ——
//     因為既有元素完全沒有被搬動,新元素只是放進新的(或現有的)區塊。
//   * 但**所有 iterator 都失效** ——
//     因為 deque 的 iterator 內部存著「目前區塊」「在 map 中的位置」等資訊,
//     一旦 map 本身被重新配置(要放更多區塊指標時),這些記錄就對不上了。
//
// 對照 vector:vector 重新配置時 **iterator、pointer、reference 一起失效**
// (元素真的被搬走了)。所以「reference 活著但 iterator 死了」是 deque 專屬的
// 情況,面試很愛拿這個分辨你是背答案還是真的理解結構。
//
// 完整規則(標準保證):
//   * 兩端插入 → 全部 iterator 失效;pointer / reference 保持有效。
//   * 中間插入 → iterator、pointer、reference **全部**失效。
//   * 兩端刪除 → 只有被刪那個元素的 iterator / reference 失效,其餘有效。
//   * 中間刪除 → 全部失效。
//
// 【4. 為什麼 stack 和 queue 預設用 deque 而不是 vector】
// std::stack 與 std::queue 是 container adaptor(容器配接器),底層可換。
// 它們的預設底層都是 **deque**:
//     template<class T, class Container = std::deque<T>> class stack;
//     template<class T, class Container = std::deque<T>> class queue;
//
// 理由:
//   * queue 需要 push_back + pop_front。vector 沒有 pop_front(會是 O(n)),
//     所以 queue **根本不能**用 vector 當底層。
//   * stack 只需要尾端進出,vector 其實可以用(std::stack<int, std::vector<int>>
//     是合法的,而且通常更快)。預設仍選 deque,是因為 deque 成長時
//     **不需要搬移既有元素**,不會有 vector 那種「偶爾一次 O(n) 大搬家」的
//     延遲尖峰,對延遲敏感的場合更平順。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 區塊有多大:本機實測
//     libstdc++ 用 _GLIBCXX_DEQUE_BUF_SIZE 決定每個區塊的位元組數,預設 **512**,
//     每塊的元素個數 = max(1, 512 / sizeof(T))。所以本機實測:
//         deque<int>  (sizeof=4)  → 每塊 128 個元素 → 第一次記憶體不連續發生在 index 128
//         deque<char> (sizeof=1)  → 每塊 512 個元素 → 第一次不連續發生在 index 512
//     這些數字全部是**實作定義**(本機 GCC 15.2.0 / libstdc++ 實測),
//     libc++ 與 MSVC 的區塊大小都不同,絕不可寫進程式邏輯裡。
//
// (B) 為什麼 sizeof(std::deque<int>) 這麼大
//     本機實測 sizeof(std::deque<int>) == **80**,而 sizeof(std::vector<int>) 只有 24
//     (兩者皆為**實作定義**)。因為 deque 要記的東西多得多:map 的位址、map 的
//     大小,再加上 start 與 finish 兩個「胖迭代器」—— 每個胖迭代器內部有 4 個
//     指標(目前元素、本區塊起點、本區塊終點、自己在 map 中的位置)。
//     這也解釋了為什麼 deque 的 ++it 比 vector 貴:它要維護的狀態本來就比較多。
//
// (C) 一個常見誤解:deque 不是「頭尾各留空間的 vector」
//     很多人以為 deque 就是「一塊連續記憶體,只是頭部預留了一些空位」。
//     不是。若真是那樣,前面的空位用完時仍要整批搬移,push_front 就不可能是
//     攤提 O(1)。分段配置才是它能兩端 O(1) 的根本原因,也正是它失去
//     連續性與 data() 的原因 —— 這兩件事是同一枚硬幣的兩面。
//
// (D) 記憶體使用的取捨
//     deque 即使只放 1 個元素,也會先配置一整個區塊(本機 512 bytes)加上 map。
//     所以**小容器時 deque 比 vector 浪費**。反過來,資料量很大時 deque 反而
//     有優勢:它不需要一整塊巨大的連續空間,在記憶體破碎的長時間執行程式中
//     比較不會因為「找不到夠大的連續區塊」而配置失敗;而且成長時不搬移既有
//     元素,沒有 vector 那種瞬間需要「舊+新」兩倍記憶體的尖峰。
//
// 【注意事項 Pay Attention】
//  1. deque 的元素**不保證連續**,沒有 data()。需要把資料交給吃 T* 的 C API,
//     必須先複製到 vector 或陣列。
//  2. 兩端插入後,**所有 iterator 失效**,但 pointer / reference 仍有效。
//     中間插入或中間刪除則三者全部失效。使用失效的 iterator 是 undefined
//     behavior,行為**不保證也不可預測**。
//  3. deque 沒有 capacity() 與 reserve() —— 這不是遺漏,是因為分段結構下
//     「容量」不是單一數字。想預先配置只能用 resize()(但那會建構元素)。
//  4. operator[] 不做邊界檢查,越界是 undefined behavior;請對不可信的 index
//     使用 at()。
//  5. 區塊大小(本機 deque<int> 每塊 128 個、deque<char> 每塊 512 個)、
//     sizeof(deque<int>)==80,全部是**實作定義**的實測值,換編譯器就會不同。
//  6. 單純「從頭走到尾」或「大量隨機存取」的工作,vector 通常明顯較快
//     (連續記憶體 + cache 友善)。只有真的需要頭部操作時才選 deque。
//  7. 小資料量時 deque 的固定開銷(整個區塊 + map)比 vector 大,不要為了
//     「感覺比較彈性」就預設用 deque。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::deque
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. deque 為什麼能做到 push_front 是 O(1),而 vector 做不到?
//     答：vector 是單一連續區塊且起點固定,在最前面插入必須把全部 n 個元素
//         往後挪,注定是 O(n),所以標準乾脆不提供 push_front。deque 則是
//         **分段配置**:資料切成固定大小的區塊,另有一個指標陣列(map)記錄
//         各區塊位址。往前長只要在 map 前端掛一個新區塊,既有元素完全不動,
//         所以是攤提 O(1)。代價就是失去記憶體連續性。
//     追問：那 deque 為什麼沒有 data() 和 reserve()?
//         → 因為元素分散在多個區塊,不存在「指向全部元素的單一指標」,
//           data() 無法定義;容量也不是單一數字(而是區塊數 + 前後餘裕),
//           reserve()/capacity() 對外沒有意義,所以標準不提供。
//
// 🔥 Q2. 對 deque 呼叫 push_back 之後,先前取得的 iterator 和 reference 還能用嗎?
//     答：**reference 和 pointer 仍然有效,但所有 iterator 都失效。** 因為既有
//         元素完全沒有被搬動(所以指向它們的位址依然正確),但 deque 的 iterator
//         內部記著「目前區塊、在 map 中的位置」等狀態,map 一旦重新配置就對不上。
//         這和 vector 不同 —— vector 重新配置時三者一起失效。
//     追問：中間插入呢?
//         → 中間插入或中間刪除會讓 iterator、pointer、reference **全部**失效,
//           因為元素真的被搬移了。只有「兩端操作」才有 reference 存活的保證。
//
// ⚠️ 陷阱. deque 有 operator[],所以它跟 vector 一樣是連續記憶體,可以寫
//          memcpy(dst, &d[0], n * sizeof(int)) 吧?
//     答：不行。有 O(1) 隨機存取**不等於**記憶體連續 —— deque 的 d[i] 是先算出
//         「第幾個區塊、塊內第幾格」再解參考,兩次跳躍仍是 O(1),但元素實際上
//         分散在多個區塊中。本機實測 deque<int> 在 index 128 處就出現第一個
//         記憶體不連續點(每塊 512 bytes / 4 = 128 個元素,屬**實作定義**)。
//         跨過區塊邊界做 memcpy 會讀到區塊外的記憶體,是 undefined behavior。
//     為什麼會錯：把「能用 [] 索引」和「連續存放」畫上等號。這兩件事在 vector
//         上剛好同時成立,讓人誤以為它們是同一回事;deque 正是拆開這兩者的
//         反例 —— 它保留了隨機存取,卻放棄了連續性。
//
// ⚠️ 陷阱. 既然 deque 兩端都能 O(1),功能又比 vector 多,那預設用 deque 不是更好?
//     答：不是。deque 的 O(1) 隨機存取常數比 vector 大(要多一層區塊定址),
//         ++it 也要多做區塊邊界檢查,加上分段配置對 CPU cache 預取不友善,
//         所以**單純走訪或隨機存取時 vector 通常明顯較快**;小容器時 deque 的
//         固定開銷(一整個區塊 + map)也比 vector 大。預設仍應該是 vector,
//         只有真的需要頭部進出時才換 deque。
//     為什麼會錯：只比較 Big-O 而忽略常數因子與記憶體區域性。同樣標示 O(1),
//         實際執行時間可以差好幾倍 —— 在現代 CPU 上,cache 命中率往往比
//         漸進複雜度更能決定實際效能。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <vector>
#include <string>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 239. Sliding Window Maximum
//   題目：給陣列 nums 和視窗大小 k,視窗從左滑到右,回傳每個位置的視窗最大值。
//   為什麼用到本主題：這是**單調佇列(monotonic deque)**的經典題,也是 deque
//     最具代表性的應用。演算法需要同時做兩件事:
//       * 從**後端**推入新元素,並把後端所有比它小的元素彈掉(維持遞減);
//       * 從**前端**淘汰已經滑出視窗的舊 index。
//     兩端都要 O(1) 進出 —— vector 做不到(pop_front 是 O(n)),
//     list 雖能兩端進出但缺乏隨機存取也較不便,deque 是唯一自然的選擇。
//   複雜度：時間 O(n)(每個 index 最多進出 deque 各一次),空間 O(k)。
// -----------------------------------------------------------------------------
std::vector<int> maxSlidingWindow(const std::vector<int>& nums, int k) {
    std::vector<int> result;
    std::deque<size_t> dq;                 // 存 index,對應值由大到小(單調遞減)

    for (size_t i = 0; i < nums.size(); ++i) {
        // (1) 前端:把已經滑出視窗的 index 丟掉 —— 需要 pop_front,O(1)
        if (!dq.empty() && dq.front() + static_cast<size_t>(k) <= i) {
            dq.pop_front();
        }
        // (2) 後端:比新元素小的都不可能再當最大值,彈掉 —— 需要 pop_back,O(1)
        while (!dq.empty() && nums[dq.back()] <= nums[i]) {
            dq.pop_back();
        }
        dq.push_back(i);
        // (3) 視窗形成後,前端永遠是當前視窗最大值
        if (i + 1 >= static_cast<size_t>(k)) {
            result.push_back(nums[dq.front()]);
        }
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】滑動視窗速率限制(sliding-window rate limiter)
//   情境：API gateway 要限制「每個使用者在最近 60 秒內最多 N 次請求」。
//     實務上會保留最近這段時間內每次請求的時間戳,新請求進來時:
//       * 從**前端**淘汰掉已經超過 60 秒的舊時間戳;
//       * 若剩下的筆數已達上限就拒絕,否則從**後端**記錄這次的時間戳。
//   為什麼用到本主題：這正是「一端進、另一端出」的教科書型態。
//     用 vector 的話 pop_front 是 O(n)(每次都要整批前移);deque 兩端都是
//     O(1),而且不需要隨機存取以外的任何額外結構。
// -----------------------------------------------------------------------------
class RateLimiter {
public:
    RateLimiter(int maxRequests, long windowSeconds)
        : maxRequests_(maxRequests), windowSeconds_(windowSeconds) {}

    // 回傳 true = 放行,false = 被限流。now 為請求發生的時間戳(秒)。
    bool allow(long now) {
        // 前端淘汰:所有落在視窗之外的舊請求 —— pop_front 是 O(1)
        while (!timestamps_.empty() && timestamps_.front() <= now - windowSeconds_) {
            timestamps_.pop_front();
        }
        if (static_cast<int>(timestamps_.size()) >= maxRequests_) {
            return false;                  // 視窗內已達上限 → 限流
        }
        timestamps_.push_back(now);        // 後端記錄 —— push_back 是 O(1)
        return true;
    }

    size_t inWindow() const { return timestamps_.size(); }

private:
    std::deque<long> timestamps_;
    int              maxRequests_;
    long             windowSeconds_;
};

int main() {
    // ── 原始課堂示範:deque 的基本操作 ────────────────────────────────────
    std::cout << "=== std::deque ===" << std::endl;

    std::deque<int> deq;

    // 可以在兩端操作
    deq.push_back(30);
    deq.push_front(20);
    deq.push_back(40);
    deq.push_front(10);

    std::cout << "元素: ";
    for (int n : deq) std::cout << n << " ";
    std::cout << std::endl;

    // 隨機存取
    std::cout << "deq[2]: " << deq[2] << std::endl;

    // 兩端刪除
    deq.pop_front();
    deq.pop_back();

    std::cout << "刪除頭尾後: ";
    for (int n : deq) std::cout << n << " ";
    std::cout << std::endl;

    // ── deque vs vector 的體積差異(皆為實作定義)──────────────────────────
    std::cout << "\n=== 物件本身的大小 (實作定義) ===" << std::endl;
    std::cout << "sizeof(std::vector<int>) = " << sizeof(std::vector<int>)
              << "  (3 個指標)" << std::endl;
    std::cout << "sizeof(std::deque<int>)  = " << sizeof(std::deque<int>)
              << "  (map 位址 + map 大小 + 兩個各含 4 指標的胖迭代器)" << std::endl;

    // ── 證明 deque 不連續:找出第一個記憶體不連續的位置 ────────────────────
    // 注意:這裡只印「索引」與「布林值」,不印原始位址(位址每次執行都不同)。
    std::cout << "\n=== deque 不是連續記憶體 (區塊大小屬實作定義) ===" << std::endl;
    std::deque<int> big;
    for (int i = 0; i < 400; ++i) big.push_back(i);

    long firstBreak = -1;
    for (size_t i = 0; i + 1 < big.size(); ++i) {
        // 相鄰元素若不是「位址剛好差一個元素」,代表跨過了區塊邊界
        if (&big[i + 1] != &big[i] + 1) { firstBreak = static_cast<long>(i + 1); break; }
    }
    std::cout << "deque<int> 第一個記憶體不連續的索引 = " << firstBreak << std::endl;
    std::cout << "  (本機 libstdc++ 每塊 512 bytes / sizeof(int)=4 → 每塊 128 個元素)" << std::endl;

    std::deque<char> bigc;
    for (int i = 0; i < 1200; ++i) bigc.push_back(static_cast<char>('a' + i % 26));
    long firstBreakC = -1;
    for (size_t i = 0; i + 1 < bigc.size(); ++i) {
        if (&bigc[i + 1] != &bigc[i] + 1) { firstBreakC = static_cast<long>(i + 1); break; }
    }
    std::cout << "deque<char> 第一個記憶體不連續的索引 = " << firstBreakC << std::endl;
    std::cout << "  (512 bytes / sizeof(char)=1 → 每塊 512 個元素)" << std::endl;
    std::cout << "→ 對照 vector:整個資料區永遠連續,不存在不連續點" << std::endl;

    // ── 兩端插入:reference 有效,iterator 失效 ────────────────────────────
    std::cout << "\n=== 兩端插入後 reference 仍有效 (deque 專屬保證) ===" << std::endl;
    std::deque<int> d = {100, 200, 300};
    int& refFirst = d[0];                  // 取得對第一個元素的 reference
    for (int i = 0; i < 500; ++i) {        // 大量 push_back,必然新增區塊
        d.push_back(i);
    }
    // 標準保證:兩端插入不搬移既有元素 → reference 仍指向同一個元素,讀取合法
    std::cout << "push_back 500 次後,原本的 reference 讀到: " << refFirst
              << " (標準保證仍有效)" << std::endl;
    std::cout << "但此時**所有 iterator 都已失效** —— 使用它們是 UB,故不示範"
              << std::endl;
    std::cout << "對照 vector:重新配置後 reference 也會一起失效" << std::endl;

    // ── LeetCode 239 ──────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 239. Sliding Window Maximum ===" << std::endl;
    std::vector<int> nums = {1, 3, -1, -3, 5, 3, 6, 7};
    int k = 3;
    std::cout << "nums = ";
    for (int n : nums) std::cout << n << " ";
    std::cout << " k=" << k << std::endl;
    std::vector<int> win = maxSlidingWindow(nums, k);
    std::cout << "每個視窗的最大值: ";
    for (int n : win) std::cout << n << " ";
    std::cout << std::endl;

    // ── 日常實務:滑動視窗速率限制 ────────────────────────────────────────
    std::cout << "\n=== 日常實務: 滑動視窗速率限制 (10 秒內最多 3 次) ===" << std::endl;
    RateLimiter limiter(3, 10);
    // 時間戳刻意設計:前 4 次擠在同一段時間,第 5 次已超過視窗
    const long requestTimes[] = {100, 101, 102, 103, 115};
    for (long t : requestTimes) {
        bool ok = limiter.allow(t);
        std::cout << "  t=" << t << " → " << (ok ? "放行" : "限流 (429)")
                  << "  [視窗內請求數=" << limiter.inWindow() << "]" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類3.cpp" -o deque_demo

// === 預期輸出 ===
// === std::deque ===
// 元素: 10 20 30 40
// deq[2]: 30
// 刪除頭尾後: 20 30
//
// === 物件本身的大小 (實作定義) ===
// sizeof(std::vector<int>) = 24  (3 個指標)
// sizeof(std::deque<int>)  = 80  (map 位址 + map 大小 + 兩個各含 4 指標的胖迭代器)
//
// === deque 不是連續記憶體 (區塊大小屬實作定義) ===
// deque<int> 第一個記憶體不連續的索引 = 128
//   (本機 libstdc++ 每塊 512 bytes / sizeof(int)=4 → 每塊 128 個元素)
// deque<char> 第一個記憶體不連續的索引 = 512
//   (512 bytes / sizeof(char)=1 → 每塊 512 個元素)
// → 對照 vector:整個資料區永遠連續,不存在不連續點
//
// === 兩端插入後 reference 仍有效 (deque 專屬保證) ===
// push_back 500 次後,原本的 reference 讀到: 100 (標準保證仍有效)
// 但此時**所有 iterator 都已失效** —— 使用它們是 UB,故不示範
// 對照 vector:重新配置後 reference 也會一起失效
//
// === LeetCode 239. Sliding Window Maximum ===
// nums = 1 3 -1 -3 5 3 6 7  k=3
// 每個視窗的最大值: 3 3 5 5 6 7
//
// === 日常實務: 滑動視窗速率限制 (10 秒內最多 3 次) ===
//   t=100 → 放行  [視窗內請求數=1]
//   t=101 → 放行  [視窗內請求數=2]
//   t=102 → 放行  [視窗內請求數=3]
//   t=103 → 限流 (429)  [視窗內請求數=3]
//   t=115 → 放行  [視窗內請求數=1]
