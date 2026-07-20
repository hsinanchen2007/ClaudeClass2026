// =============================================================================
//  課程 3.2：執行緒守衛類別設計 1  —  JoiningThread：補上移動語意的完整守衛
// =============================================================================
//
// 【主題資訊 Information】
//   class JoiningThread {
//       std::thread t;
//   public:
//       JoiningThread() noexcept = default;
//       explicit JoiningThread(std::thread thread) noexcept;
//       template<typename Func, typename... Args>              // 完美轉發
//       explicit JoiningThread(Func&& f, Args&&... args);
//       JoiningThread(JoiningThread&&) noexcept;               // 移動建構
//       JoiningThread& operator=(JoiningThread&&) noexcept;    // 移動賦值
//       JoiningThread(const JoiningThread&)            = delete;
//       JoiningThread& operator=(const JoiningThread&) = delete;
//       ~JoiningThread();                                      // joinable → join
//   };
//
//   標準版本：C++11(可變參數樣板、完美轉發、移動語意皆為 C++11)
//   標頭檔  ：<thread>、<utility>(std::move / std::forward)
//   複雜度  ：移動為 O(1);解構時的 join() 阻塞至目標執行緒結束
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要移動語意?前面的守衛缺了什麼】
//   ScopedThread 與 FlexibleThread 都不可搬移(自訂解構函式抑制了隱式
//   移動,複製又被 delete),因此:
//     * 不能 std::vector<ScopedThread>
//     * 不能從工廠函式回傳
//     * 不能當作 map 的 value
//   但「一次管理 N 個 worker」正是多執行緒最常見的需求。JoiningThread
//   補上移動建構與移動賦值,讓守衛終於能被容器裝載。
//
// 【2. 移動賦值裡那個 if (t.joinable()) t.join() 為什麼是本檔的靈魂】
//   這一行修正了 std::thread 一個非常容易致命的行為。請先看標準的規定:
//
//     std::thread::operator=(thread&& other)
//         若 *this 目前是 joinable → 呼叫 std::terminate()
//
//   也就是說:
//       std::thread a(work1);
//       std::thread b(work2);
//       a = std::move(b);          // ← a 還 joinable → std::terminate()
//   一行看起來完全無害的賦值,直接讓程式死亡。標準這樣設計的理由和
//   解構函式一致:靜默地 join(阻塞)或 detach(懸空)都比立刻失敗更糟。
//
//   JoiningThread 的移動賦值選擇了【明確的 join 語意】:
//       if (t.joinable()) t.join();   // 先把自己手上的執行緒收乾淨
//       t = std::move(other.t);       // 再接手對方的
//   代價是這個賦值可能阻塞;好處是它有明確且安全的語意,不會 terminate。
//
//   ⚠️ 請特別注意:C++20 的 std::jthread 走的是【第三條路】——
//   它的移動賦值會先 request_stop() 再 join(),然後才接手。
//   三者的規則各不相同,不可混為一談:
//       std::thread   移動賦值到 joinable 的目標 → std::terminate()
//       JoiningThread 移動賦值到 joinable 的目標 → 先 join(可能阻塞)
//       std::jthread  移動賦值到 joinable 的目標 → request_stop() + join
//   jthread 因為有取消管道,那個 join 才不會無限期卡住,程式得以存活。
//
// 【3. self-move 檢查 if (this != &other) 的必要性】
//   若沒有這個檢查,x = std::move(x) 會變成:
//       t.join();                  // 把自己的執行緒 join 掉
//       t = std::move(t);          // 自我移動,結果是未指定狀態
//   之後解構時的行為就不可預期了。加上檢查後,self-move 變成無操作 ——
//   這是安全但保守的處理。注意標準只要求 move-assign 後物件處於
//   「有效但未指定」的狀態,並不保證自我移動安全,所以這個檢查是必要的。
//
// 【4. 完美轉發建構函式:方便,但有陷阱】
//       template<typename Func, typename... Args>
//       explicit JoiningThread(Func&& f, Args&&... args)
//           : t(std::forward<Func>(f), std::forward<Args>(args)...) {}
//
//   它讓你可以直接寫 JoiningThread jt(worker, 42),而不必先建 std::thread。
//   Func&& 與 Args&&... 在樣板推導的情境下是【轉發參考(forwarding
//   reference)】,不是右值參考 —— 它能同時接住左值與右值,再用
//   std::forward 原樣傳給 std::thread 的建構函式。
//
//   陷阱:這個樣板【太貪心】。當你寫
//       JoiningThread a(...);
//       JoiningThread b(a);        // 想複製(本來就該失敗)
//   編譯器會發現樣板可以推導出 Func = JoiningThread&,比「已被 delete 的
//   複製建構函式」更匹配,於是選中樣板 → 接著在 std::thread 的建構函式
//   那裡才失敗,錯誤訊息又長又難懂。
//   正規解法是用 SFINAE 或 C++20 的 requires 把自己的型別排除掉:
//       template<typename Func, typename... Args,
//                typename = std::enable_if_t<
//                    !std::is_same_v<std::decay_t<Func>, JoiningThread>>>
//   本檔保留課文原貌(未加約束),但在示範 3 實際展示這個現象。
//
// 【5. 為什麼移動操作標 noexcept 很重要?】
//   std::vector 在 reallocation 時會用一個叫 move_if_noexcept 的策略:
//     * 元素的移動建構是 noexcept → 用移動(快)
//     * 否則若可複製 → 用複製(慢,但異常安全)
//     * 否則(不可複製)→ 還是用移動,但失去強異常保證
//   JoiningThread 不可複製,所以就算不標 noexcept 也會被移動;
//   但標上 noexcept 讓 vector 能提供強異常保證,而且 std::thread 的
//   移動建構本身就是 noexcept,標上去完全符合事實。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 explicit JoiningThread(std::thread) 與樣板建構函式不衝突?
//   多載決議規則:非樣板函式在「同樣好」的情況下勝過樣板。
//   傳一個 std::thread 右值進來時,非樣板版本是精確匹配,樣板版本也是,
//   平手 → 非樣板勝出。所以 JoiningThread jt(std::thread(f)) 會走
//   專用的那個建構函式,不會誤入樣板。
//
// (B) 為什麼建構函式要 explicit?
//   避免隱式轉換造成的意外。若不是 explicit:
//       void submit(JoiningThread);
//       submit(std::thread(work));   // 隱式建構,所有權悄悄轉移
//   對「持有執行緒」這種重量級資源,所有權轉移應該在原始碼裡看得見。
//
// (C) 這個類別為什麼還是保留了 detach()?
//   它把 std::thread 的介面原樣轉出去(joinable/get_id/join/detach/get)。
//   這是刻意的取捨:方便,但也代表使用者可以自己 detach 掉,
//   破壞「解構時一定 join」的預期。更嚴格的設計會把 detach 拿掉,
//   或至少讓 get() 回傳 const 參考。設計沒有絕對答案,只有取捨。
//
// (D) 為什麼 std::thread 的移動賦值不像 unique_ptr 那樣直接釋放舊資源?
//   unique_ptr 的移動賦值會 delete 舊指標 —— 因為「釋放記憶體」語意唯一。
//   而「釋放執行緒」有 join(阻塞)與 detach(懸空)兩種語意,標準
//   不願替你選,所以選擇 terminate。這與解構函式的設計哲學完全一致:
//   模稜兩可的情況下,標準要求你明講。
//
// 【注意事項 Pay Attention】
//   1. 移動賦值會先 join 自己手上的執行緒 —— 這可能阻塞任意久,
//      在效能敏感的路徑上要意識到這件事。
//   2. 完美轉發建構函式沒有 SFINAE 約束,傳入 JoiningThread 左值會
//      選中樣板並產生冗長的編譯錯誤(示範 3 展示)。
//   3. 類別暴露了 detach(),使用者可自行破壞「解構必 join」的不變式。
//   4. 移動走的物件處於 moved-from 狀態,joinable() 為 false,
//      再對它呼叫 join() 會丟 std::system_error。
//   5. 原始檔在類別定義之後又寫了一次 #include <iostream>/<vector>,
//      雖然標頭有 include guard 不會出錯,但慣例上所有 #include 應集中
//      於檔首;本檔已整理。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動語意與 std::thread 的賦值規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread a(f1), b(f2); a = std::move(b); 會發生什麼?
//     答：std::terminate()。標準規定 std::thread 的移動賦值若目標本身
//         仍是 joinable,就呼叫 std::terminate()。理由與解構函式相同:
//         自動 join 會無預警阻塞、自動 detach 會產生懸空參考,
//         標準寧可立刻失敗也不要靜默做錯事。
//     追問：那 std::jthread 呢?
//         → 規則不同,而且程式會存活。jthread 的移動賦值會先對自己
//           持有的執行緒 request_stop() 再 join(),然後才接手來源。
//           因為有協作取消管道,那個 join 不會無限期阻塞。
//           這兩個是【不同的規則】,不是「類似的行為」。
//
// 🔥 Q2. JoiningThread 的移動賦值為什麼要寫 if (t.joinable()) t.join()?
//     答：因為賦值目標可能已經持有一個正在跑的執行緒。若不先處理,
//         t = std::move(other.t) 這一步就會撞上 std::thread 的
//         移動賦值規則 → std::terminate()。先 join 是選擇了明確且
//         安全的語意,代價是這個賦值可能阻塞。
//     追問：能不能改成 detach 就不會阻塞了?
//         → 可以,但語意完全不同:那條執行緒會變成沒人管的背景工作,
//           若它還在存取這個物件相關的資料就會懸空。除非確定它
//           完全獨立,否則 join 才是安全的預設。
//
// ⚠️ 陷阱 1. 「JoiningThread b(a); 我明明把複製建構 = delete 了,
//              為什麼編譯錯誤指向 std::thread 的建構函式?」
//     答：完美轉發的樣板建構函式太貪心。對非 const 左值 a,樣板可以
//         推導出 Func = JoiningThread&,這比「需要 const 轉換的已刪除
//         複製建構函式」更匹配,於是選中樣板,錯誤延後到樣板實例化
//         時才在 std::thread 那裡爆開。
//     為什麼會錯：以為 = delete 就能攔住所有複製嘗試。實際上 = delete
//         只是讓那個多載「被選中時報錯」,它並不阻止別的多載被選中。
//         正解是用 enable_if/requires 把自身型別排除在樣板之外。
//
// ⚠️ 陷阱 2. 「移動走之後,原本那個 JoiningThread 還能用嗎?」
//     答：物件仍然有效、可以安全解構,但已處於 moved-from 狀態:
//         joinable() 回傳 false,再呼叫 join() 會丟 std::system_error。
//         它可以被重新賦值(move-assign 進新的值)後繼續使用。
//     為什麼會錯：把 moved-from 想成「無效物件,碰了就是 UB」。
//         標準的保證是「有效但未指定的狀態」—— 可以解構、可以賦值,
//         但不該假設它還持有原本的東西。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 主角:支援移動語意的完整執行緒守衛
// -----------------------------------------------------------------------------
class JoiningThread {
    std::thread t;

public:
    // 預設建構:不持有任何執行緒
    JoiningThread() noexcept = default;

