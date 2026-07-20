// =============================================================================
//  課程 2.3：傳遞參數給執行緒3.cpp  —  std::ref:讓執行緒真的改到原變數
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : std::thread t(func, std::ref(value));
//   標準版本  : C++11(std::thread、std::ref / std::reference_wrapper)
//   標頭檔    : <thread>;std::ref 定義在 <functional>
//               (實務上 <thread> 常間接引入它,但明確 include 才是正確做法)
//   回傳型別  : std::ref(x) → std::reference_wrapper<T>
//   關鍵規則  : std::thread 會把所有參數「decay-copy」後以右值傳給函式
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼不能直接傳變數給吃參考的函式】
//     void modify(int& x);
//     int value = 1;
//     std::thread t(modify, value);      // ✗ 編譯失敗
//
// std::thread 的建構子會把每個參數複製一份存進執行緒的內部儲存,
// 執行時再把「那份副本」傳給函式 —— 而且是以右值(rvalue)的形式傳過去。
// 非 const 的左值參考 int& 不能綁定到右值,於是編譯失敗。
//
// 本機 GCC 15.2.0 的錯誤訊息把這件事說得非常直白:
//     error: static assertion failed:
//            std::thread arguments must be invocable after conversion to rvalues
// 這是 libstdc++ 特地寫的 static_assert,就是為了避免使用者看到
// 一大坨無法閱讀的模板展開錯誤。
//
// 【2. 這個設計不是刁難,而是保護】
// 假設標準允許直接傳參考,那麼:
//     void spawn() {
//         int local = 1;
//         std::thread t(modify, local);   // 假設這能編譯,且傳的是參考
//         t.detach();
//     }   // local 在這裡被銷毀,但執行緒可能還沒跑到 modify
// 執行緒會寫入一塊已經歸還的堆疊記憶體 —— 未定義行為,而且症狀通常
// 出現在完全無關的地方,極難追查。
// 標準的選擇是:預設一律複製(安全),想要共享就必須明確寫 std::ref ——
// 讓「我知道我在做什麼」這件事在程式碼上留下痕跡。
//
// 【3. std::ref 做了什麼】
// std::ref(value) 回傳一個 std::reference_wrapper<int>,它是個
// 「可以被複製的參考」——內部其實存的是指標。
// 於是:std::thread 照樣複製它(複製的是這個輕量包裝),
// 而包裝內部仍指向原本的 value。呼叫時 reference_wrapper 會隱式轉換回 int&,
// modify 就拿到了真正的原變數。
// 「複製語意」與「共享語意」就這樣同時被滿足了。
//
// 【4. 用了 std::ref 之後,責任轉移到你身上】
// 一旦寫下 std::ref,編譯器就不再保護你了。你必須自己保證兩件事:
//   (a) 生命週期:被參考的變數必須活得比執行緒久。
//       本例在同一個函式內 join(),所以 value 一定還在 —— 這是安全的。
//       若改成 detach,就是懸空參考。
//   (b) 資料競爭:若有多條執行緒同時透過 std::ref 存取同一個變數,
//       而其中至少一條會寫入,就必須用 mutex 或 atomic 保護。
//       本例只有一條執行緒在寫,而且 join() 建立了 happens-before 關係,
//       所以主執行緒之後讀到 100 是有保證的,不是碰巧。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼是 join() 讓結果「保證看得到」
//   多執行緒下,一條執行緒寫的值另一條不一定看得到(可能還在暫存器或
//   store buffer 裡)。t.join() 除了等待,還建立了同步關係:
//   執行緒中的所有操作 happens-before join() 的返回。
//   所以 join 之後讀 value 一定是 100 —— 這是記憶體模型的保證,
//   不是「反正它跑完了應該就寫進去了」的直覺。
//
// (B) decay-copy 是什麼
//   標準規定 thread 對每個參數做 decay-copy:陣列退化成指標、
//   函式退化成函式指標、去掉 cv 與參考修飾,然後複製。
//   這解釋了為什麼傳 char[] 給吃 const std::string& 的函式時,
//   存進去的是 char*(而不是 std::string)—— 那是本課第 5 個範例檔的坑。
//
// (C) std::ref 與 std::cref
//   std::cref(x) 產生 reference_wrapper<const T>,用於「共享但唯讀」。
//   多條執行緒同時唯讀同一份資料是安全的,不需要鎖 ——
//   用 cref 能讓這個意圖在型別上就表達清楚。
//
// 【注意事項 Pay Attention】
// 1. 忘記 std::ref 是編譯錯誤(有明確的 static_assert),不是執行期的錯 ——
//    這點其實很幸運,比默默複製好得多。
// 2. 用了 std::ref 就必須自己保證被參考物件的生命週期,detach 時尤其危險。
// 3. std::ref 不提供任何同步;多條執行緒同時讀寫仍需 mutex 或 atomic。
// 4. std::ref 定義在 <functional>,請明確 include。
// 5. 只讀不寫的共享請用 std::cref,讓意圖更清楚。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::ref 與執行緒參數傳遞
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. void modify(int&) 這種函式,為什麼 std::thread t(modify, value);
//        編譯不過?
//     答：std::thread 會把參數複製一份存進執行緒內部,執行時以「右值」
//         傳給函式。非 const 的左值參考無法綁定右值,所以編譯失敗。
//         本機 GCC 的錯誤訊息就是
//         "std::thread arguments must be invocable after conversion to rvalues"。
//         正確寫法是 std::thread t(modify, std::ref(value));
//     追問：標準為什麼要這樣設計,直接傳參考不是更方便?
//         → 因為執行緒的生命週期和呼叫端脫鉤。若預設傳參考,
//           呼叫端的區域變數一銷毀,執行緒就在寫死掉的記憶體。
//           預設複製是安全的,而要共享必須明確寫 std::ref,
//           讓風險決策在程式碼上留下痕跡。
//
// 🔥 Q2. std::ref 為什麼能突破「參數會被複製」這個限制?
//     答：std::ref 產生一個 reference_wrapper,它是「可以被複製的參考」,
//         內部存的是指標。thread 複製的是這個輕量包裝,
//         而包裝仍指向原變數;呼叫時它會隱式轉換回 T&。
//         這樣複製語意與共享語意就同時成立了。
//     追問：用了 std::ref 之後還需要注意什麼?
//         → 兩件事:被參考的變數必須活得比執行緒久(detach 時特別危險);
//           以及 std::ref 不提供任何同步,多執行緒讀寫仍要自己加鎖。
//
// ⚠️ 陷阱. 「t.join() 之後讀到 value 是 100,那是因為執行緒已經跑完了,
//         值自然就寫進去了 —— 跟同步沒關係。」哪裡錯了?
//     答：「跑完了」和「我看得到」在多執行緒下是兩件事。一條執行緒寫的值
//         可能還留在該核心的 store buffer 或暫存器裡,別的核心不保證看得到。
//         你之所以一定能讀到 100,是因為 join() 本身建立了同步關係:
//         標準保證執行緒中的所有操作 happens-before join() 的返回。
//     為什麼會錯：用單執行緒的直覺想像記憶體 —— 以為寫入就是立刻對全世界生效。
//         實際上現代 CPU 有多層快取與寫入緩衝,編譯器也會重排指令。
//         「看得到別人寫的值」永遠需要某種同步機制建立 happens-before,
//         join()、mutex、atomic 都是提供這種保證的工具。
// ═══════════════════════════════════════════════════════════════════════════

