// =============================================================================
//  課程 4.4：資料競爭範例分析 1  —  Check-Then-Act：快取「先查再插」的競爭
// =============================================================================
//
// 【主題資訊 Information】
//   主題        : Check-Then-Act（先檢查、後行動）競爭模式
//   示範內容    : 無同步的 std::map 快取，兩個執行緒同時 miss 同一個 key
//   標準版本    : C++11 起（<thread>、<mutex>）
//   標頭檔      : <thread> <map> <string> <mutex> <atomic> <chrono>
//   關鍵樣態    : if (c.find(k) == c.end()) { c[k] = ...; }  ← 兩步之間可被插隊
//   本檔性質    : ⚠️ 刻意的錯誤示範。getValue() 同時犯了兩個不同層次的錯，
//                 這兩個錯必須分開理解（見【詳細解釋】第 1 點）。
//
// 【詳細解釋 Explanation】
//
// 【1. 一段程式碼、兩個不同的錯：data race 與 race condition】
// 這是本課最重要的觀念，兩者常被混用，但它們是不同層次的問題：
//
//   ● data race（資料競爭）：
//     兩個執行緒未同步地存取「同一個記憶體位置」，且至少一方是寫入。
//     → 這是【未定義行為 (UB)】，由 C++ 標準 [intro.races] 定義。
//     → 一旦成立，程式的「整個行為」都不再有任何保證，
//        不是「值算錯」而已，而是編譯器與 CPU 的假設全部失效。
//
//   ● race condition（競爭條件）：
//     執行結果取決於執行緒的相對時序，因而可能違反程式的不變量。
//     → 這是【邏輯錯誤】，不一定是 UB。
//     → 即使全程都正確上鎖、完全沒有 data race，它仍然可能存在。
//
//   本檔的 getValue() 兩者兼具：
//     (a) 沒有任何鎖 → 對 std::map 的並行讀寫是 data race → UB。
//     (b) find() 與 cache[key]=... 分成兩步 → check-then-act → race condition。
//
//   把 (a) 修好【不會】自動修好 (b)。本檔第 2 段示範就是專門證明這件事：
//   每一次存取都上鎖（data race 消失），但兩個執行緒仍然都認為 key 不存在，
//   於是重複計算了兩次 —— 這就是「有鎖但仍然錯」的典型。
//
// 【2. 為什麼 find() 與 insert 之間是個窗口】
//   執行緒 A                          執行緒 B                    cache
//   ────────────────────────────────────────────────────────────────────
//   find(1) → end()（miss）                                       {}
//                                     find(1) → end()（miss）      {}
//   計算 "computed_1"                                             {}
//                                     計算 "computed_1"            {}
//   cache[1] = ...                                                {1:...}
//                                     cache[1] = ...（覆蓋）       {1:...}
//
//   兩個執行緒都通過了「不存在」的檢查。若這個值的計算是昂貴的
//   （查資料庫、打 API、解壓縮），代價就是做了兩次，甚至更多次。
//   若快取值帶有副作用（例如「配發一個唯一 ID」），結果會直接錯誤。
//
// 【3. 容易被忽略：operator[] 本身就是寫入】
//   最後一行 return cache[key]; 看起來只是「讀」，其實不是。
//   std::map::operator[] 的語意是「若 key 不存在就『插入』一個
//   value-initialized 的元素再回傳其參考」。
//   所以即使把插入那行刪掉，這一行仍然是寫入操作，仍然構成 data race。
//   真正的唯讀查詢要用 find() 或 at()（at() 找不到會丟 std::out_of_range）。
//
// 【4. 正確作法：把 check 與 act 放進同一個臨界區段】
//   std::lock_guard<std::mutex> lock(mtx);
//   if (cache.find(key) == cache.end()) cache[key] = compute(key);
//   return cache[key];
//   關鍵不在於「有沒有鎖」，而在於「檢查與行動之間鎖有沒有放掉」。
//   臨界區段必須涵蓋整個不可分割的邏輯，而不是涵蓋每一次個別存取。
//
// 【概念補充 Concept Deep Dive】
//
// (A) std::map 的並行安全承諾到哪裡
//   標準容器的執行緒安全保證只有兩條（[res.on.data.races]）：
//     1. 同時呼叫 const 成員函式是安全的（純讀）。
//     2. 對「不同元素」的並行存取是安全的（vector<bool> 除外）。
//   注意第 2 條講的是「已存在的元素」。insert 會改動紅黑樹的內部節點指標
//   與 size，那是整個容器共用的狀態，不在保護範圍內。
//   換句話說：並行 find() + find() 安全；只要有一個 insert，全部不安全。
//
// (B) 為什麼「重複計算」有時候是可以接受的
//   真實世界的快取常刻意允許少量重複計算，換取不必在鎖裡做慢動作：
//     1. 在鎖外計算 value（慢，但並行）
//     2. 進鎖，用 try_emplace 塞進去（快）
//     3. 若別人先塞好了，try_emplace 回傳 false，丟掉自己算的那份
//   前提是計算必須是「純函式、無副作用、結果等價」。
//   這樣做沒有 data race（每次容器存取都有鎖），
//   race condition 依然存在，但它的後果被設計成「無害」——
//   這是工程上處理 race condition 的第三條路：不是消滅它，而是讓它不傷人。
//   本檔第 3 段就是這個模式的可執行版本。
//
// (C) 在鎖裡做昂貴計算的代價
//   把 compute() 放進臨界區段雖然正確，但所有 cache miss 會被串列化。
//   若 compute() 要 200ms，10 個執行緒同時 miss 就是 2 秒。
//   這是「正確但不可用」，也是實務上會被 code review 擋下來的寫法。
//   進階解法：per-key 的 std::shared_future / std::promise，
//   讓後到的執行緒等待「第一個人的計算結果」，而不是各算各的。
//
// 【注意事項 Pay Attention】
// 1. data race 是 UB，不可以用「跑起來好像對」來證明沒問題。
//    本檔第 1 段在本機連跑 20 次都沒 crash，但它依然是錯的程式。
// 2. 修掉 data race ≠ 修掉 race condition。鎖的「範圍」比「有無」更關鍵。
// 3. return cache[key] 是寫入不是讀取；唯讀查詢請用 find() / at()。
// 4. 不要在臨界區段內做 I/O、網路、sleep 或昂貴計算。
// 5. 想穩定看見第 1 段的錯誤，用 ThreadSanitizer，不要靠運氣：
//      g++ -std=c++17 -pthread -g -fsanitize=thread '課程 4.4：資料競爭範例分析1.cpp' -o race1
//      ./race1
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Check-Then-Act 與快取競爭
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. data race 和 race condition 有什麼不同？請各舉一個例子。
//     答：data race 是「未同步地存取同一記憶體位置且至少一方寫入」，
//         由標準定義為未定義行為，例如本檔無鎖的 cache[key] = ...。
//         race condition 是「結果取決於時序而違反不變量」，屬邏輯錯誤，
//         例如 check-then-act：即使每次存取都上鎖，兩個執行緒仍可能
//         都通過「不存在」的檢查而重複插入。
//     追問：可以有 race condition 但沒有 data race 嗎？
//         → 可以，而且非常常見。本檔第 2 段就是：全程上鎖、零 UB，
//           但仍然重複計算了兩次。反過來說，有 data race 就一定是 UB。
//
// 🔥 Q2. 這段快取程式碼要怎麼修才對？只把 find 那行上鎖夠不夠？
//     答：不夠。必須用同一把鎖涵蓋「find 判斷 + 插入 + 回傳」整段，
//         中途不可釋放。分別為每次存取上鎖只消除了 UB，
//         檢查與行動之間的窗口仍在，重複插入照樣發生。
//     追問：那 compute() 很慢怎麼辦？
//         → 在鎖外算，進鎖用 try_emplace；輸的人丟掉自己算的結果。
//           前提是 compute() 無副作用。或用 per-key 的 shared_future
//           讓後到者等待第一個人的結果。
//
// ⚠️ 陷阱. 「我把最後改成 return cache[key]，只是讀取，應該不用鎖吧？」
//     答：錯。std::map::operator[] 在 key 不存在時會【插入】新元素，
//         它是不折不扣的寫入操作，並行呼叫一樣是 data race。
//     為什麼會錯：多數人腦中把 operator[] 類比成陣列的索引讀取，
//         但 map 的 operator[] 語意是「取得或建立」。
//         想要純讀就得用 find()（回傳 iterator）或 at()（越界丟例外）。
//
// ⚠️ 陷阱 2. 「std::map 是標準容器，應該是執行緒安全的吧？」
//     答：不是。標準只保證「並行唯讀安全」與「並行存取不同元素安全」。
//         insert / erase 會改動樹的結構與 size，屬於全域狀態，
//         只要有一個執行緒在寫，其他任何存取都必須同步。
//     為什麼會錯：把「容器本身有內建鎖」當成預設。
//         C++ 標準庫的設計哲學是「不為你沒用到的東西付代價」，
//         同步成本由使用者自己決定，不內建在容器裡。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <map>
#include <string>
#include <mutex>
#include <atomic>
#include <chrono>
#include <vector>

