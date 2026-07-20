// =============================================================================
//  課程 3.4：執行緒例外處理 — 第 4 部分：把例外傳遞封裝成 SafeThread 類別
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<thread>、<exception>、<stdexcept>、<functional>
//   標準版本：⚠️ 本檔【必須】用 -std=c++20，原因見下方【4.】
//             （核心的 exception_ptr 機制本身是 C++11）
//
//     class SafeThread {
//         template<typename Func, typename... Args>
//         explicit SafeThread(Func&& f, Args&&... args);
//         void join();      // join 後若有例外則在【呼叫端】重新拋出
//         ~SafeThread();    // RAII：確保執行緒被回收
//     };
//
//   複雜度：join() 為一次 pthread_join；例外重拋為 O(1) 的引用計數操作。
//
// 【詳細解釋 Explanation】
//
// 【1. 這個類別在解決什麼】
//   第 3 部分示範了 exception_ptr 的裸用法，但每次都要自己寫
//   try/catch(...) → current_exception() → 存起來 → join → 檢查 → rethrow，
//   樣板碼又長又容易漏掉。SafeThread 把這整套流程封裝起來，
//   讓呼叫端寫起來就像「同步函式呼叫」一樣自然：
//       try {
//           SafeThread st(mightThrow);
//           st.join();               // ← 例外在這裡重新拋出
//       } catch (const std::exception& e) { ... }
//   這正是 std::future 的設計理念，本檔等於在手工重造一個簡化版的 future。
//
// 【2. 為什麼把 rethrow 放在 join() 而不是解構子】
//   這是本檔最重要的設計決定。解構子【絕對不能】拋出例外：
//   C++11 起解構子預設 noexcept(true)，在裡面拋例外會直接 std::terminate()；
//   若又發生在 stack unwinding 期間更是必死。
//   所以「回報錯誤」必須放在一個【使用者主動呼叫、可以拋例外】的成員函式上，
//   join() 正是這個角色。
//   解構子則只負責「確保執行緒被回收」這個不可省略的清理動作，
//   絕不碰例外 —— 職責分離得很清楚。
//
// 【3. 解構子的 join 為什麼要檢查 joinable()】
//   若使用者已經呼叫過 join()，執行緒早就被回收，此時 t.joinable() 為 false，
//   再 join 一次會丟 std::system_error（在解構子裡 = terminate）。
//   所以解構子必須先檢查。這也讓「呼叫過 join」與「沒呼叫過 join」
//   兩條路徑都安全 —— 前者解構子什麼都不做，後者解構子補做回收。
//   ⚠️ 但要注意：走解構子這條路時，例外會被【安靜吞掉】（見注意事項 2）。
//
// 【4. 為什麼本檔必須用 C++20 —— 一個容易被誤判的細節】
//   關鍵在建構子裡這一行 lambda 的 init-capture：
//       [this, f = std::forward<Func>(f), ...args = std::forward<Args>(args)]
//                                        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//   「在 lambda capture 中展開參數包（pack expansion in init-capture）」
//   是 C++20 才加入的功能（P0780R2）。
//   陷阱在於：GCC 用 -std=c++17 編譯【也會通過】，因為它把這個語法
//   當作編譯器擴充放行，只有加上 -pedantic-errors 才會如實報錯：
//       error: pack init-capture only available with '-std=c++20' or '-std=gnu++20'
//   本檔已實測驗證此事。這正是教材中判定「某功能屬於哪個標準」時
//   一定要用 -pedantic-errors 的原因 —— 只用 -std=c++17 會得到錯誤結論。
//
//   C++17 以前的等價寫法是把參數包先打包成 std::tuple 再用 std::apply 展開，
//   明顯繁瑣，這也是 P0780R2 提案的動機。
//
// 【5. 為什麼 lambda 要標 mutable】
//   init-capture 的變數預設是 const 的。f(args...) 若 f 是可變的
//   callable（例如帶狀態的 mutable lambda），或需要以非 const 方式使用
//   捕獲的 args，就會編譯失敗。標上 mutable 即可解除這個限制。
//
// 【概念補充 Concept Deep Dive】
//   * exPtr 成員的執行緒安全：worker lambda 在執行緒中寫入 exPtr，
//     main 在 join() 之後讀取。標準規定執行緒完成 synchronizes-with
//     join() 返回，建立了 happens-before 關係，因此不需要 mutex 或 atomic。
//     這個保證【只在有 join 的前提下成立】—— 若改成 detach 就完全不安全了。
//
//   * f(args...) 而非 f(std::move(args)...)：這是刻意的保守選擇。
//     用 move 會讓「同一個 callable 被呼叫兩次」時第二次拿到被搬空的值。
//     本類別的 lambda 只會執行一次，理論上可以 move，
//     但保持 lvalue 傳遞語意更不易出錯。真正的產品級實作
//     （如 std::thread 本身）會用 std::invoke + decay-copy 的完整規則處理。
//
//   * 與 std::future 的比較：
//       SafeThread   —— 手工版，只支援「無回傳值」，例外在 join() 拋出。
//       std::future  —— 標準版，支援回傳值與例外，get() 一次取回兩者，
//                       還支援 wait_for/wait_until/shared_future。
//     實務上請直接用 std::async / std::future（見第 5 部分）。
//     本檔的價值在於【看清 future 內部到底做了什麼】。
//
//   * 為什麼建構子是 explicit：避免 SafeThread st = someLambda; 這種
//     隱式轉換。建立執行緒是重量級副作用，應該明確寫出。
//
// 【注意事項 Pay Attention】
//   1. 本檔【必須】用 -std=c++20。用 -std=c++17 看似能編，但那是 GCC 擴充；
//      加 -pedantic-errors 就會如實報錯（已實測）。
//   2. 【重要限制】若使用者【沒有】呼叫 join()，解構子雖會回收執行緒，
//      但儲存的例外會被【安靜吞掉】—— 解構子不能拋例外。
//      這是本設計的已知取捨。要避免就必須主動呼叫 join()。
//      本檔的 demoSwallowedException() 會實際展示這個現象。
//   3. join() 只能呼叫一次。第二次呼叫會對 non-joinable 的 thread 操作，
//      丟出 std::system_error。
//   4. exPtr 只在有 join() 的前提下才是執行緒安全的；改成 detach 就會失去
//      happens-before 保證。
//   5. 這個類別不可複製也未定義移動，因為它含有 std::thread 成員。
//      要放進容器請先補上 noexcept 的移動建構子。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】封裝跨執行緒例外傳遞
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 SafeThread 把 rethrow 放在 join()，而不是放在解構子裡？
//     答：解構子預設是 noexcept(true)，在裡面拋例外會直接 std::terminate()；
//         若發生在 stack unwinding 期間更是必死。所以「回報錯誤」必須放在
//         使用者主動呼叫、允許拋例外的成員函式上。解構子只負責「確保執行緒
//         被回收」這個不可省略的清理，兩者職責分離。
//     追問：那使用者忘記呼叫 join() 會怎樣？
//         → 解構子仍會回收執行緒（不會 terminate），但儲存的例外會被
//           【安靜吞掉】，錯誤就此消失。這是這個設計已知的取捨，
//           也是實務上該直接用 std::future 的理由之一。
//
// 🔥 Q2. 這個類別和 std::future 有什麼關係？
//     答：本質上是同一套機制的手工簡化版。std::async / std::promise 內部
//         同樣是用 exception_ptr 把例外存進共享狀態，future::get() 做的
//         就是「若共享狀態存的是例外則 rethrow_exception」。
//         差別在 future 還支援回傳值、wait_for/wait_until 與 shared_future，
//         而 SafeThread 只處理例外、不處理回傳值。
//     追問：那實務上該用哪個？
//         → 直接用 std::async / std::future。手寫版的價值在於理解原理，
//           不在於取代標準設施。
//
// ⚠️ 陷阱. 這個檔案用 g++ -std=c++17 編譯居然通過了，
//         是不是代表 pack init-capture 在 C++17 就能用？
//     答：不是。GCC 把它當作【編譯器擴充】默默放行了。
//         加上 -pedantic-errors 就會如實報錯：
//           error: pack init-capture only available with '-std=c++20'
//         「在 lambda capture 中展開參數包」是 C++20 的 P0780R2 才加入的。
//     為什麼會錯：把「編譯器接受」等同於「標準允許」。GCC/Clang 預設
//         啟用大量擴充，-std=c++17 只設定基準而非嚴格模式。
//         判定標準版本【一定】要加 -pedantic-errors，否則會得到錯誤結論 ——
//         程式在另一個編譯器或嚴格模式下就會編不過。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺
//   本檔是資源管理與錯誤傳遞的類別設計練習。LeetCode 並行題
//   （1114/1115/1116/1117/1195）由題目框架自行建立執行緒，
//   解法只需實作被呼叫的成員函式，根本碰不到執行緒的建立與回收，
//   也不涉及例外傳遞。故沒有對應題目，從缺。

