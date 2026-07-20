// =============================================================================
//  第八課 13  —  mutable Lambda：修改「值捕獲的副本」
// =============================================================================
//
// 【主題資訊 Information】
//   語法    : [capture](params) mutable -> ret { body }
//   標準版本: C++11
//   標頭檔  : 無（語言核心特性）
//   語意    : mutable 的唯一作用是「移除 closure 的 operator() 上的 const」
//   複雜度  : 呼叫本身 O(1)；狀態存活於 closure 物件，跨呼叫累積
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「沒有 mutable 就不能改值捕獲的變數」】
//   把 lambda 展開成編譯器實際生成的 class 就一目了然：
//       auto f = [counter]() { return ++counter; };
//   等價於
//       class __lambda {
//           int counter;
//       public:
//           int operator()() const { return ++counter; }   // ← const 成員函式！
//       };
//   在 const 成員函式裡，所有非 mutable 成員都被視為 const，
//   所以 ++counter 就是「修改 const 物件」，編譯直接失敗。
//   加上 mutable 之後：
//       int operator()() { return ++counter; }             // ← const 沒了
//   於是合法。可見 mutable 修飾的不是變數，是那個 operator()。
//
// 【2. 為什麼外部 counter 不受影響】
//   值捕獲的意思就是「在建立 closure 的當下，把值拷貝一份成為成員」。
//   之後 lambda 動的永遠是自己那份成員，外部變數與它已經沒有任何關係。
//   本檔最後印出「外部 counter: 0」正是這件事的證明。
//
// 【3. 狀態會累積，不會重置】
//   很多人誤以為每次呼叫 lambda 都會「重新拷貝一次外部變數」。
//   不會。拷貝只發生在建立 closure 的那一刻，之後 closure 就是一個帶狀態
//   的物件。所以連續呼叫 increment() 會拿到 1、2、3——這正是 lambda
//   能取代「帶狀態的 functor」的原因。
//
// 【概念補充 Concept Deep Dive】
//   mutable lambda 有一個常被忽略的副作用：因為 operator() 不再是 const，
//   const 化的 closure 物件就不能呼叫它。
//       const auto f = [x]() mutable { return ++x; };
//       f();   // 編譯錯誤：無法在 const 物件上呼叫非 const 成員函式
//   同理，mutable lambda 傳給某些要求「const 可呼叫」的介面時會失敗。
//   另外注意：如果 closure 被複製，複製出來的那份會帶著「當下的狀態」，
//   兩者之後各自獨立累積——這與一般物件的複製語意完全一致。
//
// 【注意事項 Pay Attention】
// 1. mutable 不是「讓外部變數可改」。要改外部變數請用參考捕獲 [&counter]，
//    但那就要自行確保生命週期。
// 2. mutable lambda 帶狀態 ⇒ 它不是純函式。傳給 std::count_if 這類
//    「未規定呼叫順序與次數」的演算法時，結果可能不如預期。
// 3. 演算法收的是謂詞的「副本」，mutable lambda 在演算法內累積的狀態
//    不會寫回你手上那個 closure；要拿回狀態得用 std::for_each 的回傳值
//    或 std::ref。
// 4. 有 mutable 的 lambda 不能被 const closure 物件呼叫（見上方概念補充）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】mutable Lambda
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. mutable 這個關鍵字在 lambda 上到底做了什麼？
//     答：它把 closure 的 operator() 從 const 改成非 const。因為 lambda 的
//         值捕獲會變成成員變數，而 const 成員函式不能修改成員，所以沒有
//         mutable 就改不了副本。mutable 完全沒有碰到外部變數。
//     追問：加了 mutable 後外部變數會被改嗎？→ 不會，永遠不會。
//         值捕獲拷貝只發生一次（建立 closure 時），之後兩者毫無關聯。
//
// 🔥 Q2. 連續呼叫三次 mutable lambda，為什麼印出 1、2、3 而不是 1、1、1？
//     答：因為捕獲的副本是 closure 的「成員變數」，生命週期跟著 closure 物件，
//         不是每次呼叫都重新拷貝。closure 就是一個帶狀態的函數物件，
//         狀態自然跨呼叫累積。
//     追問：那把這個 lambda 複製一份，兩份會共用狀態嗎？→ 不會，
//         複製 closure 就是複製它的成員，之後各自獨立累積。
//
// ⚠️ 陷阱. 「加 mutable 就等於參考捕獲，反正都能改」——錯在哪？
//     答：兩者改的是完全不同的東西。mutable + 值捕獲改的是 closure 內部
//         那份副本，外部變數紋風不動；參考捕獲 [&] 改的才是外部變數本身，
//         但同時也背上了懸空參考的風險。
//     為什麼會錯：多數人把 mutable 讀成「可變的」，就以為它解除了某種
//         「不能修改」的限制、讓 lambda 取得寫入權限。實際上它只是
//         拿掉 operator() 的 const，作用範圍完全侷限在 closure 自己身上。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】從缺。
//   理由：mutable lambda 的價值在於「攜帶跨呼叫的狀態」，而 LeetCode 解法
//   追求的是純函式式的謂詞（帶狀態的謂詞在未規定呼叫順序的演算法中反而
//   危險）。本清單中的題目沒有一題的核心是「有狀態的 closure」，
//   硬套會傳達錯誤示範，故從缺，改以實務範例呈現它真正的用途。