// -----------------------------------------------------------------------------
// 第 1 段：原始錯誤示範 —— 完全沒有同步
// 同時具有 data race（UB）與 race condition（check-then-act）
// -----------------------------------------------------------------------------
std::map<int, std::string> cache;

// 危險！Check-Then-Act 競爭
std::string getValue(int key) {
    if (cache.find(key) == cache.end()) {  // 檢查
        // ← 另一執行緒可能在此插入相同 key！
        cache[key] = "computed_" + std::to_string(key);  // 行動
    }
    return cache[key];
}

// -----------------------------------------------------------------------------
// 第 2 段：【有鎖，但仍然錯】—— 證明 data race 與 race condition 是兩回事
//
// 每一次容器存取都在鎖的保護下 → data race 已完全消除（TSan 不會報警）。
// 但 check 與 act 分屬「兩個不同的臨界區段」，中間鎖被放掉了，
// 所以 check-then-act 的 race condition 原封不動地留著。
// 這裡刻意插入 sleep 放大窗口，讓這個邏輯錯誤穩定重現。
// -----------------------------------------------------------------------------
std::map<int, std::string> cache_locked;
std::mutex mtx_locked;
std::atomic<int> computeCount_locked{0};

std::string lockedButStillRacy(int key) {
    bool missing;
    {
        std::lock_guard<std::mutex> lock(mtx_locked);
        missing = (cache_locked.find(key) == cache_locked.end());  // 檢查
    }  // ← 鎖在此釋放，競爭窗口打開

    if (missing) {
        // 放大窗口：真實程式裡這是「昂貴的計算」所佔用的時間
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ++computeCount_locked;  // 記錄「真的算了幾次」
        std::lock_guard<std::mutex> lock(mtx_locked);
        cache_locked[key] = "computed_" + std::to_string(key);      // 行動
    }

    std::lock_guard<std::mutex> lock(mtx_locked);
    return cache_locked[key];
}