#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 【核心類別】SafeThread：把「例外裝箱 → 跨執行緒 → 重拋」封裝起來
// -----------------------------------------------------------------------------
class SafeThread {
    std::thread t;
    std::exception_ptr exPtr = nullptr;

public:
    template<typename Func, typename... Args>
    explicit SafeThread(Func&& f, Args&&... args) {
        // ⚠️ 下面的 ...args = std::forward<Args>(args) 是【C++20】的
        //    pack init-capture（P0780R2）。GCC 在 -std=c++17 下會當擴充放行，
        //    加 -pedantic-errors 才會如實報錯。
        t = std::thread([this, f = std::forward<Func>(f),
                         ...args = std::forward<Args>(args)]() mutable {
            try {
                f(args...);
            } catch (...) {
                // 裝箱：把任意型別的例外原封不動存起來
                exPtr = std::current_exception();
            }
        });
    }

    void join() {
        t.join();           // join 建立 happens-before，之後讀 exPtr 才安全
        if (exPtr) {
            std::rethrow_exception(exPtr);   // 在【呼叫端的堆疊】上重新拋出
        }
    }

    ~SafeThread() {
        // 只負責回收，絕不碰例外（解構子不能拋）
        if (t.joinable()) {
            t.join();
        }
    }
};

// -----------------------------------------------------------------------------
// 【對照示範】忘記呼叫 join() → 例外被安靜吞掉
//   這是本設計的已知取捨，實際跑一次看清楚它的後果。
// -----------------------------------------------------------------------------
void demoSwallowedException() {
    std::cout << "  情境 A：有呼叫 join()" << std::endl;
    try {
        SafeThread st([]() { throw std::runtime_error("A 的錯誤"); });
        st.join();
        std::cout << "    （不會執行到這裡）" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "    → 成功捕獲: " << e.what() << std::endl;
    }

    std::cout << "  情境 B：忘記呼叫 join()" << std::endl;
    try {
        SafeThread st([]() { throw std::runtime_error("B 的錯誤"); });
        // 這裡沒有 st.join(); → 解構子回收執行緒，但例外無處可拋
    } catch (const std::exception& e) {
        std::cout << "    → 捕獲: " << e.what() << std::endl;
    }
    std::cout << "    → 什麼都沒印出：例外被解構子安靜吞掉了" << std::endl;
}

