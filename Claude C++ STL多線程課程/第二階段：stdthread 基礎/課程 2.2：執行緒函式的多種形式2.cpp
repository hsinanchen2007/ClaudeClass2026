// =============================================================================
//  課程 2.2：執行緒函式的多種形式2.cpp  —  Lambda 表達式作為執行緒入口
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：#include <thread>
//   標準版本：lambda 本身是 C++11；init-capture（[x = expr]）與 generic lambda
//             （[](auto x){}）是 C++14；[*this] 捕獲物件複本是 C++17；
//             lambda 可為 constexpr 是 C++17；樣板參數列 []<class T>(T){} 是 C++20。
//   語法：std::thread t([捕獲列](參數列) mutable -> 回傳型別 { 函式本體 });
//   複雜度：lambda 本身沒有執行期額外開銷（closure 就是個帶成員的結構，
//           operator() 通常可完全 inline）；成本來自「開一條執行緒」。
//
// 【詳細解釋 Explanation】
//
// 【1. lambda 到底是什麼——編譯器幫你寫的一個 class】
//   lambda 不是語言的特殊魔法，而是語法糖。編譯器看到
//     int value = 42;
//     auto f = [value]() { std::cout << value; };
//   會生成一個大致長這樣的「closure type」（匿名、獨一無二）：
//     class __lambda_7_15 {
//         int value;                                    // 捕獲的東西 → 成員變數
//     public:
//         explicit __lambda_7_15(int v) : value(v) {}
//         void operator()() const { std::cout << value; }   // 預設 const！
//     };
//   所以 lambda 其實就是「檔案 4、5 講的 functor」，只是不用自己動手寫。
//   理解這一點，很多行為就變得理所當然：
//     * 捕獲 = 成員變數 → 有生命週期問題。
//     * 預設 operator() 是 const → 所以要改捕獲值必須加 mutable。
//     * 每個 lambda 都是「獨一無二的型別」→ 兩個長得一模一樣的 lambda
//       型別也不同，所以只能用 auto 接，不能寫出它的型別名。
//
// 【2. 為什麼 lambda 是四種形式裡最常用的】
//   自由函式沒辦法帶狀態；functor 要另外定義一個 class，離使用點很遠。
//   lambda 把「要跑什麼」和「要用到哪些資料」寫在同一個地方，
//   對「一次性的、需要外部資料的背景任務」最自然。
//
// 【3. 兩段複製：捕獲一次，thread 建構子再一次】
//   這是本檔最重要的機制，很多人只知道其中一半：
//     第一次複製：建立 lambda 時，[value] 把 value 複製進 closure 物件。
//                 此後外面的 value 怎麼改，closure 裡那份都不變。
//     第二次複製：std::thread 建構子對「整個 closure 物件」做 decay-copy，
//                 把它搬進新執行緒的儲存區（左值→複製建構、臨時物件→移動建構）。
//   結果是：新執行緒操作的是 closure 的複本，和 main 的區域變數完全脫鉤。
//   這正是「按值捕獲很安全、按引用捕獲很危險」的根源——按引用捕獲時，
//   closure 裡存的是「參考／位址」，複製它只是複製那個位址，
//   指向的東西還是 main 的區域變數。
//
// 【4. 按值 [=] vs 按引用 [&]：什麼時候可以用 [&]】
//   * [&] + join()：通常安全。因為 join 保證「執行緒在該作用域結束前跑完」，
//     被引用的區域變數在那之前都還活著。
//   * [&] + detach()：非常危險。detach 之後 main 可能立刻離開作用域，
//     區域變數被銷毀，而執行緒還在用它的位址 → dangling reference。
//     這是未定義行為：可能印出垃圾值、可能看似正常、也可能 crash，
//     不保證是哪一種（絕不能寫成「一定會 crash」）。
//   結論：預設用按值捕獲；要用 [&] 就必須能說清楚「誰保證它活得夠久」。
//
// 【5. mutable 的真正意義】
//   [value]() mutable { ++value; } 中的 mutable 只是把生成的 operator()
//   從 const 變成非 const，讓你能修改「closure 自己那份複本」。
//   它完全不會影響外面的原變數——外面那份從頭到尾沒被碰過。
//   把 mutable 理解成「讓我改我自己的複本」，就不會誤以為它是「傳引用」。
//
// 【概念補充 Concept Deep Dive】
//
// (A) closure 的記憶體佈局
//   closure 物件的大小就是「所有捕獲項的大小總和 + 對齊」。
//     [](){}                 → 無捕獲，是空類別，sizeof 為 1（C++ 規定完整
//                              物件不可為 0 大小，以保證不同物件位址相異）。
//     [i](){}     (int i)    → sizeof 通常 4。
//     [&i](){}              → 存的是參考，實作上是一個指標，sizeof 通常 8。
//   本檔 main 會實測印出這些值。
//
// (B) 無捕獲 lambda 可以轉成函式指標，有捕獲的不行
//   無捕獲 lambda 有一個「轉換成函式指標」的隱式轉換運算子，所以能傳給
//   C API（如 pthread_create、qsort 的比較函式）。
//   一旦有任何捕獲，closure 就帶了狀態，而裸函式指標沒有地方存那些狀態，
//   因此該轉換不存在。這也是為什麼 C 風格 API 幾乎都設計成
//   「函式指標 + void* user_data」——那個 void* 就是手動版的捕獲。
//
// (C) 為什麼每個 lambda 型別都不同
//   標準規定每個 lambda 運算式都產生一個獨一無二的 closure type。
//   即使兩個 lambda 原始碼一字不差，型別仍然不同。因此：
//     * 不能寫 std::vector<那個 lambda 的型別>，要用 std::function 抹除型別
//       （代價是可能有堆積配置與間接呼叫，見檔案 6 的事件註冊表範例）。
//     * 兩個不同 lambda 不能互相指派。
//
// 【注意事項 Pay Attention】
//   1. [&] 捕獲 + detach() 是典型的 dangling reference 來源，屬未定義行為，
//      不保證任何特定結果（可能看似正常，也可能 crash）。
//   2. [=] 在 C++11~17 捕獲成員變數時，實際捕獲的是 this 指標，不是物件複本——
//      所以物件死了之後 closure 仍會透過 this 存取已銷毀的成員（UB）。
//      C++17 起可寫 [*this] 捕獲整個物件的複本；[=, this] 在 C++20 是
//      明確寫法（C++20 起單純用 [=] 隱含捕獲 this 已被標記為 deprecated）。
//   3. mutable 改的是 closure 自己的複本，不會影響外面的變數。
//   4. 多條執行緒同時對 std::cout 輸出：std::cout 本身有內部同步，不會產生
//      data race（C++11 起標準要求 <iostream> 物件的格式化輸出對並行呼叫是
//      安全的），但「行與行之間的交錯順序」完全不保證，甚至同一行的多個
//      << 之間也可能被插隊。要保證整行完整，得自己加 mutex。
//   5. lambda 的回傳值一樣被丟棄（同自由函式）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Lambda 作為執行緒入口
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 [value] 捕獲之後，執行緒裡看到的一定是捕獲當下的值？
//     答：因為發生了兩次複製。第一次是建立 lambda 時把 value 複製成 closure
//         的成員；第二次是 std::thread 建構子對整個 closure 做 decay-copy，
//         搬進新執行緒的儲存區。執行緒操作的是複本的複本，
//         和原變數已經完全脫鉤，之後外面怎麼改都影響不到它。
//     追問：那 [&value] 呢？→ closure 存的是位址，複製 closure 只是複製位址，
//         指向的還是原變數；配 join() 通常安全，配 detach() 就是 dangling。
//
// 🔥 Q2. 無捕獲的 lambda 和有捕獲的 lambda，最大的實務差別是什麼？
//     答：無捕獲 lambda 可隱式轉成函式指標，因此能傳給 pthread_create、
//         qsort 這類 C API；有捕獲的不行，因為裸函式指標沒有地方存狀態。
//         這也解釋了 C API 為何都長成「函式指標 + void* user_data」。
//     追問：那有捕獲的要怎麼傳給 C API？→ 把 closure 位址放進 void* 參數，
//         在跳板函式裡 cast 回來呼叫；或改用 std::function 之類的包裝。
//
// ⚠️ 陷阱. 「在成員函式裡寫 std::thread t([=]{ use(member_); }); 是按值捕獲，
//           所以就算物件死了也安全。」
//     答：錯。在 C++11~17，[=] 遇到成員變數時捕獲的是 this 指標，
//         而不是成員的複本。物件一旦銷毀，closure 仍會透過那個 this
//         去存取已銷毀的成員——未定義行為。
//     為什麼會錯：把「[=] 表示全部按值」直覺套用到成員上。實際上成員不是
//         獨立的可捕獲實體，能被捕獲的只有 this；要複製整個物件必須用
//         C++17 的 [*this]，或先把需要的成員複製成區域變數再捕獲。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>   // std::min

