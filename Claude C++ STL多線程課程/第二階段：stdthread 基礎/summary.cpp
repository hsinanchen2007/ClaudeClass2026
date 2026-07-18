/*
 * ================================================================
 * 【第二階段：std::thread 基礎】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -pthread -o summary summary.cpp
 *
 * 本階段涵蓋課程：
 * - 課程 2.1：第一個多執行緒程式（建立、join、detach）
 * - 課程 2.2：執行緒函式的多種形式（函式、Lambda、成員函式、Functor）
 * - 課程 2.3：傳遞參數給執行緒（傳值、std::ref、std::move）
 * - 課程 2.4：join() 與 detach()（阻塞等待 vs 獨立運行）
 * - 課程 2.5：joinable() 狀態檢查（狀態轉換與安全模式）
 * - 課程 2.6：執行緒識別與資訊（get_id、hardware_concurrency、yield）
 * - 課程 2.7：執行緒的移動語意（move-only、容器管理、ScopedThread）
 * ================================================================
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <stdexcept>


// ===== 課程 2.1：第一個多執行緒程式 =====
//
// std::thread 是 C++11 引入的執行緒類別，建立物件時傳入可呼叫物件，
// 執行緒立即開始執行。
//
// 生命週期規則（非常重要！）：
//   建構 → 執行中 → 必須呼叫 join() 或 detach()
//   若在解構前未呼叫，程式會呼叫 std::terminate() 崩潰。
//
// join()：阻塞等待執行緒結束，之後 joinable() = false
// detach()：讓執行緒在背景獨立運行，之後 joinable() = false
//           主程式結束時，尚未完成的 detach 執行緒會被強制終止
//
// 多執行緒的輸出順序是不確定的（非確定性）。

namespace lesson_2_1 {

void sayHello() {
    std::cout << "[2.1] Hello from thread!" << std::endl;
}

// 展示 join() 的阻塞行為
void demoJoin() {
    std::cout << "[2.1] --- join() 示範 ---" << std::endl;

    std::thread t([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "[2.1]   子執行緒完成工作" << std::endl;
    });

    std::cout << "[2.1]   主執行緒等待中..." << std::endl;
    t.join();  // 阻塞到 t 完成
    std::cout << "[2.1]   join() 返回，繼續執行" << std::endl;
}

// 展示多執行緒同時執行（輸出順序不確定）
void demoMultiple() {
    std::cout << "[2.1] --- 多執行緒示範 ---" << std::endl;

    std::thread t1([]() {
        for (int i = 0; i < 3; ++i) std::cout << "A";
    });
    std::thread t2([]() {
        for (int i = 0; i < 3; ++i) std::cout << "B";
    });

    t1.join();
    t2.join();
    std::cout << std::endl;
    std::cout << "[2.1]   輸出順序不確定，這是多執行緒的本質" << std::endl;
}

} // namespace lesson_2_1


// ===== 課程 2.2：執行緒函式的多種形式 =====
//
// std::thread 接受任何「可呼叫物件（Callable）」：
//
// 1. 一般函式（Function）     - thread(func)
// 2. Lambda 表達式            - thread([](){})，最靈活，可捕獲外部變數
// 3. 成員函式（Member Func）  - thread(&Class::method, &obj)
//    語法：成員函式指標 + 物件指標 + 額外參數
// 4. 函式物件（Functor）      - thread{MyFunctor()}
//    注意 Most Vexing Parse：thread(MyFunctor()) 被解析為函式宣告
//    解決：thread((MyFunctor())) 或 thread{MyFunctor()}（推薦）

namespace lesson_2_2 {

// 形式 1：一般函式
void normalFunction() {
    std::cout << "[2.2]   [一般函式] 執行中" << std::endl;
}

// 形式 3：成員函式
class Worker {
public:
    void doWork() {
        std::cout << "[2.2]   [成員函式] 執行中" << std::endl;
    }
    void doWorkWithId(int id) {
        std::cout << "[2.2]   [成員函式帶參數] Worker " << id << " 執行中" << std::endl;
    }
};

// 形式 4：函式物件（Functor）
class Counter {
    int count;
public:
    explicit Counter(int c) : count(c) {}
    void operator()() const {
        std::cout << "[2.2]   [Functor] 計數：";
        for (int i = 0; i < count; ++i) std::cout << i << " ";
        std::cout << std::endl;
    }
};

void demoAllForms() {
    std::cout << "[2.2] --- 四種可呼叫物件示範 ---" << std::endl;

    Worker worker;

    // 1. 一般函式
    std::thread t1(normalFunction);

    // 2. Lambda（無捕獲）
    std::thread t2([]() {
        std::cout << "[2.2]   [Lambda] 執行中" << std::endl;
    });

    // 2b. Lambda（有捕獲）
    int value = 42;
    std::thread t2b([value]() {
        std::cout << "[2.2]   [Lambda 捕獲] value = " << value << std::endl;
    });

    // 3. 成員函式（需要物件指標）
    std::thread t3(&Worker::doWork, &worker);
    std::thread t3b(&Worker::doWorkWithId, &worker, 99);

    // 4. 函式物件（大括號初始化避免 Most Vexing Parse）
    std::thread t4{Counter(4)};

    t1.join(); t2.join(); t2b.join();
    t3.join(); t3b.join(); t4.join();
}

} // namespace lesson_2_2


// ===== 課程 2.3：傳遞參數給執行緒 =====
//
// std::thread 預設【複製】所有傳入的參數，即使函式期望引用也一樣。
//
// 傳遞方式總結：
// ┌─────────────────────────────────────────────┐
// │  傳值        thread(f, arg)    複製 arg      │
// │  傳引用      thread(f, ref(x)) 傳原始變數    │
// │  傳const引用 thread(f, cref(x))只讀          │
// │  傳指標      thread(f, &arg)   位址被複製    │
// │  移動語意    thread(f, move(p)) 轉移所有權   │
// └─────────────────────────────────────────────┘
//
// 傳字串陷阱：傳遞 char* 給期望 std::string 的函式，
//             配合 detach() 可能在轉換前緩衝區就被銷毀。
//             解決：明確轉換 std::string(buffer)。
//
// std::move() 用於只能移動的物件（如 std::unique_ptr）。

namespace lesson_2_3 {

void byValue(int x) {
    x = 999;  // 不影響原值
    std::cout << "[2.3]   byValue: x = " << x << "（副本被修改）" << std::endl;
}

void byRef(int& x) {
    x = 999;  // 修改原值
    std::cout << "[2.3]   byRef: x 已被修改為 " << x << std::endl;
}

void byString(const std::string& s) {
    std::cout << "[2.3]   byString: \"" << s << "\"" << std::endl;
}

void processUniquePtr(std::unique_ptr<int> ptr) {
    std::cout << "[2.3]   processUniquePtr: *ptr = " << *ptr << std::endl;
}

void demoParams() {
    std::cout << "[2.3] --- 參數傳遞示範 ---" << std::endl;

    int a = 1, b = 1;

    // 傳值：a 不被修改
    std::thread t1(byValue, a);
    t1.join();
    std::cout << "[2.3]   a = " << a << "（未被修改，如預期）" << std::endl;

    // std::ref 傳引用：b 被修改
    std::thread t2(byRef, std::ref(b));
    t2.join();
    std::cout << "[2.3]   b = " << b << "（已被修改）" << std::endl;

    // 明確轉換字串（避免 char* 陷阱）
    std::thread t3(byString, std::string("安全字串"));
    t3.join();

    // std::move 傳遞 unique_ptr（不可複製，只能移動）
    auto ptr = std::make_unique<int>(42);
    std::thread t4(processUniquePtr, std::move(ptr));
    t4.join();
    // ptr 現在是空的（所有權已轉移）
    std::cout << "[2.3]   ptr 移動後是否為空：" << (ptr == nullptr ? "是" : "否") << std::endl;
}

} // namespace lesson_2_3


// ===== 課程 2.4：join() 與 detach() =====
//
// 每個 std::thread 解構前必須明確選擇：
//
// join()
//   - 阻塞當前執行緒，直到目標執行緒結束
//   - 確保執行緒完成工作後才繼續
//   - 更安全，大多數情況應優先使用
//
// detach()
//   - 讓執行緒在背景獨立運行
//   - 立即返回，不等待
//   - 危險：detach 的執行緒不應捕獲或操作可能被提前銷毀的區域變數
//   - 適用場景：真正的背景任務（如日誌記錄器）
//
// 何時選擇？
//   需要結果 / 後續依賴執行緒完成 / 使用區域變數  → join
//   真正的背景任務 / 不需要結果                    → detach（謹慎）

namespace lesson_2_4 {

void demoJoinVsDetach() {
    std::cout << "[2.4] --- join vs detach 示範 ---" << std::endl;

    // join：確保完成
    std::thread t1([]() {
        std::cout << "[2.4]   [t1] 開始工作" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "[2.4]   [t1] 工作完成" << std::endl;
    });
    std::cout << "[2.4]   [main] 等待 t1..." << std::endl;
    t1.join();
    std::cout << "[2.4]   [main] t1 已結束，繼續" << std::endl;

    // detach：背景運行
    std::thread t2([]() {
        std::cout << "[2.4]   [t2] 背景執行中" << std::endl;
    });
    t2.detach();
    std::cout << "[2.4]   [main] t2 已分離（背景運行）" << std::endl;

    // 給 t2 一點時間完成輸出
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

// 危險示範（僅說明，此處不實際執行不安全版本）：
// void dangerous() {
//     int localVar = 42;
//     std::thread t([&localVar]() {  // 捕獲區域變數引用
//         std::this_thread::sleep_for(...);
//         std::cout << localVar; // localVar 已被銷毀！未定義行為
//     });
//     t.detach();
// }  // localVar 在此銷毀，但執行緒仍在運行

} // namespace lesson_2_4


// ===== 課程 2.5：joinable() 狀態檢查 =====
//
// joinable() 返回 true 表示執行緒物件關聯著一個真正的執行緒，
// 且尚未被 join 或 detach。
//
// joinable() == false 的情況：
//   - 預設建構的 std::thread（無關聯執行緒）
//   - 呼叫 join() 之後
//   - 呼叫 detach() 之後
//   - 被 std::move() 奪走所有權之後
//
// 對 non-joinable 執行緒呼叫 join() 或 detach() 會拋出 std::system_error。
//
// 最佳實踐：在類別的解構函式中一定要檢查 joinable() 後再 join。

namespace lesson_2_5 {

void demoJoinable() {
    std::cout << "[2.5] --- joinable() 狀態示範 ---" << std::endl;

    // 預設建構：不 joinable
    std::thread t0;
    std::cout << "[2.5]   預設建構 joinable: " << t0.joinable() << std::endl;

    // 帶執行緒：joinable
    std::thread t1([]() {});
    std::cout << "[2.5]   建立後 joinable: " << t1.joinable() << std::endl;
    t1.join();
    std::cout << "[2.5]   join 後 joinable: " << t1.joinable() << std::endl;

    // detach 後：不 joinable
    std::thread t2([]() {});
    t2.detach();
    std::cout << "[2.5]   detach 後 joinable: " << t2.joinable() << std::endl;

    // move 後：原物件不 joinable，新物件 joinable
    std::thread t3([]() {});
    std::thread t4 = std::move(t3);
    std::cout << "[2.5]   move 後原物件 joinable: " << t3.joinable() << std::endl;
    std::cout << "[2.5]   move 後新物件 joinable: " << t4.joinable() << std::endl;
    t4.join();
}

// 安全的執行緒管理類別：解構時自動 join
class SafeThread {
    std::thread t;
public:
    template<typename Func, typename... Args>
    void start(Func&& f, Args&&... args) {
        if (t.joinable()) t.join();  // 先結束舊的
        t = std::thread(std::forward<Func>(f), std::forward<Args>(args)...);
    }

    void join() {
        if (t.joinable()) t.join();
    }

    ~SafeThread() {
        join();  // 解構時自動 join，避免 terminate
    }
};

void demoSafeThread() {
    std::cout << "[2.5] --- SafeThread 示範 ---" << std::endl;
    SafeThread st;
    st.start([]() { std::cout << "[2.5]   SafeThread 任務 1" << std::endl; });
    st.start([]() { std::cout << "[2.5]   SafeThread 任務 2" << std::endl; });
    // 解構時自動 join 任務 2
}

} // namespace lesson_2_5


// ===== 課程 2.6：執行緒識別與資訊 =====
//
// 取得執行緒 ID：
//   std::this_thread::get_id()     - 在執行緒內部取得自己的 ID
//   thread_object.get_id()         - 在外部取得指定執行緒的 ID
//   std::thread::id 型別可比較和輸出
//
// 查詢硬體資訊：
//   std::thread::hardware_concurrency() - 回傳 CPU 核心數（可能為 0）
//   回傳 0 時應使用預設值（如 2 或 4）
//
// std::this_thread 命名空間：
//   get_id()      - 取得當前執行緒 ID
//   sleep_for()   - 休眠指定時間段
//   sleep_until() - 休眠到指定時間點
//   yield()       - 讓出 CPU 時間給其他執行緒（避免忙等待）

namespace lesson_2_6 {

// 儲存主執行緒 ID，供子執行緒比較
std::thread::id mainThreadId;

void checkThreadIdentity() {
    if (std::this_thread::get_id() == mainThreadId) {
        std::cout << "[2.6]   這是主執行緒" << std::endl;
    } else {
        std::cout << "[2.6]   這是子執行緒，ID: "
                  << std::this_thread::get_id() << std::endl;
    }
}

void demoThreadInfo() {
    std::cout << "[2.6] --- 執行緒識別與資訊示範 ---" << std::endl;

    mainThreadId = std::this_thread::get_id();
    std::cout << "[2.6]   主執行緒 ID: " << mainThreadId << std::endl;

    // 主執行緒身份確認
    checkThreadIdentity();

    // 子執行緒身份確認
    std::thread t(checkThreadIdentity);
    std::cout << "[2.6]   從外部看 t 的 ID: " << t.get_id() << std::endl;
    t.join();

    // 查詢 CPU 核心數
    unsigned int cores = std::thread::hardware_concurrency();
    if (cores == 0) cores = 2;  // 無法偵測時使用預設值
    std::cout << "[2.6]   硬體並行執行緒數: " << cores << std::endl;
}

// yield() 應用：避免忙等待
void demoYield() {
    std::cout << "[2.6] --- yield() 示範 ---" << std::endl;

    std::atomic<bool> ready{false};

    std::thread t([&ready]() {
        while (!ready) {
            std::this_thread::yield();  // 讓出 CPU，避免忙等待浪費資源
        }
        std::cout << "[2.6]   子執行緒收到信號，開始執行" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ready = true;
    t.join();
}

// 根據核心數建立執行緒池
void demoThreadPool() {
    std::cout << "[2.6] --- 根據核心數分配工作 ---" << std::endl;

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;
    std::cout << "[2.6]   建立 " << numThreads << " 個工作執行緒" << std::endl;

    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i]() {
            std::cout << "[2.6]   工作執行緒 " << i
                      << " (ID: " << std::this_thread::get_id() << ")" << std::endl;
        });
    }
    for (auto& t : threads) t.join();
}

} // namespace lesson_2_6


// ===== 課程 2.7：執行緒的移動語意 =====
//
// std::thread 是「只能移動（move-only）、不能複製」的型別。
//
// 原因：一個執行緒只能有一個擁有者。若允許複製，兩個物件指向同一個
//       執行緒，誰負責 join？重複 join 會崩潰。
//
// 移動操作：
//   std::thread t2 = std::move(t1);
//   移動後：t1 變成 non-joinable，t2 取得所有權。
//
// 函式可以回傳 std::thread（編譯器自動移動）。
// 函式可以接受 std::thread 參數（呼叫端需 std::move）。
//
// 警告：不可移動到仍是 joinable 的執行緒物件，否則 std::terminate()。
//
// 執行緒容器：std::vector<std::thread>，用 emplace_back 或 push_back 搭配 move。
//
// ScopedThread：RAII 包裝類別，離開作用域自動 join。

namespace lesson_2_7 {

// 複製會導致編譯錯誤（下面被注解的行）：
// std::thread t2 = t1;   // 錯誤：use of deleted function
// std::thread t2(t1);    // 錯誤：use of deleted function

// 函式回傳 std::thread（自動移動，RVO）
std::thread createWorker(int id) {
    return std::thread([id]() {
        std::cout << "[2.7]   工廠建立的執行緒 " << id << " 執行中" << std::endl;
    });
}

// 函式接受 std::thread（取得所有權）
void takeOwnership(std::thread t) {
    std::cout << "[2.7]   takeOwnership：取得執行緒所有權" << std::endl;
    if (t.joinable()) t.join();
}

// RAII 包裝：解構時自動 join
class ScopedThread {
    std::thread t;
public:
    explicit ScopedThread(std::thread thread)
        : t(std::move(thread)) {
        if (!t.joinable()) {
            throw std::logic_error("ScopedThread：執行緒不可 join");
        }
    }

    ~ScopedThread() {
        t.join();  // 自動 join
    }

    // 禁止複製
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;

    // 允許移動
    ScopedThread(ScopedThread&&) = default;
    ScopedThread& operator=(ScopedThread&&) = default;
};

void demoMoveSemantics() {
    std::cout << "[2.7] --- 移動語意示範 ---" << std::endl;

    // 手動移動所有權
    std::thread t1([]() {
        std::cout << "[2.7]   t1 的任務執行中" << std::endl;
    });
    std::cout << "[2.7]   t1 joinable: " << t1.joinable() << std::endl;

    std::thread t2 = std::move(t1);  // 所有權轉移

    std::cout << "[2.7]   移動後 t1 joinable: " << t1.joinable() << "（已空）" << std::endl;
    std::cout << "[2.7]   移動後 t2 joinable: " << t2.joinable() << "（取得所有權）" << std::endl;
    t2.join();  // 由 t2 負責
}

void demoFactory() {
    std::cout << "[2.7] --- 工廠函式示範 ---" << std::endl;

    std::thread t = createWorker(42);
    t.join();
}

void demoTakeOwnership() {
    std::cout << "[2.7] --- 所有權轉移到函式 ---" << std::endl;

    std::thread t([]() {
        std::cout << "[2.7]   工作執行緒" << std::endl;
    });
    takeOwnership(std::move(t));
    std::cout << "[2.7]   t joinable 後: " << t.joinable() << std::endl;
}

void demoThreadVector() {
    std::cout << "[2.7] --- std::vector<thread> 示範 ---" << std::endl;

    std::vector<std::thread> threads;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([i]() {
            std::cout << "[2.7]   向量中的執行緒 " << i << std::endl;
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}

void demoScopedThread() {
    std::cout << "[2.7] --- ScopedThread RAII 示範 ---" << std::endl;

    {
        ScopedThread st(std::thread([]() {
            std::cout << "[2.7]   ScopedThread 的任務" << std::endl;
        }));
        // 離開作用域時自動 join
    }
    std::cout << "[2.7]   ScopedThread 已自動 join" << std::endl;
}

} // namespace lesson_2_7


// ===== main()：完整示範第二階段所有概念 =====

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "  第二階段：std::thread 基礎 — 總複習" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << std::endl;

    // --- 課程 2.1 ---
    std::cout << "========== 課程 2.1：第一個多執行緒程式 ==========" << std::endl;
    {
        // 最簡單的執行緒
        std::thread t(lesson_2_1::sayHello);
        t.join();
    }
    lesson_2_1::demoJoin();
    lesson_2_1::demoMultiple();
    std::cout << std::endl;

    // --- 課程 2.2 ---
    std::cout << "========== 課程 2.2：執行緒函式的多種形式 ==========" << std::endl;
    lesson_2_2::demoAllForms();
    std::cout << std::endl;

    // --- 課程 2.3 ---
    std::cout << "========== 課程 2.3：傳遞參數給執行緒 ==========" << std::endl;
    lesson_2_3::demoParams();
    std::cout << std::endl;

    // --- 課程 2.4 ---
    std::cout << "========== 課程 2.4：join() 與 detach() ==========" << std::endl;
    lesson_2_4::demoJoinVsDetach();
    std::cout << std::endl;

    // --- 課程 2.5 ---
    std::cout << "========== 課程 2.5：joinable() 狀態檢查 ==========" << std::endl;
    lesson_2_5::demoJoinable();
    lesson_2_5::demoSafeThread();
    std::cout << std::endl;

    // --- 課程 2.6 ---
    std::cout << "========== 課程 2.6：執行緒識別與資訊 ==========" << std::endl;
    lesson_2_6::demoThreadInfo();
    lesson_2_6::demoYield();
    lesson_2_6::demoThreadPool();
    std::cout << std::endl;

    // --- 課程 2.7 ---
    std::cout << "========== 課程 2.7：執行緒的移動語意 ==========" << std::endl;
    lesson_2_7::demoMoveSemantics();
    lesson_2_7::demoFactory();
    lesson_2_7::demoTakeOwnership();
    lesson_2_7::demoThreadVector();
    lesson_2_7::demoScopedThread();
    std::cout << std::endl;

    std::cout << "================================================================" << std::endl;
    std::cout << "  第二階段完成！" << std::endl;
    std::cout << "  涵蓋：建立執行緒、可呼叫物件、參數傳遞、join/detach、" << std::endl;
    std::cout << "        joinable 狀態、執行緒 ID、移動語意與執行緒容器" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}