#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】產生帶序號的追蹤 ID（trace id generator）
//   場景：微服務要為每個進來的請求配一個唯一的追蹤編號，
//         格式如 "svc-api-00001"、"svc-api-00002"。
//   為什麼用 mutable lambda：序號必須「跨呼叫累積」，這正是有狀態 closure
//     的典型用途。相較於全域計數器，closure 把狀態封裝在自己內部——
//     可以同時存在多個 generator（不同服務前綴各自計數），互不干擾，
//     也不會有全域變數那種「誰都能改」的問題。
//   注意：這個 generator 不是執行緒安全的；多執行緒共用需另加同步機制。
// -----------------------------------------------------------------------------
auto makeTraceIdGenerator(const std::string& prefix) {
    int seq = 0;
    // 值捕獲 prefix 與 seq（seq 是區域變數，函式回傳後就消失，
    // 所以「絕不能」用參考捕獲——那會是懸空參考）。
    return [prefix, seq]() mutable {
        ++seq;
        std::string num = std::to_string(seq);
        // 補零到 5 位。注意 num.size() 是無號型別，若直接寫 5 - num.size()
        // 而 num 超過 5 位就會 unsigned underflow 成天文數字 → 立刻爆記憶體。
        // 這是 size_t 運算最常見的坑，務必先比大小再相減。
        std::string pad = (num.size() < 5) ? std::string(5 - num.size(), '0') : "";
        return prefix + "-" + pad + num;
    };
}

int main() {
    std::cout << "=== mutable lambda 累積狀態 ===" << std::endl;
    int counter = 0;

    // 錯誤：預設情況下不能修改值捕獲的變數
    // auto increment = [counter]() { return ++counter; };  // 編譯錯誤
    
    // 使用 mutable
    auto increment = [counter]() mutable {
        return ++counter;  // 修改的是 Lambda 內部的副本
    };
    
    std::cout << "increment(): " << increment() << std::endl;  // 1
    std::cout << "increment(): " << increment() << std::endl;  // 2
    std::cout << "increment(): " << increment() << std::endl;  // 3
    std::cout << "外部 counter: " << counter << std::endl;     // 0（未被修改）

    // closure 被複製後，兩份狀態各自獨立累積
    std::cout << "\n=== 複製 closure：狀態各自獨立 ===" << std::endl;
    auto copy_of_increment = increment;   // 複製時帶著當下的狀態（3）
    std::cout << "原本的 increment(): " << increment() << std::endl;        // 4
    std::cout << "複製品     copy(): " << copy_of_increment() << std::endl; // 4
    std::cout << "原本的 increment(): " << increment() << std::endl;        // 5

    std::cout << "\n=== 日常實務: 追蹤 ID 產生器 ===" << std::endl;
    auto api_id = makeTraceIdGenerator("svc-api");
    auto db_id  = makeTraceIdGenerator("svc-db");
    // 兩個 generator 各自持有自己的序號，互不干擾
    std::cout << api_id() << std::endl;
    std::cout << api_id() << std::endl;
    std::cout << db_id()  << std::endl;
    std::cout << api_id() << std::endl;
    std::cout << db_id()  << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探13.cpp -o mutable_lambda

// === 預期輸出 ===
// === mutable lambda 累積狀態 ===
// increment(): 1
// increment(): 2
// increment(): 3
// 外部 counter: 0
//
// === 複製 closure：狀態各自獨立 ===
// 原本的 increment(): 4
// 複製品     copy(): 4
// 原本的 increment(): 5
//
// === 日常實務: 追蹤 ID 產生器 ===
// svc-api-00001
// svc-api-00002
// svc-db-00001
// svc-api-00003
// svc-db-00002
