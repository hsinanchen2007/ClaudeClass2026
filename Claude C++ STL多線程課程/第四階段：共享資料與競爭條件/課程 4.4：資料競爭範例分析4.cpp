// 檔案：lesson_4_4_iterator_invalidation.cpp
// 說明：迭代器失效造成的資料競爭
//
// 【本檔是「刻意示範錯誤」的範例，不要照抄到實際專案】
//
// reader() 與 writer() 同時存取同一個 std::vector，其中一方是寫入，
// 中間沒有任何同步 → 這是【資料競爭 (data race)】，屬於【未定義行為】。
//
// ⚠️ 未定義行為【沒有固定結果】，所以本檔不會宣稱「一定會 crash」：
//    實測（g++ 15.2 / Ubuntu 26.04，連續執行 20 次）約 8 次 segfault、
//    12 次看似正常印完 —— 而「看似正常」並不代表程式是對的。
//    這正是資料競爭最危險的地方：它可能在測試時完全不出現。
//
// ✅ 要穩定地「看見」這個錯誤，用 ThreadSanitizer，不必等它剛好 crash：
//      g++ -std=c++17 -pthread -g -fsanitize=thread -o race4 '課程 4.4：資料競爭範例分析4.cpp'
//      ./race4
//    → 會明確報出 "WARNING: ThreadSanitizer: data race"，
//      並指出 writer() 的 push_back 與 reader() 的解參考互相衝突。
//
// 正確作法：用 std::mutex 保護整段走訪與修改（見第五階段）。

