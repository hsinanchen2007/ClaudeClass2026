// ============================================================================
// C++20 多執行緒總複習：正確性、生命週期、效能與面試決策
// ============================================================================
// 這是一份面試前可獨立閱讀的濃縮教材，涵蓋同目錄 01–30 章。先記住核心順序：
//   ownership -> invariant -> synchronization -> shutdown -> measurement
// 若連「誰能讀寫、何時停止、資料是否要排空」都沒定義，先寫 atomic/CAS 只會讓 bug
// 更難重現。
//
// ┌───────────────────────────┬───────────────────────────────┐
// │ 問題                      │ 優先工具                      │
// ├───────────────────────────┼───────────────────────────────┤
// │ 獨立工作 + RAII join      │ jthread                       │
// │ 單一結果/exception        │ async/future/promise          │
// │ 多欄共享 invariant        │ mutex + RAII lock             │
// │ 多把 mutex                │ scoped_lock / std::lock       │
// │ 讀多寫少                  │ shared_mutex（先 benchmark）  │
// │ 單一 counter/flag/state   │ atomic                        │
// │ 等某 predicate            │ condition_variable            │
// │ C++20 單一 atomic 值變更  │ atomic::wait/notify           │
// │ 一次性 N worker rendezvous│ latch                         │
// │ 多 phase rendezvous       │ barrier                       │
// │ N 個資源 permit           │ counting_semaphore            │
// │ 大量短 task               │ bounded thread pool           │
// │ 讀極多、寫少 snapshot     │ atomic shared_ptr + COW       │
// │ SPSC bounded stream       │ SPSC ring（前提不可放寬）     │
// └───────────────────────────┴───────────────────────────────┘
//
// 【01–07 基礎模型】
// 01 thread：join 建立 worker 完成到 caller 的 happens-before；detach 讓生命週期難證明。
// 02 data race：同址未同步衝突且至少一寫就是 UB；volatile 不提供 thread synchronization。
// 03 mutex：保護 invariant；unlock synchronizes-with 後續 lock；鎖外做 I/O/callback。
// 04 atomic：單 object 不可分割操作；多個 atomic 不自動形成一致 transaction。
// 05 condition variable：等待 state，不是 notification；永遠用 predicate 重查。
// 06 future：一次性 result/exception channel；async policy 與 future destructor 要理解。
// 07 jthread：destructor request_stop+join；取消合作式，blocking call 必須支援喚醒。
//
// 【08–15 執行架構、memory model、工具】
// 08 pool：重用 worker；bounded queue/backpressure；task 不可在持 queue lock 時執行。
// 09 Parallel STL：policy 不保證實際速度；par callable 無 race；backend 可能需 oneTBB。
// 10 shared_mutex：shared readers/exclusive writer；無 portable upgrade，公平性不保證。
// 11 memory model：sequenced-before + synchronizes-with => happens-before。
// 12 C++20 sync：latch 一次、barrier 多 phase、semaphore permits、atomic wait value-based。
// 13 false sharing：不同 object 仍爭同 cache line；padding/local reduction 要 profile。
// 14 TSan：動態覆蓋跑到的 interleaving，不是「零報告即形式證明」。
// 15 init_once：local static/call_once 正常返回才完成；exception 時後續可重試。
//
// 【16–20 進階資料結構與回收：沒有 sibling 也能速查】
// 16 SPSC ring：前提是「恰好一個 producer 寫 head、恰好一個 consumer 寫 tail」；因此
//    不需 CAS。producer 寫完 slot 後 release-publish head，consumer acquire-load head 後才讀
//    slot；反方向以 tail 防 producer 覆寫未讀資料。以空槽區分 full/empty 時 usable=N-1。
//    一旦 T 非 trivial，還要另管 slot 的 construct/destroy 與例外，不能只複製 bytes。
// 17 sharded MPMC：每個 shard 各有 mutex/queue，push/pop 在線性化點只鎖一 shard，可降低
//    contention；代價是不同 shard 間沒有 global FIFO。hash 偏斜會形成 hot shard，暫時 pop
//    不到不代表所有 producer 已完成，仍需 close + pending/progress protocol。
// 18 COW snapshot：atomic<shared_ptr<const State>> 讓 reader load 後擁有 immutable 版本；writer
//    copy、修改、CAS 發布。CAS 失敗會重做，mutator 不可含不可重播副作用；寫多時有 copy
//    amplification，慢 reader 又會延長舊版本生命。atomic shared_ptr 也不保證 lock-free。
// 19 hazard pointer：reader 先 load candidate、發布到 hazard slot、再重讀 shared pointer；若變了
//    就重試。remover 只能 retire node，掃描確認無 hazard 後才 delete。它解決 reclamation，
//    不自動解決 ABA、線性化或 hazard slot 耗盡；memory order 與 thread registration 都是契約。
// 20 coroutine：co_await 只把控制流轉成 coroutine frame/suspension，不會自帶 thread 或平行。
//    scheduler 通常在鎖內 enqueue、鎖外 resume；coroutine_handle 不擁有 frame，必須唯一 destroy。
//    promise 要保存 result/exception；final_suspend、continuation、取消與 pool shutdown 必須一起設計。
//
// 【21–30 production、演算法與低階工具：沒有 sibling 也能速查】
// 21 deadlock：Coffman 條件中通常以固定 lock hierarchy 或 scoped_lock 破壞 circular wait。
//    wait-for graph 不只含 mutex，也含 join/future/callback；RAII 只保證解鎖，不保證無死鎖。
//    try_lock+退避可變成 livelock；長期搶不到資源則是 starvation。
// 22 graceful shutdown：在同一 mutex 下把 running 線性化成 closing 並拒絕新工作，再依契約
//    drain 或 cancel pending，notify_all 讓所有 waiter 重查，最後在不持鎖時 join。close 應冪等，
//    deadline/剩餘工作/背景錯誤要可回報；先 join 再改 state/notify 是典型永久等待。
// 23 affinity：先讀 OS/cgroup allowed CPU mask，再以 opt-in OS API 綁定；它是 constraint/hint，
//    不保證獨占 core，也不等於 NUMA memory placement。錯誤 pinning 會破壞負載平衡，必須 benchmark。
// 24 async logger：queue 中 record 必須擁有資料，不能保存 caller string_view；單一背景 writer
//    序列化輸出。bounded queue 要明訂 block/drop/sample；close+drain+join 只證明交給 I/O，
//    不等於 fsync 後可抵抗斷電，寫入錯誤也必須有回報通道。
// 25 parallel mergesort：兩半獨立，可 async sort 後 merge；work O(n log n)，但 threshold/depth
//    必須限制 task 數，且明確用 launch::async。comparator 要 strict weak ordering；預配置 scratch
//    buffer 可減少每層 allocation，平行度仍應由 executor 控制而非遞迴無限生 thread。
// 26 parallel scan：operator 必須 associative；流程是各 block local scan、scan block totals、
//    再把 prefix offset 加回。只做第一步會讓右塊缺前綴。理想 work O(n)、span O(log n)，
//    但浮點重排可能改 rounding；scan 是 compaction、radix sort 與 GPU allocator 的基礎。
// 27 Disruptor：預配置 power-of-two ring，以單調 sequence 判斷 wrap/lag，再 modulo 映射 slot；
//    producer 不得越過最慢 consumer gating sequence，資料寫完才 release-publish。單一 published
//    sequence 只適合依序發布的 single producer；busy-spin/yield/block 是 latency/CPU 取捨。
// 28 mutex variants：lock_guard 最小 RAII；unique_lock 才支援 defer/move/unlock/CV；scoped_lock
//    一次取得多鎖。timed/try lock 要處理失敗，失敗者不可 unlock；recursive_mutex 常掩蓋
//    lock dependency 與過大 critical section，shared_mutex 也沒有 portable upgrade/fairness 保證。
// 29 atomic_flag/ref/fence：flag 可做極短 spinlock，但 holder 被 deschedule 時其他人只會燒 CPU；
//    atomic_ref 不擁有 object，要求 lifetime/alignment，且衝突存取不可混用 non-atomic。fence 本身
//    不傳資料，仍需某個 atomic reads-from 關係串起同步；能用 acquire/release object 就優先用。
// 30 interview：先定 shared invariant/owner，再指出 linearization point、happens-before、progress、
//    reclamation、failure 與 shutdown contract；最後才寫 code，並以 deterministic invariant、stress、
//    TSan、timeout 與 failure injection 驗證。測試沒重現不構成 race/deadlock 不存在的證明。
//
// 【memory_order 速查】
// relaxed：atomicity + modification order，不發布其他資料。
// acquire load：後方操作不能越過；讀到 release 值時取得之前資料。
// release store：前方操作不能越過；發布給讀到它的 acquire。
// acq_rel：用於 read-modify-write 同時取得並發布。
// seq_cst：以上再加所有 seq_cst 操作的單一全域順序；預設、最易推理。
// consume：實務編譯器通常提升成 acquire；教材不以它建立脆弱設計。
//
// 【progress guarantee】
// blocking：可能睡眠/等鎖；lock-free：整體至少一 thread 持續進展；wait-free：每個操作
// 有有界步數；obstruction-free：獨自執行可完成。lock-free 不等於無等待、不等於更快，
// 更不等於安全回收已自動解決。
//
// 【生命週期檢查清單】
// - thread/coroutine 是否可能存取已離開 scope 的 reference/string_view/span？
// - 擁有 worker 的 class，worker member 是否會在其依賴的 mutex/cv/data 之前銷毀？
// - future/task exception 是否有人 get/observe？
// - close 是否冪等？等待者是否全部 notify？queue 是 drain 還是 drop？
// - lock-free node 何時可 delete？誰證明沒有 reader？
//
// 【效能檢查清單】
// 先量 sequential baseline；分離 work/span；找 lock contention、queue depth、cache miss、
// false sharing、oversubscription、NUMA、task granularity。不要以 thread 數等於 core 數
// 當結論；I/O-bound 與 CPU-bound 策略不同。benchmark 不可用 debug build 單次 wall time。
// 【複雜度判讀】除了 Big-O work，平行演算法還要看 critical-path span、同步次數、
// task 建立成本與記憶體流量；O(n) 平行版本不保證勝過 cache-friendly 的 O(n) 單執行緒。

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <future>
#include <iostream>
#include <latch>
#include <mutex>
#include <queue>
#include <semaphore>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// ----------------------------------------------------------------------------
// 基礎整合：ownership partition + latch，不共享 read-modify-write
// ----------------------------------------------------------------------------
long long parallel_sum(const std::vector<int>& values)
{
    const std::size_t middle = values.size() / 2U;
    long long partial[2]{0LL, 0LL};
    std::latch complete(2);
    auto sum_range = [&](std::size_t slot, std::size_t first, std::size_t last) {
        long long local = 0;
        for (std::size_t index = first; index < last; ++index) local += values.at(index);
        partial[slot] = local;
        complete.count_down();
    };
    std::jthread left(sum_range, 0U, 0U, middle);
    std::jthread right(sum_range, 1U, middle, values.size());
    complete.wait();
    // latch wait 建立可見性；jthread 仍會在 scope 結束 join。
    return partial[0] + partial[1];
}