    // 從 std::thread 移動建構(非樣板,多載決議時優先於下面的樣板)
    explicit JoiningThread(std::thread thread) noexcept
        : t(std::move(thread)) {}

    // 直接建構執行緒(完美轉發)
    // ⚠️ 未加 SFINAE 約束,傳入 JoiningThread 左值會誤選這裡(見示範 3)
    template<typename Func, typename... Args>
    explicit JoiningThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}

    // 移動建構:接手對方的執行緒,對方變成 moved-from
    JoiningThread(JoiningThread&& other) noexcept
        : t(std::move(other.t)) {}

    // 移動賦值:【先收乾淨自己的】,再接手對方的
    //   若省略這個 join,t = std::move(other.t) 會撞上 std::thread 的
    //   移動賦值規則(目標 joinable → std::terminate())
    JoiningThread& operator=(JoiningThread&& other) noexcept {
        if (this != &other) {      // self-move 保護
            if (t.joinable()) {
                t.join();          // 先處理當前執行緒
            }
            t = std::move(other.t);
        }
        return *this;
    }

    // 禁止複製
    JoiningThread(const JoiningThread&) = delete;
    JoiningThread& operator=(const JoiningThread&) = delete;

    // 解構:自動 join
    ~JoiningThread() {
        if (t.joinable()) {
            t.join();
        }
    }

    // 工具方法(原樣轉出 std::thread 介面)
    bool joinable() const noexcept { return t.joinable(); }
    std::thread::id get_id() const noexcept { return t.get_id(); }

    void join() {
        t.join();
    }

    void detach() {
        t.detach();
    }

    std::thread& get() noexcept { return t; }
};