// -----------------------------------------------------------------------------
// 【對照示範】帶參數的完美轉發，以及非 std::exception 型別的例外
// -----------------------------------------------------------------------------
void demoPerfectForwarding() {
    try {
        SafeThread st(
            [](const std::string& table, int rowId) {
                std::cout << "    正在處理 " << table << " 的第 " << rowId
                          << " 列" << std::endl;
                if (rowId < 0) {
                    throw std::invalid_argument("rowId 不可為負數");
                }
            },
            std::string("orders"), -1);
        st.join();
    } catch (const std::invalid_argument& e) {
        std::cout << "    → 捕獲 invalid_argument: " << e.what() << std::endl;
    }

    // exception_ptr 對【任意型別】都有效，不限 std::exception 的衍生類別
    try {
        SafeThread st([]() { throw 404; });
        st.join();
    } catch (int code) {
        std::cout << "    → 捕獲 int 型別的例外: " << code << std::endl;
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔平行載入與驗證（啟動期 fail-fast）
//   情境：服務啟動時要同時載入多份設定（資料庫連線、快取、憑證、路由表）。
//         為了縮短啟動時間，四份設定平行載入；但只要任一份無效，
//         服務就【必須立刻啟動失敗】並明確告訴維運人員是哪一份、錯在哪 ——
//         帶著壞設定啟動比啟動失敗危險得多。
//   為何用 SafeThread：每個載入器可能拋出不同型別的驗證例外，
//         需要把型別完整的錯誤帶回主執行緒。SafeThread 讓呼叫端
//         寫起來就像一般的同步呼叫，錯誤處理集中在一個 try/catch。
//   設計重點：先全部建立（平行開跑），再逐一 join —— 而不是
//         「建一個 join 一個」，否則就退化成序列執行、失去平行的意義。
// -----------------------------------------------------------------------------
struct ConfigError : std::runtime_error {
    std::string section;
    ConfigError(std::string sec, const std::string& msg)
        : std::runtime_error(msg), section(std::move(sec)) {}
};

void loadConfigSection(const std::string& name, bool valid, int delayMs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    if (!valid) {
        throw ConfigError(name, "設定區段 [" + name + "] 驗證失敗：缺少必要欄位");
    }
}

void demoConfigLoading() {
    // 先全部建立 → 四份設定【平行】載入
    std::vector<std::unique_ptr<SafeThread>> loaders;
    loaders.push_back(std::make_unique<SafeThread>(loadConfigSection,
                                                   std::string("database"), true, 40));
    loaders.push_back(std::make_unique<SafeThread>(loadConfigSection,
                                                   std::string("cache"), true, 30));
    loaders.push_back(std::make_unique<SafeThread>(loadConfigSection,
                                                   std::string("tls"), false, 20));
    loaders.push_back(std::make_unique<SafeThread>(loadConfigSection,
                                                   std::string("routes"), true, 10));

    // 再逐一 join：任一份失敗就 fail-fast
    int ok = 0;
    for (auto& loader : loaders) {
        try {
            loader->join();
            ++ok;
        } catch (const ConfigError& e) {
            std::cout << "  ✗ 啟動失敗於區段 [" << e.section << "]" << std::endl;
            std::cout << "    原因: " << e.what() << std::endl;
            std::cout << "  （已成功載入 " << ok << " 份設定，服務拒絕帶著壞設定啟動）"
                      << std::endl;
            return;
        }
    }
    std::cout << "  ✓ 全部 " << ok << " 份設定載入成功，服務可以啟動" << std::endl;
}

int main() {
    std::cout << "=== 基本示範：SafeThread 把例外帶回呼叫端 ===" << std::endl;
    try {
        SafeThread st([]() {
            throw std::runtime_error("錯誤！");
        });
        st.join();
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }

    std::cout << "\n=== 已知取捨：忘記 join() 例外會被吞掉 ===" << std::endl;
    demoSwallowedException();

    std::cout << "\n=== 完美轉發與任意例外型別 ===" << std::endl;
    demoPerfectForwarding();

    std::cout << "\n=== 日常實務：平行載入設定檔（啟動期 fail-fast） ===" << std::endl;
    demoConfigLoading();

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra -pthread "課程 3.4：執行緒例外處理4.cpp" -o except4
//   ⚠️ 本檔【必須】用 -std=c++20。建構子中的 pack init-capture
//      （...args = std::forward<Args>(args)）是 C++20 的 P0780R2。
//      用 -std=c++17 看似可編譯，但那是 GCC 的擴充；
//      加上 -pedantic-errors 就會如實報錯（已實測）：
//        error: pack init-capture only available with '-std=c++20' or '-std=gnu++20'

// 註:
//   （本檔輸出是【確定的】：每一段都在 join() 之後才繼續，
//   平行載入的四份設定也是依索引順序 join，所以失敗回報順序固定。）

// === 預期輸出 ===
// === 基本示範：SafeThread 把例外帶回呼叫端 ===
// 捕獲: 錯誤！
//
// === 已知取捨：忘記 join() 例外會被吞掉 ===
//   情境 A：有呼叫 join()
//     → 成功捕獲: A 的錯誤
//   情境 B：忘記呼叫 join()
//     → 什麼都沒印出：例外被解構子安靜吞掉了
//
// === 完美轉發與任意例外型別 ===
//     正在處理 orders 的第 -1 列
//     → 捕獲 invalid_argument: rowId 不可為負數
//     → 捕獲 int 型別的例外: 404
//
// === 日常實務：平行載入設定檔（啟動期 fail-fast） ===
//   ✗ 啟動失敗於區段 [tls]
//     原因: 設定區段 [tls] 驗證失敗：缺少必要欄位
//   （已成功載入 2 份設定，服務拒絕帶著壞設定啟動）