void basic_demo()
{
    // parallel_sum 會建立並同步 threads；NDEBUG 不可把整個教材操作刪除。
    [[maybe_unused]] const long long sum = parallel_sum({1, 2, 3, 4, 5, 6});
    assert(sum == 21LL);
}

// ----------------------------------------------------------------------------
// LeetCode 1115：Print FooBar Alternately（semaphore state machine）
// ----------------------------------------------------------------------------
class FooBar {
public:
    explicit FooBar(int repetitions) : repetitions_(repetitions) {}

    void foo()
    {
        for (int i = 0; i < repetitions_; ++i) {
            foo_turn_.acquire();
            output_ += "foo";
            bar_turn_.release();
        }
    }

    void bar()
    {
        for (int i = 0; i < repetitions_; ++i) {
            bar_turn_.acquire();
            output_ += "bar";
            foo_turn_.release();
        }
    }

    const std::string& result() const { return output_; }

private:
    int repetitions_;
    std::binary_semaphore foo_turn_{1};
    std::binary_semaphore bar_turn_{0};
    std::string output_;  // permit 保證任何時刻只有一個 writer。
};

void leetcode_demo()
{
    FooBar value(3);
    std::thread bar([&] { value.bar(); });
    std::thread foo([&] { value.foo(); });
    foo.join();
    bar.join();
    assert(value.result() == "foobarfoobarfoobar");
}