//
// =============================================================================
//  【教科書段落】迭代器失效：為什麼它比「讀到舊值」嚴重得多
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    迭代器失效造成的資料競爭；五大競爭模式之四
//   標準版本：std::vector 為 C++98；std::thread 為 C++11
//   標頭檔：  <vector>、<thread>、<mutex>
//   實作定義：本機 libstdc++ 的 vector 成長倍率為 2×（MSVC 為 1.5×），
//             此為實作定義值，標準只要求「攤銷 O(1) 的 push_back」
//   偵測工具：g++ -fsanitize=thread（data race）/ -fsanitize=address（use-after-free）
//
// 【詳細解釋 Explanation】
//
// 【1. 這個錯誤為什麼是「升級版」的資料競爭】
//   前幾個範例（counter++、check-then-act）的後果通常是「數值不對」。
//   迭代器失效不同 —— 它的後果是【存取已經被釋放的記憶體】：
//     * reader 手上的 it 指向 vector 舊緩衝區裡的某個位置
//     * writer 的 push_back 發現容量不足 → 配置新緩衝區 → 搬移元素 →
//       【釋放舊緩衝區】
//     * reader 下一次 *it 就是 use-after-free
//   從「答案錯」升級成「記憶體毀損」，而且崩潰的地點往往離肇因很遠，
//   讓除錯變得極為困難。
//
// 【2. vector 的哪些操作會使迭代器失效】
//   ┌────────────────────┬──────────────────────────────────────────┐
//   │ 操作                │ 失效範圍                                  │
//   ├────────────────────┼──────────────────────────────────────────┤
//   │ push_back（未擴容） │ end() 失效；其餘 iterator/reference 仍有效  │
//   │ push_back（擴容）   │ 【全部】iterator / reference / 指標失效     │
//   │ insert              │ 插入點之後全部失效；擴容則全部失效           │
//   │ erase               │ 被刪元素之後全部失效                       │
//   │ clear               │ 全部失效                                  │
//   │ reserve（變大）     │ 全部失效                                  │
//   │ operator[] / at     │ 不失效（純讀寫既有元素）                    │
//   └────────────────────┴──────────────────────────────────────────┘
//   → 危險之處在於「未擴容的 push_back 不會失效」：
//     單執行緒測試時容量夠用，一切正常；
//     上線後資料量變大觸發擴容，才開始隨機崩潰。
//
// 【3. 不同容器的失效規則不同（面試常考）】
//   * std::vector      ：擴容時全部失效（元素連續存放，必須搬家）
//   * std::deque       ：兩端插入使 iterator 失效但 reference 仍有效；
//                        中間插入則全部失效
//   * std::list        ：只有被刪除的那個節點失效，其餘完全不受影響
//   * std::map / set   ：插入不使任何既有 iterator 失效；
//                        erase 只讓被刪節點失效
//   * unordered_map/set：rehash 時 iterator 全部失效，但 reference 仍有效
//   → 節點式容器（list / map）的失效規則寬鬆得多，
//     這是它們在需要「持有長期引用」的場景中的優勢。
//     但要注意：【失效規則寬鬆 ≠ 執行緒安全】。
//     即使 list 的 iterator 不失效，一邊走訪一邊修改仍然是 data race。
//
// 【4. 為什麼「範圍 for 迴圈」也躲不掉】
//   for (int v : data) { ... }
//   會被展開成 begin()/end() 的迭代器迴圈，
//   所以完全一樣會踩到失效問題。而且它把迭代器藏起來了，
//   讓人更容易忘記「這裡有迭代器」。
//   → 在多執行緒環境裡看到範圍 for 走訪共享容器，
//     要立刻警覺這是一段需要保護的臨界區段。
//
// 【5. 正確作法：三種選擇】
//   ① 整段走訪都在鎖內（最直接，但走訪期間別人完全無法寫入）
//   ② 先在鎖內複製一份，再在鎖外走訪複本
//      （走訪時間長時較好；代價是複製成本與資料可能過時）
//   ③ 不提供走訪介面，改成 forEach(callback) 在鎖內執行
//      （見課程 5.5-5 的 SafeContainer）
//   絕對不可行的是「只鎖 begin()」——
//   迭代器一旦拿到手，保護就必須持續到走訪結束。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 vector 擴容必須搬家
//   vector 保證元素在記憶體中【連續存放】（這是它可以用 data() 傳給 C API、
//   以及對快取極度友善的原因）。連續存放意味著容量不足時，
//   無法「就地擴充」（後面那塊記憶體可能已被別人占用），
//   只能配置一塊更大的、把元素全部搬過去、再釋放舊的。
//   → 連續性帶來的效能優勢，代價就是擴容時的全面失效。
//
// (B) 成長倍率為什麼是 2× 或 1.5×
//   若每次只擴充固定量，push_back 的攤銷複雜度會退化成 O(n)。
//   按倍數成長才能保證【攤銷 O(1)】。
//   本機 libstdc++ 用 2×，MSVC 用 1.5× ——
//   1.5× 的理論優勢是「釋放的舊區塊有機會被後續的配置重複使用」，
//   2× 則實作簡單、擴容次數少。兩者都是實作定義的選擇，標準沒有規定。
//
// (C) reserve() 能不能解決這個問題
//   不能。reserve() 只是把擴容【提前】並減少次數，
//   一旦超過預留容量仍然會擴容並使迭代器全部失效。
//   而且就算永遠不擴容，「一邊走訪一邊 push_back」
//   仍然是對同一塊記憶體的無同步讀寫 → data race → UB。
//   → reserve 是效能優化，不是正確性方案。
//
// 【注意事項 Pay Attention】
// 1. 本檔是 UB 示範：不可說「一定會 crash」，也不可說「最壞只是讀到舊值」。
// 2. 迭代器失效的後果是 use-after-free，比數值錯誤嚴重得多。
// 3. 「未擴容的 push_back 不失效」最危險：測試時正常，上線資料量變大才爆。
// 4. 範圍 for 迴圈內部也是迭代器，一樣會失效。
// 5. 不同容器的失效規則不同，但【規則寬鬆不等於執行緒安全】。
// 6. reserve() 是效能優化，不能解決失效或 data race。
// 7. 保護必須涵蓋【整段走訪】，只鎖 begin() 等於沒鎖。
// 8. vector 成長倍率 2× 是本機 libstdc++ 的實作定義值，非標準保證。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   迭代器失效是 C++ 容器的記憶體模型議題，LeetCode 沒有對應題目：
//   允許使用的設計題（146/155/705/707/1603）都在單執行緒下操作自己的結構，
//   不會遇到「走訪期間被別人修改」；並行題（1114～1117/1195）也不涉及容器走訪。
//   硬湊只會失焦，故從缺，改以下方兩個真實情境
//   （連線清單的走訪與剔除、事件訂閱者的迭代）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】迭代器失效
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector 的哪些操作會讓迭代器失效?
//     答：push_back / insert 若觸發擴容 → 【全部】iterator、reference、
//         指標都失效（元素連續存放，擴容必須搬到新的緩衝區並釋放舊的）。
//         未擴容的 push_back 只讓 end() 失效；
//         erase 讓被刪元素之後的全部失效；clear 與 reserve(變大) 全部失效。
//         operator[] / at 這類純讀寫既有元素則不失效。
//     追問：其他容器呢?
//         → std::list 只有被刪的那個節點失效；
//           std::map/set 插入完全不失效、erase 只失效被刪節點；
//           unordered_map rehash 時 iterator 失效但 reference 仍有效。
//           節點式容器的規則寬鬆得多 —— 但這與執行緒安全無關。
//
// 🔥 Q2. 為什麼迭代器失效造成的錯誤比「數值算錯」嚴重?
//     答：因為它的後果是 use-after-free。擴容會【釋放舊緩衝區】，
//         而 reader 手上的迭代器仍指向那塊已釋放的記憶體，
//         再解參考就是存取已歸還給配置器的記憶體。
//         這已經不是「答案不對」，是記憶體毀損 ——
//         而且崩潰的地點往往離肇因很遠，極難追查。
//     追問：怎麼可靠地抓到?
//         → ThreadSanitizer 抓 data race，AddressSanitizer 抓 use-after-free。
//           兩者不能同時開啟，要分成兩次編譯執行。
//
// ⚠️ 陷阱. 「我先 reserve() 足夠大的容量，就不會擴容，
//         這樣一邊走訪一邊 push_back 應該就安全了」——錯在哪?
//     答：reserve 確實能避免擴容與迭代器失效，但【完全不能】解決 data race。
//         一條執行緒在讀 vector 的元素、另一條在寫（push_back 至少要寫
//         元素本身與 size），兩者之間沒有任何同步 →
//         依標準定義仍然是 data race → 未定義行為。
//     為什麼會錯：把「迭代器失效」與「資料競爭」當成同一件事。
//         它們是兩個獨立的問題：
//         失效是【容器的記憶體佈局規則】，data race 是【語言的記憶體模型規則】。
//         reserve 只處理前者；後者只能靠同步機制解決。
//         就算容器完全不動，無同步的並行讀寫依然是 UB。
// ═══════════════════════════════════════════════════════════════════════════
//
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include <algorithm>
#include <atomic>

