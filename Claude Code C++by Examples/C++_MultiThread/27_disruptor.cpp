// =============================================================
// 27_disruptor.cpp  --  Disruptor / LMAX 風格的 SPMC ring buffer
// =============================================================
//
// 本課目標:
//   1. 理解 LMAX Disruptor 與 lesson 16 的 SPSC ring 之間的差別:
//        SPSC : 單 producer / 單 consumer,buffer 是「pop 後
//               位置就被釋放」── 適合單向 1:1 streaming。
//        Disruptor: 單 producer / *多* consumer,每個 consumer
//               有自己的 cursor (sequence),producer 等到所有
//               consumer 都消費過某格後才回收那格。slot 從不
//               被「pop」── 它只是被「讀」過。
//   2. 看到 Disruptor 為何能跑得超快 (LMAX 號稱單機數百萬 tps):
//        - 多 consumer 之間沒有競爭,各看各的。
//        - 沒有 mutex / cv,完全 cache-line align 的 atomic。
//        - producer 只跟「最慢的 consumer」同步一次。
//        - slot 預分配,沒有動態 alloc。
//   3. 自己寫一個簡化版,讓兩個 consumer 同時消費同一條串流
//      (例如 stage1 = parsing,stage2 = logging),驗算結果。
//
// 注意:這只是 SPMC + 廣播風格 (每個 consumer 看 *相同*
// 的事件)。Disruptor 還支援「pipeline」(consumer A 必須先看,
// consumer B 才能看) 與「sharding」(多 consumer 各看一段),
// 概念都是「sequence + barrier」的延伸。本課示範最基本款。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 27_disruptor.cpp -o 27_disruptor
//
// 執行方式:
//     ./27_disruptor
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     Disruptor / LMAX SPMC ring ── 廣播風格高吞吐 ring
// 前置課程: lesson 11, 13, 16
// 觀念詞彙:
//   - SPMC                ── 單 producer、多 consumer
//   - sequence            ── 單調遞增的事件序號
//   - cursor              ── 各方目前的進度
//   - barrier             ── 「最慢者必須先到」的約束
//   - broadcast           ── 同一事件每個 consumer 都看到
// 新介紹 API:
//   std::array<T, N> 預配置 slot       不做 new/delete
//   alignas(64) on cursors             避免 false sharing
//   producer 只跟 *最慢* consumer 同步  min_consumer_seq()
// 何時使用:
//   - 需要把同一串事件 *廣播* 給多個 stage / 監聽者
//   - 對 latency / jitter 嚴格 (金融、即時通訊、UI thread 廣播)
//   - 不需要動態長度,固定容量可接受
// 何時不要用:
//   - 多 producer → 要做 MP-Disruptor,複雜許多
//   - pop 語意 (項目取走後消失) → 用 lesson 16 SPSC 或 17 sharded
//   - 動態長度的工作 → 用 thread pool
// 常見錯誤:
//   - producer 沒等最慢 consumer → 覆寫尚未讀的 slot
//   - cursor 沒 padding → false sharing 拖慢
//   - 用 release/acquire 配錯邊 → 讀到尚未發布的 slot
//   - 期待嚴格 FIFO + 多 producer → 不適合,改其他結構
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── LMAX Disruptor 的核心思想
// =============================================================
//
// 1. 起源與動機
//    LMAX 是英國一家高頻交易所,2010 年公開了他們的 Disruptor 設計
//    (Martin Thompson 等人)。當時主流訊息系統用 BlockingQueue (mutex +
//    cv) 做 inter-thread 通訊,測量發現:
//      - cache miss 占用 60% 以上時間 (queue head/tail 共享 cache line)
//      - lock contention 在高負載下崩潰
//    Disruptor 用環形 buffer + cursor 取代 queue,單機 LMAX 撐 6M tx/s,
//    p99 latency 微秒級。後來 Java/C# 都有移植版,C++ 也有實作。
//
// 2. 與傳統 queue 的 5 個不同
//    A. *Pre-allocated* 環形 buffer ── 元素槽位開機就建好,never alloc
//       per message。GC 友善 (Java)、cache 友善 (C++)。
//    B. *Single producer* (基本版) ── 沒有 producer 之間爭用,只有 1 個
//       cursor。多 producer 版本要 CAS,但 LMAX 推崇「單寫者原則」。
//    C. *Multiple consumer broadcast* ── 每個 consumer 有自己的 cursor,
//       讀同一份資料 (不像 queue 每個 message 只給一個 consumer)。
//    D. *No locks*,只用 release/acquire ordering + busy-wait。臨界路
//       徑零 syscall。
//    E. *Cache-line padding*:cursor、head、tail 各自獨佔 cache line,
//       false sharing 完全消滅。
//
// 3. 為什麼「單寫者原則」這麼重要
//    Martin Thompson 反覆強調:
//      若你有 N 個 thread 同時寫同一個變數 → 該變數變熱,cache line
//      在 N 個核之間 ping-pong,效能崩塌。
//    工程作法:
//      - 每個變數設計上 *只有一條 thread* 寫,其他 thread 只讀。
//      - 多 producer 場景 → 把 N 個 producer 各自寫進 N 個 SPSC ring,
//        然後 1 個 thread 合併 (multiplexer 模式)。
//    這個原則影響 Aeron messaging、Erlang OTP、Actor 模式等設計。
//
// 4. SPMC 廣播語意
//    傳統 queue:1 message 進 → 1 consumer 拿走 → 再無人能讀。
//    Disruptor:1 message 進 → N 個 consumer 各自獨立讀,直到「所有人
//    都讀過」producer 才能覆蓋這個 slot。
//    用途:
//      - 訊息匯流排 ── 一筆訂單同時餵給「風控」「結算」「審計」三個
//        consumer,每個獨立處理。
//      - event sourcing 的 fan-out。
//
// 5. cursor 機制
//    producer cursor          ── 指向「下一個要寫的 slot」 (publish 後 +1)
//    consumer cursor (×N)      ── 各自指向「下一個要讀的 slot」
//    寫前檢查:producer 的 next_slot = cursor + 1 是否已被「最慢的 consumer」
//    讀完?最慢 consumer cursor + ring_size > producer cursor → 安全。
//    否則 spin 等。這是 producer 的 backpressure。
//
// 6. memory order 配對
//    producer:資料寫入 slot[i] 後,cursor.store(i, release)
//    consumer:cursor.load(acquire),然後讀 slot[i]
//    publish-subscribe 套路 (與 lesson 16 SPSC 一樣)。
//    所有 cursor 比對 (producer 等 slowest consumer) 用 relaxed 即可,
//    因為「多大」這件事本身不需要與資料同步,只是流量控制。
//
// 7. 何時不適合 Disruptor
//    - 需要嚴格 FIFO 的多 producer (Disruptor 多 producer 版本不保證
//      published 順序與 commit 順序一致)
//    - 訊息很大 (slot 預配多 KB):記憶體浪費
//    - 動態擴展容量 (環形 buffer 開機固定)
//    - 跨 process / 跨機器 (這是純 in-process)
//
// 8. 與 SPSC ring (lesson 16) 的關係
//    Disruptor 是 SPSC ring 的廣播版。把 lesson 16 的「consumer tail」
//    從 1 個改成 N 個,producer 等的不再是「tail >= head - size」,而是
//    「min(tail_1, tail_2, ..., tail_N) >= head - size」── 概念簡單,
//    工程細節是性能差距所在。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── Disruptor 是工業設計,
//     LC 不考。
//   → 但概念延伸:lesson 30 Q5 BoundedBlockingQueue 是 SPSC ring 的
//     mutex 退化版;Disruptor 是它的「廣播 + 多 consumer」擴充版。若
//     要把 Q5 改造成「一條訊息可被多個 consumer 各看一次」(訊息匯流排),
//     Disruptor 就是模型。
//
// 主要 API 對照 (cppreference):
//   - std::atomic<T>                    https://en.cppreference.com/w/cpp/atomic/atomic
//   - std::memory_order                 https://en.cppreference.com/w/cpp/atomic/memory_order
//   - alignas / hardware_destructive_interference_size
//                                       https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size
//   - 第三方參考:LMAX Disruptor (Java)、folly::ProducerConsumerQueue
//
// 練習建議:
//   - 把 lesson 30 Q5 BoundedBlockingQueue 改造成「一個 producer 廣播
//     給 N 個 consumer」── 這就是 Disruptor 的最小變形。
// =============================================================