// ----------------------------------------------------------------------------
// 實務整合：固定 worker executor + future exception + graceful drain
// ----------------------------------------------------------------------------
// 狀態機：accepting=true 可 submit；close 改 false、喚醒；worker 只有 queue 空且關閉才退。
// packaged_task 把 value/exception 交給 future。close 後所有已接受 task 都完成。
class MiniExecutor {
public:
    explicit MiniExecutor(std::size_t worker_count)
    {
        if (worker_count == 0U) {
            throw std::invalid_argument("worker_count must be positive");
        }
        workers_.reserve(worker_count);
        try {
            for (std::size_t index = 0; index < worker_count; ++index) {
                static_cast<void>(index);
                workers_.emplace_back([this] { worker_loop(); });
            }
        } catch (...) {
            // thread 建立到一半失敗時，既有 worker 正等在 condition variable。
            // 必須在 stack unwinding 銷毀 jthread 前先改 predicate、通知並 join。
            request_shutdown();
            join_workers();
            throw;
        }
    }

    MiniExecutor(const MiniExecutor&) = delete;
    MiniExecutor& operator=(const MiniExecutor&) = delete;
    MiniExecutor(MiniExecutor&&) = delete;
    MiniExecutor& operator=(MiniExecutor&&) = delete;

    ~MiniExecutor() { close(); }