// -----------------------------------------------------------------------------
// 第 3 段：正確作法 A —— check 與 act 放進同一個臨界區段
// -----------------------------------------------------------------------------
std::map<int, std::string> cache_safe;
std::mutex mtx_safe;
std::atomic<int> computeCount_safe{0};

std::string safeGetValue(int key) {
    std::lock_guard<std::mutex> lock(mtx_safe);   // 一把鎖從頭罩到尾
    auto it = cache_safe.find(key);               // 檢查
    if (it == cache_safe.end()) {
        ++computeCount_safe;
        it = cache_safe.emplace(key, "computed_" + std::to_string(key)).first;  // 行動
    }
    return it->second;                            // 檢查與行動之間鎖從未釋放
}

// -----------------------------------------------------------------------------
// 【日常實務範例】高併發快取：在鎖外計算、進鎖 try_emplace
//
// 情境：把使用者 ID 轉成顯示名稱，轉換要查資料庫（慢）。
//       正確作法 A 會把所有 cache miss 串列化 —— 10 個執行緒同時 miss
//       就得排隊等 10 次資料庫往返。這裡改成：
//         1. 鎖外計算（並行，快）
//         2. 進鎖 try_emplace（極短的臨界區段）
//         3. 輸的人丟掉自己算的結果（try_emplace 回傳 false）
//
//       這個寫法【沒有 data race】，但【刻意保留 race condition】：
//       允許偶爾重複計算，換取不在鎖裡做慢動作。
//       前提：計算必須無副作用且結果等價，否則不能這樣做。
// -----------------------------------------------------------------------------
std::map<int, std::string> userNameCache;
std::mutex userNameMtx;
std::atomic<int> dbLookupCount{0};

// 模擬一次昂貴的資料庫查詢（無副作用、結果等價）
static std::string lookupUserNameFromDB(int userId) {
    ++dbLookupCount;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return "user_" + std::to_string(userId);
}