std::vector<int> data = {1, 2, 3, 4, 5};

void reader() {
    for (auto it = data.begin(); it != data.end(); ++it) {
        // 💀 另一執行緒可能同時 push_back，使 vector 重新配置記憶體，
        //    it 立刻變成懸空迭代器；此時解參考就是未定義行為。
        std::cout << *it << " ";
    }
}

void writer() {
    data.push_back(6);  // 💀 可能導致重新配置，使 reader 手上的迭代器全部失效
}


// -----------------------------------------------------------------------------
// 【正確版】整段走訪都在鎖內
// -----------------------------------------------------------------------------
class SafeIntList {
private:
    mutable std::mutex mtx;
    std::vector<int> items;

public:
    void add(int v) {
        std::lock_guard<std::mutex> lock(mtx);
        items.push_back(v);
    }

    // ✓ 整段走訪在鎖內 —— 期間任何人都無法修改容器
    long sum() const {
        std::lock_guard<std::mutex> lock(mtx);
        long s = 0;
        for (int v : items) s += v;      // 範圍 for 內部也是迭代器
        return s;
    }

    // ✓ 另一種做法：先複製再在鎖外處理（走訪很久時較好）
    std::vector<int> snapshot() const {
        std::lock_guard<std::mutex> lock(mtx);
        return items;
    }