// -----------------------------------------------------------------------------
// 示範 1:三種建構方式
// -----------------------------------------------------------------------------
void demoThreeWaysToConstruct() {
    // 方式一:直接建構(走完美轉發樣板)
    {
        JoiningThread jt1([]() { std::cout << "  執行緒 1(直接建構)\n"; });
    }  // 解構 → join

    // 方式二:從既有的 std::thread 移動(走非樣板的專用建構函式)
    {
        std::thread t([]() { std::cout << "  執行緒 2(由 std::thread 移入)\n"; });
        JoiningThread jt2(std::move(t));
    }

    // 方式三:帶參數(可變參數樣板 + 完美轉發)
    {
        JoiningThread jt3([](int x, const std::string& tag) {
            std::cout << "  執行緒 3(帶參數): " << tag << " = " << x << "\n";
        }, 42, std::string("answer"));
    }
}

// -----------------------------------------------------------------------------
// 示範 2:移動賦值 —— 對照 std::thread 會 terminate 的情境
//   同樣的操作,std::thread 會 std::terminate(),JoiningThread 則安全地
//   先 join 再接手。
// -----------------------------------------------------------------------------
void demoMoveAssign() {
    JoiningThread a([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        std::cout << "  [a 原本的執行緒] 我會先被 join 掉\n";
    });
    JoiningThread b([]() {
        std::cout << "  [b 的執行緒] 之後由 a 接手\n";
    });

    std::cout << "  賦值前:a.joinable()=" << std::boolalpha << a.joinable()
              << " b.joinable()=" << b.joinable() << "\n";

    a = std::move(b);  // 若換成 std::thread,這一行就是 std::terminate()

    std::cout << "  賦值後:a.joinable()=" << a.joinable()
              << " b.joinable()=" << b.joinable() << "(b 已 moved-from)\n";
}