std::string getUserName(int userId) {
    {   // 快路徑：純讀，但仍需鎖（因為別人可能正在寫）
        std::lock_guard<std::mutex> lock(userNameMtx);
        auto it = userNameCache.find(userId);
        if (it != userNameCache.end()) return it->second;
    }

    // 慢路徑：在鎖【外】計算，多執行緒可並行進行
    std::string name = lookupUserNameFromDB(userId);

    {   // 極短臨界區段：只做一次插入嘗試
        std::lock_guard<std::mutex> lock(userNameMtx);
        auto [it, inserted] = userNameCache.try_emplace(userId, std::move(name));
        // inserted == false 代表別人先塞好了 → 用他的，丟掉自己算的那份
        return it->second;
    }
}

int main() {
    std::cout << "=== 第 1 段：無同步（data race + race condition）===\n";
    {
        std::thread t1([]() { getValue(1); });
        std::thread t2([]() { getValue(1); });

        t1.join();
        t2.join();

        std::cout << "cache.size() = " << cache.size()
                  << "，cache[1] = " << cache[1] << "\n";
        std::cout << "本段是未定義行為，「看起來對」不代表它是對的。\n";
    }

    std::cout << "\n=== 第 2 段：每次存取都上鎖，但 check/act 分離 ===\n";
    {
        std::thread t1([]() { lockedButStillRacy(2); });
        std::thread t2([]() { lockedButStillRacy(2); });
        t1.join();
        t2.join();

        std::cout << "實際計算次數 = " << computeCount_locked.load()
                  << "（理想值 1）\n";
        std::cout << "→ data race 已消除（TSan 不會報警），"
                     "但 race condition 還在：重複計算了。\n";
    }

    std::cout << "\n=== 第 3 段：check 與 act 同屬一個臨界區段（正確）===\n";
    {
        std::thread t1([]() { safeGetValue(3); });
        std::thread t2([]() { safeGetValue(3); });
        t1.join();
        t2.join();

        std::cout << "實際計算次數 = " << computeCount_safe.load()
                  << "（理想值 1）\n";
        std::cout << "cache_safe[3] = " << cache_safe[3] << "\n";
    }

    std::cout << "\n=== 日常實務：鎖外計算 + try_emplace ===\n";
    {
        std::vector<std::thread> threads;
        for (int i = 0; i < 4; ++i) {
            // 4 個執行緒搶同一個 userId=42
            threads.emplace_back([]() { getUserName(42); });
        }
        for (auto& t : threads) t.join();

        std::cout << "userNameCache[42] = " << userNameCache[42] << "\n";
        std::cout << "資料庫查詢次數 = " << dbLookupCount.load()
                  << "（1~4 都可能，取決於時序；重複的那幾份被丟棄）\n";
        std::cout << "→ 沒有 data race；race condition 被設計成無害。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.4：資料競爭範例分析1.cpp' -o race1
// TSan: g++ -std=c++17 -Wall -Wextra -pthread -g -fsanitize=thread '課程 4.4：資料競爭範例分析1.cpp' -o race1_tsan

// === 預期輸出 ===
// 【以下為本機（g++ 15.2.0 / Ubuntu 26.04 / 16 執行緒）某次實際執行結果。
//   第 1 段是未定義行為、第 4 段的查詢次數取決於時序，每次執行都可能不同。】
//
// === 第 1 段：無同步（data race + race condition）===
// cache.size() = 1，cache[1] = computed_1
// 本段是未定義行為，「看起來對」不代表它是對的。
//
// === 第 2 段：每次存取都上鎖，但 check/act 分離 ===
// 實際計算次數 = 2（理想值 1）
// → data race 已消除（TSan 不會報警），但 race condition 還在：重複計算了。
//
// === 第 3 段：check 與 act 同屬一個臨界區段（正確）===
// 實際計算次數 = 1（理想值 1）
// cache_safe[3] = computed_3
//
// === 日常實務：鎖外計算 + try_emplace ===
// userNameCache[42] = user_42
// 資料庫查詢次數 = 4（1~4 都可能，取決於時序；重複的那幾份被丟棄）
// → 沒有 data race；race condition 被設計成無害。
//
// 補充：第 1 段在本機連續執行 20 次均未 crash，但那不是「安全」的證據——
//       它是 data race，屬未定義行為。用 TSan 編譯即可穩定看到
//       "WARNING: ThreadSanitizer: data race"，指向 getValue() 的 cache 存取。