#include <functional>   // std::ref / std::cref
#include <iostream>
#include <string>
#include <thread>
#include <vector>

void modify(int& x) {
    x = 100;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】平行統計:每條執行緒把結果寫回呼叫端的變數
//   情境: 一份訂單清單要同時算出「總金額」與「最大單筆金額」。
//         兩件事互相獨立,適合各開一條執行緒;結果要寫回主執行緒的變數,
//         所以必須用 std::ref 共享。
//   為什麼用本主題: 這是 std::ref 最正當的用法 ——
//         兩條執行緒寫的是「不同」的變數,沒有資料競爭;
//         主執行緒在 join() 之後才讀,由 join 建立 happens-before,
//         結果保證正確。這也示範了不需要 mutex 的正確平行寫入設計。
// -----------------------------------------------------------------------------
struct Order {
    std::string id;
    long        amount;
};

void computeTotal(const std::vector<Order>& orders, long& out) {
    long sum = 0;
    for (const Order& o : orders) sum += o.amount;
    out = sum;
}

void computeMax(const std::vector<Order>& orders, const Order*& out) {
    const Order* best = nullptr;
    for (const Order& o : orders) {
        if (best == nullptr || o.amount > best->amount) best = &o;
    }
    out = best;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   本檔的主題是「參數如何被傳進執行緒」,屬於 std::thread 的 API 語意。
//   LeetCode 的並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   的評測框架會自己建立執行緒並呼叫你的成員函式,你根本沒有機會決定
//   參數怎麼傳,考點全在同步原語。本課第 7 個範例檔(總整理)會示範
//   真正對應得上的 1116 Print Zero Even Odd,此處從缺以免失焦。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:用 std::ref 傳遞引用 ===" << std::endl;
    {
        int value = 1;
        std::cout << "  呼叫前 value = " << value << std::endl;

        std::thread t(modify, std::ref(value));   // 傳遞引用
        t.join();                                 // join 建立 happens-before

        std::cout << "  呼叫後 value = " << value << "  (現在是 100)" << std::endl;
    }

    std::cout << "\n=== 對照:不用 std::ref 會怎樣 ===" << std::endl;
    std::cout << "  std::thread t(modify, value);   // ✗ 編譯失敗" << std::endl;
    std::cout << "  本機 GCC 15.2.0 實測錯誤訊息:" << std::endl;
    std::cout << "    error: static assertion failed: std::thread arguments"
                 " must be invocable after conversion to rvalues" << std::endl;
    std::cout << "  (該行未寫進本檔,否則整支程式無法編譯)" << std::endl;

    std::cout << "\n=== 實務:兩條執行緒各自寫回不同的結果變數 ===" << std::endl;
    {
        std::vector<Order> orders = {
            {"A-001", 1200}, {"A-002", 350}, {"A-003", 9800}, {"A-004", 640},
        };

        long         total = 0;
        const Order* biggest = nullptr;

        // 兩條執行緒「唯讀」共享 orders(用 cref 表達意圖),
        // 各自「獨佔寫入」不同的輸出變數 → 沒有資料競爭,不需要 mutex
        std::thread t1(computeTotal, std::cref(orders), std::ref(total));
        std::thread t2(computeMax,   std::cref(orders), std::ref(biggest));
        t1.join();
        t2.join();

        std::cout << "  訂單總金額 = " << total << std::endl;
        std::cout << "  最大單筆   = " << biggest->id << " (" << biggest->amount
                  << ")" << std::endl;
        std::cout << "  ↑ 兩條執行緒寫不同變數 + join 後才讀 → 免鎖也正確"
                  << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.3：傳遞參數給執行緒3.cpp" -o pass_by_ref

// 注意:以下為某一次實際執行的結果。
//   本檔的輸出「每次執行都相同」—— 因為所有結果都是在 join() 之後才讀取的,
//   而 join 建立了 happens-before 關係。這正是本檔想示範的重點:
//   多執行緒程式的輸出並非注定混亂,只要同步做對了,結果就是確定的。

// === 預期輸出 ===
// === 原始示範:用 std::ref 傳遞引用 ===
//   呼叫前 value = 1
//   呼叫後 value = 100  (現在是 100)
//
// === 對照:不用 std::ref 會怎樣 ===
//   std::thread t(modify, value);   // ✗ 編譯失敗
//   本機 GCC 15.2.0 實測錯誤訊息:
//     error: static assertion failed: std::thread arguments must be invocable after conversion to rvalues
//   (該行未寫進本檔,否則整支程式無法編譯)
//
// === 實務:兩條執行緒各自寫回不同的結果變數 ===
//   訂單總金額 = 11990
//   最大單筆   = A-003 (9800)
//   ↑ 兩條執行緒寫不同變數 + join 後才讀 → 免鎖也正確