    std::future<int> submit(std::packaged_task<int()> task)
    {
        std::future<int> result = task.get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!accepting_) throw std::runtime_error("executor closed");
            tasks_.push(std::move(task));
        }
        condition_.notify_one();
        return result;
    }

    // close 採 drain 契約；lifecycle_mutex_ 讓多個「外部 owner thread」同時 close 時
    // 只有一位 joiner。仍不可從本 executor 的 task 內呼叫，否則 worker 會 join 自己。
    // production API 可另拆 request_close() 與 owner wait()，把這項限制做成型別/介面契約。
    void close() noexcept
    {
        std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
        request_shutdown();
        join_workers();
    }

    [[nodiscard]] int completed() const
    {
        return completed_.load(std::memory_order_relaxed);
    }

private:
    void request_shutdown() noexcept
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            accepting_ = false;
        }
        condition_.notify_all();
    }

    void join_workers() noexcept
    {
        for (std::jthread& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }
    void worker_loop()
    {
        for (;;) {
            std::packaged_task<int()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] { return !accepting_ || !tasks_.empty(); });
                if (tasks_.empty() && !accepting_) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
            completed_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<std::packaged_task<int()>> tasks_;
    bool accepting_ = true;
    std::atomic<int> completed_{0};
    std::vector<std::jthread> workers_;
    std::mutex lifecycle_mutex_;  // 序列化 joinable()/join()；不保護 task queue。
};

void practical_demo()
{
    [[maybe_unused]] bool rejected_zero_workers = false;
    try {
        MiniExecutor invalid(0U);
    } catch (const std::invalid_argument& error) {
        rejected_zero_workers = std::string{error.what()} == "worker_count must be positive";
    }
    assert(rejected_zero_workers);

    MiniExecutor executor(2U);
    auto first = executor.submit(std::packaged_task<int()>{[] { return 20; }});
    auto second = executor.submit(std::packaged_task<int()>{[] { return 22; }});
    auto failure = executor.submit(std::packaged_task<int()>{[]() -> int {
        throw std::runtime_error("bad task");
    }});

    // future::get() 會消耗 unique future 的結果，只能呼叫一次；先取值再 assert。
    [[maybe_unused]] const int first_result = first.get();
    [[maybe_unused]] const int second_result = second.get();
    assert(first_result + second_result == 42);
    try {
        static_cast<void>(failure.get());
        assert(false);
    } catch (const std::runtime_error& error) {
        assert(std::string{error.what()} == "bad task");
    }
    // 兩個外部 owner 同時 close：lifecycle mutex 保證只有一個 joiner。
    std::thread close_one([&] { executor.close(); });
    std::thread close_two([&] { executor.close(); });
    close_one.join();
    close_two.join();
    assert(executor.completed() == 3);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "MultiThread summary：同步、FooBar 與 executor drain 測試通過\n";
}