// -----------------------------------------------------------------------------
// 示範 3:self-move 是安全的無操作
//
//   真實世界的 self-move 幾乎不會寫成 x = std::move(x)(編譯器會用
//   -Wself-move 警告你),而是像下面這樣【透過參考間接發生】——
//   兩個參數在呼叫端剛好指向同一個物件,編譯器看不出來。
//   這正是 if (this != &other) 存在的理由。
// -----------------------------------------------------------------------------
void reassignWorker(JoiningThread& dst, JoiningThread& src) {
    dst = std::move(src);  // dst 與 src 可能是同一個物件
}

void demoSelfMove() {
    JoiningThread jt([]() { std::cout << "  [self-move 示範] 執行緒仍然存活\n"; });

    reassignWorker(jt, jt);  // 間接的 self-move → 被 this != &other 擋下

    std::cout << "  self-move 後 joinable() = " << std::boolalpha
              << jt.joinable() << "(未被誤 join)\n";
}

// -----------------------------------------------------------------------------
// 示範 4:貪心樣板的實證
//   下面這行若解除註解,編譯會失敗 —— 而且錯誤訊息指向 std::thread,
//   不是「複製建構已被刪除」,正是【注意事項 2】描述的現象。
//
//       JoiningThread a([]{});
//       JoiningThread b(a);      // ❌ 樣板被選中 → std::thread 建構失敗
//
//   正解是為樣板加上約束:
//       template<typename Func, typename... Args,
//                typename = std::enable_if_t<
//                    !std::is_same_v<std::decay_t<Func>, JoiningThread>>>
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔【刻意不提供】
//
//   說明:LeetCode 的多執行緒題(1114 / 1115 / 1116 / 1117 / 1195)由評測
//   框架建立與回收執行緒,作答者只實作成員函式,拿不到 std::thread,
//   因此移動語意、移動賦值、守衛類別設計在那些題目裡都不會出現。
//   本檔主題是【資源管理類別的設計】,與之無交集,故從缺。
//   (下一檔「執行緒守衛類別設計2」示範用容器管理多個執行緒時,
//    會以 LeetCode 1114 的真實解法當作被驅動的工作負載。)
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】限流器(rate limiter)的令牌桶補充執行緒
//
//   情境:API 閘道對每個租戶做限流,採令牌桶(token bucket)演算法 ——
//         背景執行緒定期往桶裡補令牌,請求進來時取令牌,取不到就拒絕。
//
//   為何需要移動語意:租戶是動態的(上線、下線、方案變更),
//         限流器物件會被放進容器、從工廠函式回傳、在重新設定時被整個
//         取代。這些操作全都需要可移動 —— 而「重新設定」正是移動賦值
//         的典型場景:新的補令牌執行緒接手,舊的必須先被安全收掉。
//         若成員直接用 std::thread,那個賦值就是 std::terminate()。
// -----------------------------------------------------------------------------
class RateLimiter {
    std::shared_ptr<std::atomic<int>> tokens;
    JoiningThread                     refiller;  // 可移動 → RateLimiter 也可移動

public:
    RateLimiter() = default;