/*
補充筆記：disruptor
  - disruptor 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - disruptor 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/
#include <atomic>
#include <array>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cstddef>

constexpr std::size_t CACHE_LINE = 64;

// -------------------------------------------------------------
// 一個簡化版的 Disruptor:
//   - 容量是 2 的次方,index 用 mask 取代 modulo。
//   - producer cursor (next_): producer 下一個要寫的 sequence。
//   - 每個 consumer 有自己的 cursor (Consumer::seen_)。
//   - producer 寫之前,確認那一格不會把任何 consumer 還沒
//     看過的 slot 蓋掉 (= producer 寫的 sequence 不會超出
//     「最慢的 consumer 看到的 sequence + Capacity」)。
//
// 為什麼這比「pop-style queue」快?
//   - 多 consumer 各自獨立讀同一個 slot,完全沒互相 wait。
//   - producer 不必跟個別 consumer 同步,只跟「最慢的那個」
//     同步一次。
//   - Slot 是預先 in-place 配置的 array,沒有 heap alloc。
// -------------------------------------------------------------
template <typename T, std::size_t Capacity>
class Disruptor {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    static constexpr std::size_t MASK = Capacity - 1;

public:
    // 所有 consumer 都要在 producer 開跑前註冊。
    // 為了簡化,我們用一個固定大小的 cursor 陣列。
    static constexpr int MAX_CONSUMERS = 8;

    Disruptor()
    {
        for (auto& c : consumers_) c.seen_.store(-1, std::memory_order_relaxed);
    }

    // 註冊一個 consumer,回傳 consumer id (用來查它的 cursor)。
    int register_consumer()
    {
        int id = next_consumer_id_++;
        if (id >= MAX_CONSUMERS) std::abort();
        return id;
    }

    // 把所有 consumer cursor 中最小的找出來,告訴 producer
    // 「最遠可以寫到哪裡」。
    long long min_consumer_seq() const
    {
        long long mn = next_.load(std::memory_order_acquire);
        for (int i = 0; i < next_consumer_id_.load(std::memory_order_acquire); ++i) {
            long long s = consumers_[i].seen_.load(std::memory_order_acquire);
            if (s < mn) mn = s;
        }
        return mn;
    }

    // Producer:把 v 放到下一個 slot,若滿了就 yield 等。
    void publish(const T& v)
    {
        long long seq = next_.load(std::memory_order_relaxed);
        // 確保不會把 slot[seq & MASK] 蓋過去而那個還有 consumer
        // 沒看到。「seq - min_consumer_seq」不能 ≥ Capacity。
        while (seq - min_consumer_seq() >= static_cast<long long>(Capacity)) {
            std::this_thread::yield();
        }
        buf_[seq & MASK] = v;
        // 發布:用 release-store 把 cursor 推進。
        // 這條 release 對應 consumer 那邊讀 cursor 的 acquire,
        // 確保 consumer 看到我們上面對 buf_[] 的寫。
        cursor_.store(seq + 1, std::memory_order_release);
        next_.store(seq + 1, std::memory_order_relaxed);
    }

    // Consumer:取得 producer cursor (seq 已可讀的下一個);用
    // acquire 接 producer 的 release。
    long long producer_cursor() const
    {
        return cursor_.load(std::memory_order_acquire);
    }

    // 真正讀 slot 的內容 (假設 sequence 已 < producer_cursor)。
    const T& at(long long seq) const
    {
        return buf_[seq & MASK];
    }

    // Consumer 看完一格後,把自己的 cursor 推進。
    void mark_seen(int consumer_id, long long seq)
    {
        consumers_[consumer_id].seen_.store(seq, std::memory_order_release);
    }

private:
    // ★ 重點:cursor 與 next_ 各自獨佔 cache line (lesson 13)。
    alignas(CACHE_LINE) std::atomic<long long> cursor_{0};   // producer 已發布的 seq+1
    alignas(CACHE_LINE) std::atomic<long long> next_{0};     // producer 下一個要寫的 seq

    struct alignas(CACHE_LINE) Cursor {
        std::atomic<long long> seen_;        // 該 consumer 已處理到的 seq
    };
    alignas(CACHE_LINE) std::array<Cursor, MAX_CONSUMERS> consumers_{};
    alignas(CACHE_LINE) std::atomic<int>                  next_consumer_id_{0};

    alignas(CACHE_LINE) std::array<T, Capacity> buf_{};
};


// =============================================================
// DEMO:1 個 producer 推送 N 個事件,2 個 consumer 各自消費。
//   consumer A 把所有事件加總
//   consumer B 把所有事件做 xor
// =============================================================
int main()
{
    constexpr std::size_t CAP = 1024;
    constexpr long long   N   = 5'000'000;

    Disruptor<int, CAP> d;
    int cidA = d.register_consumer();
    int cidB = d.register_consumer();

    long long sumA  = 0;
    long long xorB  = 0;

    auto t0 = std::chrono::steady_clock::now();

    std::thread consumerA([&]{
        long long seq = 0;
        while (seq < N) {
            long long avail = d.producer_cursor();
            while (seq < avail) {
                sumA += d.at(seq);
                ++seq;
            }
            d.mark_seen(cidA, seq - 1);
            if (seq >= N) break;
            std::this_thread::yield();
        }
    });

    std::thread consumerB([&]{
        long long seq = 0;
        while (seq < N) {
            long long avail = d.producer_cursor();
            while (seq < avail) {
                xorB ^= d.at(seq);
                ++seq;
            }
            d.mark_seen(cidB, seq - 1);
            if (seq >= N) break;
            std::this_thread::yield();
        }
    });

    std::thread producer([&]{
        for (long long i = 0; i < N; ++i) {
            d.publish(static_cast<int>(i & 0x7fffffff));
        }
    });

    producer.join();
    consumerA.join();
    consumerB.join();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0).count();

    // 驗算
    long long expectedSum = 0, expectedXor = 0;
    for (long long i = 0; i < N; ++i) {
        int v = static_cast<int>(i & 0x7fffffff);
        expectedSum += v;
        expectedXor ^= v;
    }

    std::cout << "[Disruptor] " << N << " events broadcast in " << ms << " ms\n";
    std::cout << "[Disruptor] throughput ~ "
              << (N * 1000.0 / (ms ? ms : 1) / 1'000'000) << " M events/s\n";
    std::cout << "[Disruptor] sumA = " << sumA
              << " (expected " << expectedSum << ") "
              << (sumA == expectedSum ? "OK" : "FAIL") << '\n';
    std::cout << "[Disruptor] xorB = " << xorB
              << " (expected " << expectedXor << ") "
              << (xorB == expectedXor ? "OK" : "FAIL") << '\n';

    // ---------------------------------------------------------
    // 實戰範例: 兩階段 pipeline (parser → executor)
    // ---------------------------------------------------------
    // 應用場景: 交易訊息要先 parse, 再 execute, 兩個 stage 各跑
    // 一條 thread, 但要保證 executor 看到的事件「parser 已處理
    // 完」── 這就是 Disruptor 的 sequence barrier 模式。
    //
    // 實作骨架: executor 在處理 sequence S 之前, 必須等
    // parser_cursor >= S。我們重用 Disruptor 的 cursor 機制,
    // executor 把 parser cursor 當「上游 barrier」, 而不是看
    // producer cursor。這裡用簡單的 small workload 示意。
    // ---------------------------------------------------------
    {
        std::cout << "\n[demo] 2-stage pipeline (parser → executor)\n";
        constexpr long long M = 1000;
        Disruptor<int, 128> dp;
        int pid = dp.register_consumer();   // parser 註冊
        // executor 不直接讀 producer cursor, 它看 parser cursor

        std::atomic<long long> parsed_cursor{-1};
        std::atomic<long long> exec_count{0};

        std::thread parser([&]{
            long long seq = 0;
            while (seq < M) {
                long long avail = dp.producer_cursor();
                while (seq < avail) {
                    // 假裝 parse (這裡只是讀值)
                    (void)dp.at(seq);
                    ++seq;
                }
                parsed_cursor.store(seq - 1, std::memory_order_release);
                dp.mark_seen(pid, seq - 1);
                if (seq >= M) break;
                std::this_thread::yield();
            }
        });
        std::thread executor([&]{
            long long seq = 0;
            while (seq < M) {
                long long ready = parsed_cursor.load(std::memory_order_acquire);
                while (seq <= ready && seq < M) {
                    // executor 處理已被 parser 完成的事件
                    exec_count.fetch_add(1, std::memory_order_relaxed);
                    ++seq;
                }
                std::this_thread::yield();
            }
        });
        std::thread prod([&]{
            for (long long i = 0; i < M; ++i)
                dp.publish(static_cast<int>(i));
        });
        prod.join(); parser.join(); executor.join();
        std::cout << "  executor processed " << exec_count.load()
                  << " events (預期 " << M << ")\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：Disruptor 跟 SPSC ring (lesson 16) 主要差別在哪?
    //    A：SPSC 是「pop 語意」── consumer 拉走後該 slot 立刻可被 producer
    //       覆寫;只支援 1 producer + 1 consumer。Disruptor 是「廣播語意」
    //       ── slot 永遠不被 pop,只是被讀;每個 consumer 有自己的 cursor,
    //       同一份 event 每個 consumer 各看一次;producer 等的不是「tail」
    //       而是「最慢 consumer 的 cursor」。SPSC 適合 1:1 streaming,
    //       Disruptor 適合 1:N 廣播 (event sourcing、訊息匯流排)。
    //
    //  Q2：什麼是 sequence barrier?多 stage pipeline 怎麼用它?
    //    A：sequence barrier 是「等到某個 sequence 已被某些 cursor 通過」
    //       的同步點。多 stage pipeline 用法:consumer A 必須先處理完
    //       sequence S,consumer B 才能處理 S。實作上 B 的 barrier =
    //       「等 A_cursor >= S」。經典 pipeline:parse → validate →
    //       execute → log,四 stage 各跑一個核,同事件依序穿過 ── 每個
    //       stage 自己 sequence,B 的「上游 cursor」是 A 的 cursor。
    //       這把 ring 同時當 broadcast 與依賴 DAG 用。
    //
    //  Q3：為什麼 Disruptor 推崇「single writer principle」?
    //    A：一個變數若有 N 條 thread 同時寫 → cache line 在 N 個核之間
    //       ping-pong,效能崩塌 (見 lesson 13)。Disruptor 設計上每個
    //       cursor 只有「一條 thread 寫」(producer 寫 cursor、consumer
    //       i 寫 cursor_i),其他 thread 只讀 → cache line 大多時間在
    //       Shared 狀態,不需 invalidate broadcast。多 producer 場景就
    //       退化成 N 個 SPSC + 一個 multiplexer,而非直接 N 個 producer
    //       搶同個 ring。Aeron、Erlang OTP、Actor 模型都遵循這原則。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. Disruptor 的關鍵設計:
//      (a) Slot 永遠不會「pop」掉 ── slot 只是被讀過。
//      (b) 多 consumer 各自獨立、各自 cursor;互不影響。
//      (c) Producer 只跟「最慢的 consumer」同步,而且不是
//          鎖,而是讀一個 atomic min 而已。
//      (d) 一切資料結構 cache-line align (lesson 13)。
//      (e) 一切 sequence 推進都是 release/acquire (lesson 11)。
//
// 2. 為何 Disruptor 在金融、即時通訊、UI thread 廣播這類場景
//    特別合適:
//      - 廣播語意:同一份事件每個 stage / 每個 listener 都要
//        看一次 (用一般 queue 要發 N 份)。
//      - 低、可預測的延遲 (沒有 GC 抖動、沒有鎖)。
//      - 預分配 slot:無 heap、無 alloc latency。
//
// 3. Disruptor *不適合* 的場景:
//      - 多 producer:本課展示的是 SP*M*C。MP 版本要再加一層
//        「claim sequence」── 多個 producer 用 atomic CAS 競爭
//        下一個 slot,寫完之後等所有 ≤ self 的 producer 也都
//        publish 了才推 cursor。複雜許多。
//      - 動態結構 (要 pop / 要刪除特定項目)。Disruptor 假設
//        事件是「append-only event log」。
//
// 4. 與 lesson 16 SPSC ring 的關係:Disruptor 可以視為
//    「SPSC ring + 多個 consumer cursor」的廣義化。SPSC 是
//    「producer 等 consumer pop 後才能覆寫」;Disruptor 是
//    「producer 等 *最慢的* consumer 看過後才能覆寫」。
//
// 5. 與 lesson 27 (本課) 之外的進階變體:
//      - Multi-stage pipeline:consumer A 必須在 sequence S
//        看完後,consumer B 才能看 sequence S。用 barrier
//        實作。經典範例:把「parse → validate → execute → log」
//        拆成四個 stage,各自跑在自己的核心,同一個事件依序
//        經過。
//      - Aeron / NATS / Chronicle Queue 都是 LMAX 思路的
//        商業化變體。
//
// 6. 真實的 LMAX Disruptor (Java) 在現代硬體上能跑到 *每秒
//    上億事件* 的單機水準。我們這個 C++ demo 在沒有特別
//    調校下也能輕易跑出每秒數千萬事件 ── 比 lesson 17 sharded
//    queue 快好幾倍,正是因為「不 pop、不 alloc、不鎖」。
// =============================================================