// -----------------------------------------------------------------------------
// 【日常實務範例】並行下載佇列：每條 worker 用 lambda 捕獲自己的分片索引
//   情境：要抓一批 URL，開 N 條 worker 各自負責一段（static partition）。
//   為什麼用 lambda：每條 worker 需要「自己的編號與範圍」，這是最典型的
//         「一次性任務 + 需要捕獲外部資料」，用 lambda 最直接。
//   重點：
//     * begin/end/id 都是按值捕獲 → 每條執行緒有自己的複本，互不干擾。
//     * urls 與 results 是按引用捕獲 → 兩者都活到 join() 之後，安全。
//     * results 每個 slot 只被一條執行緒寫入（不同 index）→ 沒有 data race，
//       所以不需要對 results 加鎖。
//     * cout 另外用 mutex 保護，是為了讓「整行」不被插隊（不是為了避免 UB）。
// -----------------------------------------------------------------------------
std::mutex g_cout_mtx;

void parallelFetch(const std::vector<std::string>& urls, int workers) {
    std::vector<std::string> results(urls.size());
    std::vector<std::thread> pool;

    const size_t chunk = (urls.size() + workers - 1) / static_cast<size_t>(workers);
    for (int id = 0; id < workers; ++id) {
        const size_t begin = static_cast<size_t>(id) * chunk;
        if (begin >= urls.size()) break;
        const size_t end = std::min(begin + chunk, urls.size());

        // id/begin/end 按值捕獲；urls/results 按引用捕獲（活得比執行緒久）
        pool.emplace_back([id, begin, end, &urls, &results]() {
            for (size_t i = begin; i < end; ++i) {
                results[i] = "200 OK <- " + urls[i];   // 各寫各的 slot，無 race
            }
            std::lock_guard<std::mutex> lk(g_cout_mtx);
            std::cout << "  [worker " << id << "] 完成 " << (end - begin)
                      << " 個 URL（index " << begin << ".." << (end - 1) << "）\n";
        });
    }
    for (auto& t : pool) t.join();   // 所有引用捕獲的物件在此之前都還活著

    std::cout << "  彙總：" << results.size() << " 筆，第一筆 = " << results.front() << "\n";
}

