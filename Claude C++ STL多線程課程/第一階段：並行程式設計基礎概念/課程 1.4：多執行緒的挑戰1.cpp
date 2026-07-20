// 檔案：lesson_1_4_race_condition.cpp
//
// 【本檔是「刻意示範錯誤」的範例】
//
// ⚠️ 這是 genuine data race → 未定義行為（Undefined Behavior, UB）：
//    兩個執行緒對同一個非 atomic 的 `counter` 並行讀寫，且至少一方是寫，
//    彼此之間沒有任何 happens-before 關係。C++ 標準 [intro.races] 直接判 UB。
//
// ⚠️ 因此「實際」那一行印出什麼，標準【不保證】：
//    不可以說「一定少算」、也不可以說「一定印出某個數字」。
//    本機實測就同時觀察到「明顯少算」與「剛好等於 200000」兩種結果（見檔尾）。
//
// ✅ 正確作法（第三、四階段詳述）：
//    std::atomic<int> counter{0};  再用 counter.fetch_add(1, std::memory_order_relaxed);
//    或用 std::mutex / std::lock_guard 保護。
//
// =============================================================================
//  課程 1.4：多執行緒的挑戰1.cpp  —  data race：兩條執行緒同時 ++ 同一個變數
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    data race（資料競爭），多執行緒三大挑戰的第一個
//   語法：    int counter = 0;  ++counter;   // 在多執行緒下未加同步 → UB
//   標準版本：std::thread 是 C++11；C++11 才首次定義「記憶體模型」與 data race
//             （C++98/03 的標準裡根本沒有執行緒的概念）
//   標頭檔：  <thread>（std::thread）、<iostream>
//   複雜度：  每條執行緒 O(N) 次遞增；但本檔重點不是複雜度，是「正確性」
//
// 【詳細解釋 Explanation】
//
// 【1. 這個檔名叫 race_condition，但它示範的其實是 data race】
//   兩個名詞常被混用，面試會考，必須分清楚（本課程規範明訂）：
//     * data race（資料競爭）：兩條以上執行緒「同時」存取同一記憶體位置，
//       至少一方是寫入，且兩者之間沒有同步（happens-before）關係。
//       → 這是 C++ 標準層級的 UB，是「語言規則」問題。
//     * race condition（競爭條件）：程式的「結果取決於執行時序」的邏輯瑕疵。
//       → 這是「邏輯設計」問題，跟語言 UB 無關。
//   兩者的關係是交叉而非包含：
//     * 有 data race（本檔）→ 通常也造成 race condition，且必定是 UB。
//     * 沒有 data race 也可能有 race condition：例如兩條執行緒都正確地用
//       mutex 保護，但一條先 `if (帳戶餘額 >= 100)`、放開鎖、再 `扣款 100`，
//       中間被另一條插隊 → 完全沒有 data race（每次存取都有鎖），
//       卻仍然扣成負數。這種 check-then-act 就是純粹的 race condition。
//   本檔屬於前者：`++counter` 完全沒有同步 → data race → UB。
//
// 【2. 為什麼 ++counter 不是一個「不可分割」的動作】
//   `++counter` 在原始碼是一個字，但它是「讀-改-寫」（read-modify-write）三個
//   步驟。本機 g++ 15.2 以 -O0 產生的組譯碼（實測，x86-64 Intel 語法）：
//       mov  eax, DWORD PTR counter[rip]   ; ① load：把 counter 讀進暫存器
//       add  eax, 1                        ; ② modify：暫存器 +1
//       mov  DWORD PTR counter[rip], eax   ; ③ store：寫回記憶體
//   兩條執行緒若在 ① 和 ③ 之間交錯，就會「兩次 +1 只長 1」，這就是典型的
//   lost update（更新遺失）。原始講義（挑戰2.cpp）畫的那張交錯時序表，
//   描述的正是這個現象。
//
// 【3. 但「lost update」只是最容易理解的後果，不是標準的保證】
//   一旦踩到 UB，標準就不再對程式的任何行為負責。把它想成「少算了一點」是
//   一種過度樂觀的心智模型 —— 那只是在 x86 這種強記憶體模型的機器、在特定
//   最佳化等級下「碰巧」呈現的樣子。換 ARM、換編譯器、換最佳化等級，
//   表現可以完全不同（見【概念補充】的 -O2 實測）。
//
// 【4. 怎麼修（本檔刻意不修，留到後續課程）】
//   * std::atomic<int> + fetch_add：本例最合適，計數器不需要互斥區。
//   * std::mutex + std::lock_guard：要保護的是「一段複合操作」時用。
//   * 每條執行緒各自累加區域變數，最後才合併一次：競爭次數從 N 降到執行緒數，
//     這是效能最好的做法（第六階段的 reduction 模式）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) -O0 與 -O2 的天差地別（本機實測，這是本檔最重要的一課）
//   同一份原始碼，只改最佳化等級，結果完全不同：
//     -O0：五次執行得到 100000 / 107669 / 124094 / 112265 / 119527（每次都不同）
//     -O2：五次執行【全部】印出 200000，看起來「完全正確」
//   為什麼？看 -O2 產生的組譯碼（實測）：
//       _Z18incrementManyTimesv:
//           add  DWORD PTR counter[rip], 100000
//           ret
//   編譯器發現迴圈只是把 counter 加 100000 次 1，於是把整個迴圈摺疊成
//   【一條指令】。競爭視窗從「10 萬次 × 三步驟」縮小到「一條指令」，
//   小到實務上幾乎撞不到，所以答案看起來對了。
//
// (B) 但 -O2 的 `add [counter], 100000` 仍然不是原子操作
//   這點極容易誤解：「一條指令」≠「原子」。x86 的記憶體 RMW 指令只有在加上
//   LOCK 前綴（`lock add`）時才保證原子性；上面那條沒有 LOCK。
//   它在微架構上仍是「讀 cache line → 加 → 寫回」，兩顆核心仍可能交錯。
//   所以 -O2 印出 200000【不是】程式正確的證明，只是競爭視窗變窄的假象。
//   → 教訓：「我開 -O2 跑了幾次答案都對」永遠不能用來證明沒有 data race。
//      std::atomic 的 fetch_add 編出來才會是帶 LOCK 前綴的版本。
//
// (C) 為什麼 volatile 不能拿來修
//   C++ 的 volatile 只承諾「編譯器不得省略或合併這個讀寫」。它【不提供】：
//     * 原子性：volatile 的 ++ 仍是 load/add/store 三步，仍會 lost update。
//     * 可見性與順序：與其他非 volatile 存取之間沒有 happens-before。
//   volatile 是給 memory-mapped I/O（硬體暫存器）用的，不是同步原語。
//   注意這是 C++ 的規則：Java 的 volatile 有 acquire/release 語意，比 C++ 強，
//   從 Java 轉過來的人特別容易在這裡摔跤。
//
// (D) 怎麼「證明」有 data race，而不是靠肉眼看數字
//   用 ThreadSanitizer，它會直接指出競爭的兩個存取點：
//       g++ -std=c++17 -fsanitize=thread -g -pthread 本檔.cpp -o race_tsan
//       ./race_tsan          # 會印出 WARNING: ThreadSanitizer: data race
//   這比「跑幾次看數字對不對」可靠得多，因為 UB 本來就可能偽裝成正確。
//
// 【注意事項 Pay Attention】
// 1. data race 是 UB，不是「數字會小一點」。任何「一定會 / 一定不會」的
//    敘述都是錯的（唯一能說的是：標準不保證）。
// 2. 答案剛好等於 200000 完全不代表程式沒問題（見 (A)(B) 實測）。
// 3. volatile 不是同步工具；請用 std::atomic 或 std::mutex。
// 4. 本例的 counter 是全域變數，兩條執行緒都直接抓 rip-relative 位址，
//    沒有任何同步；這是最單純、最容易觀察的 data race 形態。
// 5. 真正的專案請用 -fsanitize=thread 在 CI 跑，別靠人眼判讀輸出。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】data race / race condition
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. ++counter 是原子操作嗎？為什麼兩條執行緒各加 10 萬次，結果不是 20 萬？
//     答：不是。它是 load / add / store 三步驟的 read-modify-write。
//         兩條執行緒可能都讀到同一個舊值、各自 +1、再各自寫回，
//         兩次遞增只長了 1（lost update）。
//         更精確地說：這是 data race，屬於 UB，標準對結果不做任何保證。
//     追問：那我開 -O2 之後每次都印出 200000，是不是就修好了？
//         → 沒有。-O2 只是把整個迴圈摺疊成一條 `add [counter], 100000`，
//           競爭視窗變到極窄；那條指令沒有 LOCK 前綴，仍非原子。
//           換一台 ARM 機器或改變負載就可能再度出錯。
//
// 🔥 Q2. data race 和 race condition 有什麼差別？
//     答：data race 是「未同步的並行存取且至少一方是寫」，是語言層級的 UB。
//         race condition 是「結果取決於時序」的邏輯錯誤，不一定是 UB。
//         有 data race 幾乎必然導致 race condition；但反過來不成立 ——
//         全程用 mutex 保護、完全沒有 data race 的程式，
//         仍可能因為 check-then-act（先檢查餘額、放鎖、再扣款）而有 race condition。
//     追問：舉一個「沒有 data race 卻有 race condition」的例子？
//         → 上面的轉帳扣款；或 `if (!map.count(k)) map[k] = v;` 兩次分別上鎖。
//
// ⚠️ 陷阱1. 把 counter 宣告成 volatile int 就能解決，對嗎？
//     答：不對，完全沒解決。volatile 只阻止編譯器省略/合併存取，
//         不提供原子性，也不建立 happens-before。
//         volatile 的 ++ 一樣是三步驟，一樣會 lost update，一樣是 data race。
//     為什麼會錯：多數人腦中是「Java 的 volatile」。Java 的 volatile 具有
//         acquire/release 語意，能保證可見性；C++ 的 volatile 沒有這些保證，
//         它的用途是 memory-mapped I/O。兩者同名不同義。
//
// ⚠️ 陷阱2. 「這支程式的實際值一定會小於 200000」——這句話對嗎？
//     答：不對。這是 UB，標準不保證任何結果。
//         本機 -O0 實測確實都小於 20 萬，但 -O2 實測【五次全部剛好等於 200000】。
//         所以既不能說「一定小於」，也不能說「一定會出錯」。
//     為什麼會錯：把 UB 想成「一個會壞掉但可預測的行為」。
//         UB 的定義是「標準不加以定義」，包含「看起來完全正常」也是合法結果 ——
//         這正是 data race 最危險的地方：它可以在測試環境安靜通過，上線才爆。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>