    size_t size() const { std::lock_guard<std::mutex> lock(mtx); return items.size(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 1】連線清單：走訪送出訊息的同時剔除斷線者
//   情境：聊天伺服器要對所有線上連線廣播訊息，
//         同時另一條執行緒會在偵測到斷線時把連線從清單移除。
//   天真寫法：廣播端 for (auto& c : connections) c.send(msg);
//         清理端 connections.erase(...)
//         → erase 使廣播端的迭代器失效 → use-after-free。
//   正解：① 廣播與清理用同一把鎖；
//         ② 由於送訊息可能很慢（網路 I/O），不該整段佔著鎖 ——
//            改成「鎖內複製一份連線清單 → 鎖外送訊息」。
//         這同時解決了失效與「鎖內做 I/O」兩個問題。
// -----------------------------------------------------------------------------
class ConnectionRegistry {
private:
    mutable std::mutex mtx;
    std::vector<int> connIds;          // 用 id 代表連線
    long messagesSent = 0;

public:
    void add(int id) {
        std::lock_guard<std::mutex> lock(mtx);
        connIds.push_back(id);
    }

    void remove(int id) {
        std::lock_guard<std::mutex> lock(mtx);
        connIds.erase(std::remove(connIds.begin(), connIds.end(), id), connIds.end());
    }

    // ✓ 鎖內只複製清單，真正的「送訊息」在鎖外做
    long broadcast() {
        std::vector<int> targets;
        {
            std::lock_guard<std::mutex> lock(mtx);
            targets = connIds;             // 複製一份快照
        }
        // ── 鎖外：模擬送訊息（可能是很慢的網路 I/O）──
        long sent = 0;
        for (int id : targets) {
            if (id >= 0) ++sent;
        }
        {
            std::lock_guard<std::mutex> lock(mtx);
            messagesSent += sent;
        }
        return sent;
    }

    size_t size() const { std::lock_guard<std::mutex> lock(mtx); return connIds.size(); }
    long totalSent() const { std::lock_guard<std::mutex> lock(mtx); return messagesSent; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】事件訂閱者：回呼中「取消訂閱」的經典陷阱
//   情境：事件系統走訪所有訂閱者並呼叫它們的回呼。
//         但回呼本身可能呼叫 unsubscribe() 把自己移除 ——
//         這會在走訪途中修改容器，使迭代器失效（單執行緒也會發生！）。
//   正解：走訪前先複製一份訂閱者清單，對複本走訪。
//         這樣回呼中的增刪只影響「下一輪」，本輪的走訪完全安全。
//   注意這個問題【與多執行緒無關】—— 單執行緒的重入就足以觸發，
//   所以即使程式完全單執行緒也必須這樣寫。
// -----------------------------------------------------------------------------
class EventBus {
private:
    mutable std::mutex mtx;
    std::vector<int> subscribers;

public:
    void subscribe(int id) {
        std::lock_guard<std::mutex> lock(mtx);
        subscribers.push_back(id);
    }

    void unsubscribe(int id) {
        std::lock_guard<std::mutex> lock(mtx);
        subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), id),
                          subscribers.end());
    }

    // ✓ 對【複本】走訪：回呼中的增刪不會弄壞本輪迭代
    //   （注意：回呼在鎖外執行，所以它可以安全地呼叫 unsubscribe，
    //     不會發生對同一把 std::mutex 重複上鎖的 UB）
    int publish() {
        std::vector<int> snapshot;
        {
            std::lock_guard<std::mutex> lock(mtx);
            snapshot = subscribers;
        }
        int delivered = 0;
        for (int id : snapshot) {
            ++delivered;
            // 模擬「回呼中取消訂閱自己」——這在原本的寫法中會使迭代器失效
            if (id % 3 == 0) unsubscribe(id);
        }
        return delivered;
    }

    size_t count() const { std::lock_guard<std::mutex> lock(mtx); return subscribers.size(); }
};

int main() {
    std::cout << "=== 錯誤示範：走訪期間另一條執行緒 push_back（UB）===\n";
    std::thread t1(reader);
    std::thread t2(writer);
    
    t1.join();
    t2.join();
    std::cout << "\n（上面那串數字每次執行都可能不同，也可能崩潰 —— 不受標準保證）\n";

    std::cout << "\n=== 正確版：整段走訪都在鎖內 ===\n";
    {
        SafeIntList list;
        std::vector<std::thread> ths;
        std::atomic<int> badReads{0};

        // 4 條寫入者持續 add，2 條讀取者持續走訪加總
        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&list, i] {
                for (int k = 0; k < 5000; ++k) list.add(i * 5000 + k);
            });
        }
        for (int i = 0; i < 2; ++i) {
            ths.emplace_back([&list, &badReads] {
                for (int k = 0; k < 2000; ++k) {
                    long s = list.sum();          // 整段走訪在鎖內 → 絕不會失效
                    if (s < 0) badReads.fetch_add(1);
                }
            });
        }
        for (auto& t : ths) t.join();