int main() {
    std::cout << "=== 無捕獲 lambda ===" << std::endl;
    std::thread t1([]() {
        std::cout << "Lambda 1" << std::endl;
    });
    t1.join();

    std::cout << "\n=== 按值捕獲：捕獲當下就定案 ===" << std::endl;
    int value = 42;
    std::thread t2([value]() {
        std::cout << "Value: " << value << std::endl;
    });
    value = 999;      // 改的是 main 的變數；closure 裡那份是 42，不受影響
    t2.join();
    std::cout << "（main 這邊 value 已改成 " << value
              << "，但執行緒印的是捕獲當下的 42）" << std::endl;

    std::cout << "\n=== mutable：改的是 closure 自己的複本 ===" << std::endl;
    int counter = 10;
    std::thread t3([counter]() mutable {
        ++counter;                                  // 只動複本
        std::cout << "  closure 內 counter = " << counter << std::endl;
    });
    t3.join();
    std::cout << "  main 的 counter 仍是 " << counter << "（完全沒被動過）" << std::endl;

    std::cout << "\n=== closure 的大小 = 捕獲項大小總和 ===" << std::endl;
    int a = 0;
    auto l0 = []() {};
    auto l1 = [a]() { (void)a; };
    auto l2 = [&a]() { (void)a; };
    std::cout << "  sizeof([](){})   = " << sizeof(l0)
              << "  <- 空類別，C++ 規定完整物件不可為 0 大小" << std::endl;
    std::cout << "  sizeof([a](){})  = " << sizeof(l1) << "  <- 存了一個 int" << std::endl;
    std::cout << "  sizeof([&a](){}) = " << sizeof(l2) << "  <- 存的是位址（實作定義）" << std::endl;

    std::cout << "\n=== 無捕獲 lambda 可轉成函式指標 ===" << std::endl;
    void (*fp)() = []() { std::cout << "  我是透過函式指標被呼叫的" << std::endl; };
    std::thread t4(fp);
    t4.join();
    // 若上面 lambda 改成 [value]{...}，這行轉換會編譯失敗——有捕獲就沒有該轉換

    std::cout << "\n=== 日常實務：並行下載佇列 ===" << std::endl;
    std::vector<std::string> urls = {
        "https://example.com/a", "https://example.com/b",
        "https://example.com/c", "https://example.com/d",
        "https://example.com/e",
    };
    parallelFetch(urls, 3);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.2：執行緒函式的多種形式2.cpp" -o callable2

// 註：最後一段三個 [worker N] 的**出現順序每次執行都不同**（三條執行緒並行，

// === 預期輸出 ===
// （closure 大小為實作定義值；本機 x86-64 / GCC 15.2）
//
// === 無捕獲 lambda ===
// Lambda 1
//
// === 按值捕獲：捕獲當下就定案 ===
// Value: 42
// （main 這邊 value 已改成 999，但執行緒印的是捕獲當下的 42）
//
// === mutable：改的是 closure 自己的複本 ===
//   closure 內 counter = 11
//   main 的 counter 仍是 10（完全沒被動過）
//
// === closure 的大小 = 捕獲項大小總和 ===
//   sizeof([](){})   = 1  <- 空類別，C++ 規定完整物件不可為 0 大小
//   sizeof([a](){})  = 4  <- 存了一個 int
//   sizeof([&a](){}) = 8  <- 存的是位址（實作定義）
//
// === 無捕獲 lambda 可轉成函式指標 ===
//   我是透過函式指標被呼叫的
//
// === 日常實務：並行下載佇列 ===
//   [worker 0] 完成 2 個 URL（index 0..1）
//   [worker 1] 完成 2 個 URL（index 2..3）
//   [worker 2] 完成 1 個 URL（index 4..4）
//   彙總：5 筆，第一筆 = 200 OK <- https://example.com/a
//
//     誰先搶到 cout 的鎖就先印）。每行內容本身是完整的（有 mutex 保護），
//     但行的先後順序不保證。其餘各段都在 join() 之後才繼續，順序是確定的。