int counter = 0;

void incrementManyTimes() {
    for (int i = 0; i < 100000; ++i) {
        ++counter;  // 這不是原子操作！
    }
}

int main() {
    std::thread t1(incrementManyTimes);
    std::thread t2(incrementManyTimes);
    
    t1.join();
    t2.join();
    
    std::cout << "預期: 200000" << std::endl;
    std::cout << "實際: " << counter << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread 課程\ 1.4：多執行緒的挑戰1.cpp -o race
//
// 偵測 data race（建議，比人眼判讀可靠）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread 課程\ 1.4：多執行緒的挑戰1.cpp -o race_tsan

// ⚠️ 本檔是 genuine data race → UB，輸出【每次執行都不同】，
//   → 每次都不同，且都小於 200000（lost update）。

// === 預期輸出 ===
//    下面是本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）的真實實測，不是保證值。
//
// 【以檔頭編譯指令（未指定 -O，即 -O0）實測，exit=0】
// 預期: 200000
// 實際: 129722
//
// 同一支程式連續執行五次，「實際」分別為：
//   100000 / 107669 / 124094 / 112265 / 119527
//   註：100000 這種「剛好等於一條執行緒的份」代表另一條的貢獻幾乎整個被覆蓋掉。
//
// 【改用 -O2 實測（g++ -std=c++17 -O2 -Wall -Wextra -pthread），exit=0】
// 預期: 200000
// 實際: 200000        ← 連續五次全部如此
//
//   這【不代表程式正確】。-O2 把整個迴圈摺疊成單一指令
//   `add DWORD PTR counter[rip], 100000`（實測組譯碼），
//   競爭視窗窄到幾乎撞不到；但該指令沒有 LOCK 前綴，仍非原子操作，
//   data race 與 UB 依然存在。
//
//   → 本檔的核心教訓：不能用「跑幾次答案對」來證明沒有 data race，
//     要用 ThreadSanitizer 或改用 std::atomic / std::mutex 從根本解決。