        std::cout << "4 寫入者 × 5000 筆，2 讀取者各走訪 2000 次\n";
        std::cout << "最終元素數: " << list.size() << "（必定為 20000）\n";
        std::cout << "走訪異常次數: " << badReads.load() << "（必定為 0）\n";
        std::cout << "→ 走訪期間容器不可能被修改，迭代器不可能失效\n";
    }

    std::cout << "\n=== 日常實務 1：廣播訊息時同時有人斷線 ===\n";
    {
        ConnectionRegistry reg;
        for (int i = 0; i < 500; ++i) reg.add(i);

        std::atomic<bool> stop{false};
        std::thread broadcaster([&reg, &stop] {
            while (!stop.load(std::memory_order_relaxed)) reg.broadcast();
        });
        // 同時不斷有連線斷開
        std::thread cleaner([&reg] {
            for (int i = 0; i < 400; ++i) reg.remove(i);
        });

        cleaner.join();
        stop.store(true);
        broadcaster.join();

        std::cout << "初始 500 條連線，清理執行緒移除其中 400 條\n";
        std::cout << "剩餘連線: " << reg.size() << "（必定為 100）\n";
        std::cout << "廣播過程未崩潰、未失效（廣播端走訪的是鎖內取得的複本）\n";
        std::cout << "→ 送訊息在鎖外做，所以慢速 I/O 不會拖住整個註冊表\n";
    }

    std::cout << "\n=== 日常實務 2：回呼中取消訂閱（單執行緒也會踩到）===\n";
    {
        EventBus bus;
        for (int i = 1; i <= 30; ++i) bus.subscribe(i);

        std::cout << "初始訂閱者: " << bus.count() << "\n";
        int delivered = bus.publish();     // 過程中會有 10 個取消訂閱自己
        std::cout << "本輪送達: " << delivered << "（必定為 30，複本不受影響）\n";
        std::cout << "publish 後剩餘訂閱者: " << bus.count()
                  << "（必定為 20，被移除的 10 個是 3 的倍數）\n";
        std::cout << "→ 若直接走訪原容器，erase 會使迭代器失效 ——\n";
        std::cout << "  這與多執行緒無關，單執行緒的重入就足以觸發。\n";
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.4：資料競爭範例分析4.cpp' -o race4
//
// 偵測（第一段是 UB；兩種 sanitizer 不可同時開啟，要分兩次編譯）:
//   g++ -std=c++17 -g -fsanitize=thread  -pthread '課程 4.4：資料競爭範例分析4.cpp' -o race4_tsan
//   g++ -std=c++17 -g -fsanitize=address -pthread '課程 4.4：資料競爭範例分析4.cpp' -o race4_asan

// ⚠️ 第一段「錯誤示範」是 genuine data race + 可能的 use-after-free → UB，
// 那一行走訪結果【每次執行都可能不同，也可能整個崩潰】，不受標準保證。
// 檔頭已記錄本機連續 20 次的觀察（約 8 次 segfault、12 次看似正常）。
// 下面貼的是本機某一次「沒有崩潰」時的真實實測 —— 沒崩潰不代表程式是對的。
//
// 其餘各段皆為確定值（每次執行完全相同，本機連續三次實測 md5 一致）：
// 走訪全在鎖內、廣播走訪的是複本、事件匯流排走訪的也是複本。

// === 預期輸出 ===
// === 錯誤示範：走訪期間另一條執行緒 push_back（UB）===
// 1 2 3 4 5 
// （上面那串數字每次執行都可能不同，也可能崩潰 —— 不受標準保證）
//
// === 正確版：整段走訪都在鎖內 ===
// 4 寫入者 × 5000 筆，2 讀取者各走訪 2000 次
// 最終元素數: 20000（必定為 20000）
// 走訪異常次數: 0（必定為 0）
// → 走訪期間容器不可能被修改，迭代器不可能失效
//
// === 日常實務 1：廣播訊息時同時有人斷線 ===
// 初始 500 條連線，清理執行緒移除其中 400 條
// 剩餘連線: 100（必定為 100）
// 廣播過程未崩潰、未失效（廣播端走訪的是鎖內取得的複本）
// → 送訊息在鎖外做，所以慢速 I/O 不會拖住整個註冊表
//
// === 日常實務 2：回呼中取消訂閱（單執行緒也會踩到）===
// 初始訂閱者: 30
// 本輪送達: 30（必定為 30，複本不受影響）
// publish 後剩餘訂閱者: 20（必定為 20，被移除的 10 個是 3 的倍數）
// → 若直接走訪原容器，erase 會使迭代器失效 ——
//   這與多執行緒無關，單執行緒的重入就足以觸發。