    // refillCount:補幾次;每次補 1 個令牌
    RateLimiter(int capacity, int refillCount)
        : tokens(std::make_shared<std::atomic<int>>(0))
    {
        auto tk = tokens;  // 複製 shared_ptr,執行緒安全持有,不依賴 this
        refiller = JoiningThread([tk, capacity, refillCount]() {
            for (int i = 0; i < refillCount; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                int cur = tk->load(std::memory_order_relaxed);
                if (cur < capacity) {
                    tk->fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    // 移動操作:因為成員都可移動,直接 = default 即可
    RateLimiter(RateLimiter&&) noexcept = default;
    RateLimiter& operator=(RateLimiter&&) noexcept = default;

    RateLimiter(const RateLimiter&) = delete;
    RateLimiter& operator=(const RateLimiter&) = delete;

    int availableTokens() const {
        return tokens ? tokens->load(std::memory_order_relaxed) : 0;
    }
};

int main() {
    std::cout << "=== 示範 1:三種建構方式 ===\n";
    demoThreeWaysToConstruct();

    std::cout << "\n=== 示範 2:移動賦值(std::thread 在此會 terminate)===\n";
    demoMoveAssign();

    std::cout << "\n=== 示範 3:self-move 安全 ===\n";
    demoSelfMove();

    std::cout << "\n=== 日常實務:限流器的令牌補充執行緒 ===\n";
    {
        RateLimiter limiter(10, 4);          // 補 4 次,每次間隔 5ms
        RateLimiter moved = std::move(limiter);  // 整個限流器被移動(容器/工廠場景)
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        std::cout << "  移動後仍正常運作,可用令牌數 = " << moved.availableTokens() << "\n";
    }  // moved 解構 → refiller 解構 → join
    std::cout << "  限流器已關閉,補令牌執行緒已被 join\n";

    std::cout << "\n=== 全部示範結束 ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.2：執行緒守衛類別設計1.cpp" -o guard1

// 註:示範 2 與示範 3 中「工作執行緒印出訊息」的【相對位置】沒有同步保證,
//     每次執行都可能不同 —— 執行緒何時被排程到是作業系統決定的。
//     ⚠️ 而且這幾段【沒有加輸出鎖】:std::cout 保證不會有 data race,
//     但【不保證整行的原子性】,所以工作執行緒的訊息可能插進主執行緒
//     那一行的中間(例如「賦值前:… b.joinable()=true  [b 的執行緒] …」
//     出現在同一行)。這不是 bug,也不是未定義行為,而是缺少輸出鎖的必然結果。
//     以下是本機某一次的實際執行結果。
//     有確定保證的是:所有執行緒的輸出都會出現在其所屬示範區段結束【之前】,
//     因為 JoiningThread 解構時一定會 join。

// === 預期輸出 ===
// === 示範 1:三種建構方式 ===
//   執行緒 1(直接建構)
//   執行緒 2(由 std::thread 移入)
//   執行緒 3(帶參數): answer = 42
//
// === 示範 2:移動賦值(std::thread 在此會 terminate)===
//   賦值前:a.joinable()=true b.joinable()=true
//   [b 的執行緒] 之後由 a 接手
//   [a 原本的執行緒] 我會先被 join 掉
//   賦值後:a.joinable()=true b.joinable()=false(b 已 moved-from)
//
// === 示範 3:self-move 安全 ===
//   self-move 後 joinable() = true(未被誤 join)
//   [self-move 示範] 執行緒仍然存活
//
// === 日常實務:限流器的令牌補充執行緒 ===
//   移動後仍正常運作,可用令牌數 = 4
//   限流器已關閉,補令牌執行緒已被 join
//
// === 全部示範結束 ===