// ============================================================================
// 常見陷阱與面試快問快答
// ============================================================================
// Q1: data race 和 race condition 差別？
// A : data race 是 C++ 定義的未同步衝突且為 UB；race condition 是更廣泛的時序邏輯
//     問題，即使每筆存取有鎖，check-then-act 分成兩段仍可能錯。
//
// Q2: volatile 能修多執行緒嗎？
// A : 不能。它不保證 atomicity 或 happens-before；用 atomic/mutex。
//
// Q3: condition_variable 為何一定要 predicate？
// A : spurious wakeup 與早到 notification。共享 state 才是真相，notify 只是提示重查。
//
// Q4: mutex 與 atomic 如何選？
// A : 單一 counter/flag/RMW 可 atomic；跨多欄 invariant、複合 transaction 用 mutex。
//     memory ordering proof 不清楚時先用 seq_cst 或 mutex。
//
// Q5: lock-free 是否一定更快？
// A : 否。CAS retry、cache line bouncing、reclamation 與 tail latency 都可能更差。
//
// Q6: shared_mutex 一定改善讀多寫少？
// A : 否。讀 critical section 太短時 overhead 更高，公平性也可能造成 starvation。
//
// Q7: future.get 與 wait 差別？
// A : wait 只等待、不取值/不重拋；get 取得值或重拋 exception，且 unique future 只一次。
//
// Q8: async 不存 future 可以 fire-and-forget？
// A : 不可靠；temporary future destructor 可能等待，使呼叫反而同步。使用明確 executor。
//
// Q9: jthread request_stop 是否強制中止？
// A : 否，合作式。worker 要檢查 token，blocking wait 也要能因 stop 醒來。
//
// Q10: 如何避免 deadlock？
// A  : 全域 lock order、scoped_lock 多鎖、持鎖不呼叫 unknown code、不持鎖 join/wait。
//
// Q11: false sharing 如何判斷？
// A  : perf/profile coherence/cache 指標，搭配 layout 實驗；不是看到兩 atomic 相鄰就斷言。
//
// Q12: TSan 沒報告就安全？
// A  : 否，只代表這次 instrumented execution 未觀察到；仍需 code review/stress/design proof。
//
// Q13: SPSC ring 能否加第二 producer？
// A  : 不能。head 會變多 writer，需要 claim/CAS/per-slot sequence；前提改變即換演算法。
//
// Q14: COW snapshot 的 reader 為何不鎖？
// A  : reader 擁有 immutable shared_ptr 版本；writer 不修改舊版，而是原子發布新 copy。
//
// Q15: hazard pointer 的必要 recheck？
// A  : load pointer 到發布 hazard 中間可能已被移除；發布後重讀 shared pointer，不同就重試。
//
// Q16: coroutine 是不是較輕 thread？
// A  : 不是同一抽象。coroutine 是可暫停 state machine；在哪個 thread resume 由 scheduler。
//
// Q17: graceful shutdown 順序？
// A  : reject new -> mark close/stop -> notify -> drain/cancel per contract -> join -> report leftovers。
//
// Q18: affinity 為何可能更慢？
// A  : 阻止 scheduler 平衡、全 pin 同 core、忽略 NUMA、thermal/SMT competition。
//
// Q19: Parallel STL callback 可以 lock mutex 嗎？
// A  : par 可能可但會序列化；par_unseq 下 vectorization-unsafe blocking synchronization 不可用。
//
// Q20: 如何驗證 concurrency code？
// A  : deterministic unit invariants + repeated stress + TSan + timeout/deadlock diagnostics +
//     sanitizer 分開跑 + failure/shutdown injection + production metrics。
//
// 【最後練習】
// 1. 為 MiniExecutor 加 bounded queue，定義 submit_for timeout 與 close 競態的 linearization。
// 2. 讓 practical task 接 stop_token，區分 cancel pending 與 drain accepted。
// 3. 畫出 submit、worker pop、packaged_task set、future.get 的 happens-before 圖。
// 4. 不改正確性前提下，以 per-worker queue/work stealing 降低中央 mutex contention。
