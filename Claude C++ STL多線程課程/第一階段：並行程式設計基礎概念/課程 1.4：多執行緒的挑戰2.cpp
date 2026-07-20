/*
# 第一階段：並行程式設計基礎概念

## 課程 1.4：多執行緒的挑戰

---

### 引言

多執行緒程式設計能帶來效能提升，但也引入了單執行緒程式不會遇到的問題。本課介紹三個最常見的挑戰：**競爭條件（Race Condition）**、**死結（Deadlock）**、**飢餓（Starvation）**。

---

### 一、競爭條件（Race Condition）

當多個執行緒同時存取共享資料，且至少一個執行緒進行寫入時，結果取決於執行緒的執行順序，這就是競爭條件。

#### 範例：競爭條件的發生

```cpp
// 檔案：lesson_1_4_race_condition.cpp

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
```

#### 為什麼會出錯？

`++counter` 看似一個操作，實際上是三個步驟：

```
1. 讀取 counter 的值到暫存器
2. 暫存器的值 +1
3. 將結果寫回 counter
```

兩個執行緒可能交錯執行：

```
時間   執行緒1              執行緒2           counter
────────────────────────────────────────────────────
 1    讀取 counter (0)                          0
 2                         讀取 counter (0)     0
 3    加 1 (得到 1)                             0
 4                         加 1 (得到 1)        0
 5    寫回 (1)                                  1
 6                         寫回 (1)             1
────────────────────────────────────────────────────
結果：兩次 +1，但 counter 只增加了 1！
```

---

### 二、死結（Deadlock）

當兩個或多個執行緒互相等待對方釋放資源，導致所有執行緒都無法繼續執行。

#### 範例：死結的發生

```cpp
// 檔案：lesson_1_4_deadlock.cpp

#include <iostream>
#include <thread>
#include <mutex>

std::mutex mutexA;
std::mutex mutexB;

void thread1() {
    std::cout << "執行緒1: 嘗試獲取 mutexA..." << std::endl;
    mutexA.lock();
    std::cout << "執行緒1: 已獲取 mutexA" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "執行緒1: 嘗試獲取 mutexB..." << std::endl;
    mutexB.lock();  // 等待執行緒2釋放 mutexB
    std::cout << "執行緒1: 已獲取 mutexB" << std::endl;
    
    mutexB.unlock();
    mutexA.unlock();
}

void thread2() {
    std::cout << "執行緒2: 嘗試獲取 mutexB..." << std::endl;
    mutexB.lock();
    std::cout << "執行緒2: 已獲取 mutexB" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "執行緒2: 嘗試獲取 mutexA..." << std::endl;
    mutexA.lock();  // 等待執行緒1釋放 mutexA
    std::cout << "執行緒2: 已獲取 mutexA" << std::endl;
    
    mutexA.unlock();
    mutexB.unlock();
}

int main() {
    std::thread t1(thread1);
    std::thread t2(thread2);
    
    t1.join();
    t2.join();
    
    std::cout << "完成（這行永遠不會執行）" << std::endl;
    return 0;
}
```

#### 死結示意圖

```
┌──────────────┐         ┌──────────────┐
│   執行緒 1   │         │   執行緒 2   │
└──────┬───────┘         └───────┬──────┘
       │                         │
       │ 持有 mutexA             │ 持有 mutexB
       │                         │
       │ 等待 mutexB ──────────► │
       │ ◄────────────── 等待 mutexA
       │                         │
       ▼                         ▼
    [阻塞]                    [阻塞]
    
    互相等待，永遠無法繼續！
```

---

### 三、飢餓（Starvation）

某個執行緒長時間無法獲得所需資源，導致無法執行。

#### 常見原因

```
┌─────────────────────────────────────────┐
│            飢餓的常見原因                │
├─────────────────────────────────────────┤
│                                         │
│  1. 優先權不公平                         │
│     高優先權執行緒總是搶先執行            │
│                                         │
│  2. 鎖的獲取不公平                       │
│     某些執行緒總是比其他執行緒先獲得鎖    │
│                                         │
│  3. 資源分配策略問題                     │
│     某些執行緒持有資源時間過長            │
│                                         │
└─────────────────────────────────────────┘
```

#### 簡單示意

```cpp
// 概念示意（非完整程式碼）

// 執行緒 A：非常活躍，頻繁獲取鎖
while (true) {
    lock();
    // 快速完成工作
    unlock();
    // 幾乎立即再次嘗試獲取鎖
}

// 執行緒 B：很少有機會獲取鎖
while (true) {
    lock();    // 總是等待，因為 A 太快了
    // 工作
    unlock();
}
```

---

### 四、三大問題對照表

| 問題 | 原因 | 症狀 | 解決方向 |
|------|------|------|----------|
| 競爭條件 | 非同步存取共享資料 | 結果不可預測 | 互斥鎖、原子操作 |
| 死結 | 循環等待資源 | 程式卡住不動 | 鎖順序、超時機制 |
| 飢餓 | 資源分配不公平 | 某執行緒無法執行 | 公平鎖、優先權調整 |

---

### 五、死結的四個必要條件（Coffman Conditions）

死結發生必須同時滿足以下四個條件：

```
1. 互斥 (Mutual Exclusion)
   資源一次只能被一個執行緒使用

2. 持有並等待 (Hold and Wait)
   執行緒持有資源的同時等待其他資源

3. 不可搶占 (No Preemption)
   已分配的資源不能被強制取走

4. 循環等待 (Circular Wait)
   存在執行緒的循環等待鏈
```

**打破任一條件即可避免死結。**

---

### 六、本課重點回顧

1. **競爭條件**：多執行緒同時存取共享資料導致結果不可預測
2. **死結**：執行緒互相等待對方的資源，導致全部卡住
3. **飢餓**：某執行緒長期無法獲得資源
4. 死結需要四個條件同時成立才會發生
5. 這些問題都有對應的解決方案（後續課程會詳細介紹）

---

### 下一課預告

在 **課程 1.5：C++ 多執行緒發展史** 中，我們將了解：
- C++11 之前如何處理多執行緒
- C++11 標準執行緒函式庫的誕生
- C++14/17/20 的持續演進

---

準備好繼續嗎？
*/

// 檔案：lesson_1_4_deadlock.cpp
//
// 【本檔是「刻意示範死結」的範例，開啟示範後程式極可能永遠停住】
//
// ⚠️ 這【不是】未定義行為，每一次 lock() 都是合法的；
//    問題出在兩個執行緒以【相反順序】取鎖，形成循環等待（AB-BA 死結）。
//
// ⚠️ 但它也【不是「保證」死結】：
//    若 thread1 剛好在 thread2 開始前就整段跑完，兩者不會交錯，
//    程式就會正常印出最後一行並結束。
//    中間的 sleep_for(100ms) 是【刻意加大】交錯機率的手段，
//    讓死結幾乎每次都發生，但仍屬機率問題，而非標準保證。
//    → 因此原版「完成（這行永遠不會執行）」的說法並不精確，已改寫。
//
//    實測（g++ 15.2、Ubuntu 26.04）：連續 3 次執行皆死結（exit=124）。
//
// 為了不讓整份課程的批次執行卡死，這個示範預設【關閉】。
// 要親眼觀察，請自行加上 -DDEMONSTRATE_UB：
//    g++ -std=c++17 -pthread -DDEMONSTRATE_UB -o deadlock '課程 1.4：多執行緒的挑戰2.cpp'
//    timeout 5 ./deadlock ; echo "exit=$?"   # 預期 exit=124（逾時）
//
// ✅ 解法方向（第五階段會詳細示範）：統一取鎖順序，
//    或改用 std::scoped_lock 一次取得多把鎖。

// =============================================================================
//  課程 1.4：多執行緒的挑戰2.cpp  —  AB-BA 死結：兩把鎖、相反順序、循環等待
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    deadlock（死結），多執行緒三大挑戰的第二個
//   語法：    mutexA.lock(); mutexB.lock();   // 執行緒1
//             mutexB.lock(); mutexA.lock();   // 執行緒2 → 順序相反 = 危險
//   標準版本：std::mutex / std::lock_guard / std::lock 是 C++11
//             std::scoped_lock（可變參數、一次鎖多把）是 C++17
//             std::unique_lock（可延遲、可轉移）是 C++11
//   標頭檔：  <mutex>（mutex 與各種 lock）、<thread>、<chrono>
//   複雜度：  死結不是效能問題而是「活性（liveness）」問題 ——
//             程式不會變慢，是直接【停住不動】
//
// 【詳細解釋 Explanation】
//
// 【1. 這支程式在做什麼】
//   thread1 先鎖 A 再鎖 B；thread2 先鎖 B 再鎖 A。
//   若兩條執行緒都成功拿到「自己的第一把」，接著都要去拿「對方手上那把」，
//   就形成循環等待：
//       thread1 持有 A，等待 B  ──┐
//                                 │  互相等待，誰都不會放手
//       thread2 持有 B，等待 A  ──┘
//   兩條執行緒都在 lock() 裡永久阻塞，main 的兩個 join() 也就永遠回不來。
//
// 【2. 這【不是】未定義行為，也【不是】保證會發生 —— 兩個都要講清楚】
//   * 不是 UB：每一次 lock() 都是完全合法的呼叫，沒有任何未定義行為。
//     死結是「程式邏輯把自己鎖死了」，屬於活性問題，
//     標準對它的行為有明確定義（就是永遠阻塞在 lock()）。
//     → 這一點和 1.4-1 的 data race 完全不同，別混為一談。
//   * 但也不是保證：如果排程剛好讓 thread1 從頭到尾整段跑完（拿 A、拿 B、
//     放 B、放 A）之後 thread2 才開始，兩者根本沒有交錯，程式就會正常結束。
//     程式中的 sleep_for(100ms) 是【刻意用來加大交錯機率】的裝置：
//     它讓「已持有第一把鎖」的狀態維持 100 毫秒，幾乎保證另一條執行緒
//     在這段時間內也拿到自己的第一把鎖，於是死結幾乎必然成立。
//     本機實測連續三次都死結（exit=124），但這是「機率極高」，不是「標準保證」。
//   → 面試時把 AB-BA 死結說成「一定會死結」是會被追問的；
//     正確說法是「形成循環等待的必要條件已具備，實際是否觸發取決於時序」。
//
// 【3. 死結的四個必要條件（Coffman conditions，1971）】
//   四個條件必須【同時】成立才可能發生死結，缺一不可：
//     ① 互斥（Mutual Exclusion）    ：資源一次只能被一條執行緒持有。
//                                      std::mutex 天生就是互斥的。
//     ② 持有並等待（Hold and Wait） ：已經持有資源，還要再去要別的資源。
//                                      本例：拿著 A 再去要 B。
//     ③ 不可搶佔（No Preemption）   ：拿到的鎖不能被外力強制收回。
//                                      std::mutex 沒有任何人能替你 unlock。
//     ④ 循環等待（Circular Wait）   ：等待關係形成一個環。
//                                      本例：1→B→2→A→1，成環。
//   → 破解死結 = 打破其中【任何一個】條件。
//
// 【4. 四種實務破解法（對應要打破哪個條件）】
//   (a) 統一鎖順序（打破 ④ 循環等待）—— 最常用、成本最低
//       全專案約定「永遠先鎖 A 再鎖 B」（例如依位址、依 ID 由小到大）。
//       沒有環，就不可能死結。缺點是要靠紀律與 code review 維持。
//   (b) std::scoped_lock / std::lock（打破 ②③ 的組合）—— C++ 標準解
//       std::scoped_lock lk(mutexA, mutexB);   // C++17，一行搞定
//       它內部用的是「全有或全無」的演算法：試著鎖住所有鎖，
//       只要有任一把拿不到，就【把已拿到的全部放掉】再重試。
//       因為不會「拿著一把死等另一把」，就沒有 hold-and-wait，也不會成環。
//       注意：標準只保證「不會因為這組鎖而死結」，未規定具體演算法。
//       C++11 沒有 scoped_lock，要寫成：
//           std::unique_lock<std::mutex> la(mutexA, std::defer_lock);
//           std::unique_lock<std::mutex> lb(mutexB, std::defer_lock);
//           std::lock(la, lb);                 // C++11 等效寫法
//   (c) try_lock + backoff（打破 ② 持有並等待）
//       用 try_lock() 而非 lock()；拿不到第二把就放掉第一把、隨機等一下再試。
//       ⚠️ 注意這會引入 livelock（活鎖）風險：兩條執行緒不斷「同時搶、同時退」
//          誰也前進不了。程式沒卡死（CPU 100% 忙碌）但也沒進展。
//          解法是加上【隨機化】的退避時間，打破對稱性。
//       這也正是「死結」與「活鎖」的差別：死結是靜止的，活鎖是忙碌的。
//   (d) 逾時（timed_mutex）—— 偵測而非預防
//       try_lock_for() / try_lock_until()（C++11 的 std::timed_mutex），
//       逾時就放棄整筆交易並回報錯誤。資料庫的死結處理就走這條路
//       （偵測到死結 → 選一個 victim transaction rollback）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼作業系統不幫我們自動偵測死結？
//   理論上 OS 可以維護「等待圖（wait-for graph）」並找環，但：
//     * 成本高：每次 lock/unlock 都要更新圖並偵測環路，
//       而 mutex 的無競爭路徑（uncontended path）本來快到只是一次原子操作。
//     * 語意難：偵測到之後要怎麼辦？強制把某條執行緒的鎖收走，會讓它保護的
//       不變式（invariant）處於半修改狀態，比死結更危險。
//     * 因此 pthread / Linux 選擇「不偵測、不介入」。
//   Linux 的 std::mutex 底層是 futex（fast userspace mutex）：
//   無競爭時純使用者空間的原子操作、完全不進核心；
//   真的要等待時才 futex_wait 讓執行緒進入睡眠。所以死結時 CPU 使用率是 0%，
//   兩條執行緒安安靜靜地睡著，永遠不會被喚醒 —— 這是辨識死結的重要特徵。
//
// (B) 現場怎麼確認「是不是死結」（實務除錯技巧）
//   * 特徵：程式沒有結束、CPU 幾乎 0%、沒有任何輸出進展。
//     （若 CPU 100% 但沒進展，那是 livelock 或忙等，不是死結。）
//   * gdb：  gdb -p <pid>  然後 `thread apply all bt`
//            會看到兩條執行緒都停在 __lll_lock_wait / pthread_mutex_lock。
//   * 自動化：g++ -fsanitize=thread 的 ThreadSanitizer 具備
//            lock-order-inversion 偵測，能在【還沒真的死結】時就先警告
//            「這兩把鎖曾以相反順序被取得」，比等它當機再查有效得多。
//
// (C) 為什麼「鎖排序」在真實系統要靠位址或 ID，而不是靠人腦記憶
//   轉帳 transfer(a, b) 與 transfer(b, a) 同時發生就是教科書級的 AB-BA。
//   實務上不可能替每個帳戶物件手動排順序，所以用一個「全序」的鍵：
//       if (&a < &b) { lock(a); lock(b); } else { lock(b); lock(a); }
//   用物件位址或帳號 ID 當排序鍵，任何兩把鎖的取得順序就有了一致的全序，
//   循環等待在結構上不可能成立。（見下方【日常實務範例】。）
//   更大型的系統會定義「鎖階層（lock hierarchy）」：替每把鎖標一個層級號碼，
//   規定只能由高層往低層取，並在 debug build 用執行緒區域變數驗證。
//
// (D) 這支程式還藏著一個「沒有 data race 的 race condition」
//   本機實測某次執行印出這樣的畫面：
//       執行緒1: 已獲取 mutexA執行緒2: 嘗試獲取 mutexB...
//       執行緒2: 已獲取 mutexB
//   兩條訊息被「絞」在一起。這是因為 `std::cout << x << std::endl;`
//   是【多次】獨立的 << 呼叫，兩條執行緒的呼叫可以交錯。
//   關鍵在於：C++11 起標準保證同步的標準串流物件（std::cout）並行使用
//   【不會產生 data race】（不是 UB），但它【不保證】多次 << 之間不被插隊。
//   → 所以這是一個「完全沒有 data race，卻結果取決於時序」的 race condition，
//     正好是 1.4-1 檔頭那組定義的最佳實例：兩者真的是不同的概念。
//     要修就得自己加鎖，或先組好 std::ostringstream 再一次輸出。
//
// 【注意事項 Pay Attention】
// 1. 死結不是 UB。不要說「死結是未定義行為」——它的行為有明確定義：永遠阻塞。
// 2. 死結不保證發生。AB-BA 只是「具備了必要條件」，是否觸發取決於時序；
//    sleep_for 只是提高機率的裝置，不是保證。
// 3. 別把「不保證發生」當成「不嚴重」：正因為它只在特定時序下出現，
//    才會測試環境全綠、上線後在高負載下才爆，而且極難重現。
// 4. std::scoped_lock 只解決「同時取得多把鎖」的死結。
//    如果是「A 函式鎖 A 後呼叫 B 函式再鎖 B」這種跨函式的巢狀取鎖，
//    仍需靠鎖排序或鎖階層。
// 5. 對同一個非遞迴的 std::mutex 在同一條執行緒重複 lock() 是 UB，
//    那是另一種錯誤（見後續課程），不要和本檔的循環等待混淆。
// 6. try_lock 迴圈務必加隨機退避，否則死結換成活鎖，問題只是換了個樣子。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deadlock / starvation
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是死結？發生死結需要哪四個條件？
//     答：兩條以上執行緒各自持有對方需要的資源並互相等待，全體無法前進。
//         四個必要條件（Coffman）：互斥、持有並等待、不可搶佔、循環等待，
//         必須同時成立；打破任一個即可避免。
//         最常用的是打破「循環等待」——統一全域鎖順序。
//     追問：那 std::scoped_lock 是打破哪一個條件？
//         → 打破「持有並等待」。它採全有或全無策略：任一把拿不到就把
//           已取得的全部釋放再重試，因此不會有人「拿著一把死等另一把」。
//
// 🔥 Q2. 兩個帳戶互相轉帳造成 AB-BA 死結，你會怎麼修？
//     答：(1) 依帳戶位址或 ID 建立全序，永遠由小到大取鎖；
//         (2) 或直接 std::scoped_lock lk(m1, m2);（C++17）一次取兩把；
//         (3) C++11 環境用 std::lock(l1, l2) 搭配 defer_lock 的 unique_lock。
//         另外要處理「自己轉給自己」的退化情況，否則會對同一把非遞迴 mutex
//         重複 lock，那是 UB。
//     追問：如果改用 try_lock 迴圈重試會有什麼新問題？
//         → livelock（活鎖）：雙方不斷同時搶、同時退讓，CPU 忙碌但無進展。
//           必須加入隨機化退避打破對稱性。
//
// 🔥 Q3. 死結、活鎖、飢餓三者差在哪？
//     答：死結 = 互相等待、全體靜止不動，CPU 接近 0%。
//         活鎖 = 一直在動、一直讓步，CPU 很忙但沒有人前進。
//         飢餓 = 系統整體有進展，但【某一條】執行緒長期搶不到資源。
//         死結是「全都不動」，飢餓是「有人動、有人永遠輪不到」。
//     追問：飢餓怎麼解？
//         → 公平鎖（FIFO 排隊）、優先權老化（aging，等越久優先權越高）、
//           或改用票號制（ticket lock）保證先到先服務。
//           注意 std::mutex【不保證公平】，標準未規定喚醒順序。
//
// ⚠️ 陷阱1. 「這支程式一定會死結，最後一行永遠不會被印出來」——對嗎？
//     答：不對。這是【時序相依】的死結，不是標準保證。
//         若排程讓 thread1 整段先跑完再輪到 thread2，程式會正常結束並印出 "完成"。
//         sleep_for(100ms) 只是把交錯機率拉到極高（本機實測三次皆死結），
//         但「機率極高」不等於「保證」。
//     為什麼會錯：把「我每次跑都死結」當成「標準保證死結」。
//         並行程式的正確描述要區分「必要條件已具備」與「實際觸發」；
//         這也正是死結難抓的原因 —— 反過來說，「我跑幾次都沒死結」
//         同樣不能證明程式安全。
//
// ⚠️ 陷阱2. 把兩個 lock() 都換成 std::lock_guard 就不會死結了，對嗎？
//     答：不對。lock_guard 解決的是「忘記 unlock / 例外路徑漏放鎖」（RAII），
//         它【不改變取鎖順序】。
//             std::lock_guard<std::mutex> g1(mutexA);
//             std::lock_guard<std::mutex> g2(mutexB);   // 順序仍然相反 → 照樣死結
//         要一次安全取得多把鎖，需要的是 std::scoped_lock（C++17）
//         或 std::lock（C++11），它們才有死結避免演算法。
//     為什麼會錯：把 RAII（資源釋放保證）誤當成同步順序保證。
//         lock_guard 保證的是「離開作用域一定會解鎖」，
//         對「兩條執行緒以相反順序取鎖」這件事完全無能為力。
// ═══════════════════════════════════════════════════════════════════════════
//
// ───────────────────────────────────────────────────────────────────────────
// 【日常實務範例】銀行轉帳：AB-BA 死結與「鎖排序」修法
// ───────────────────────────────────────────────────────────────────────────
// 情境：轉帳必須同時鎖住轉出與轉入兩個帳戶，才能保證「扣款 + 入帳」是一筆
//       不可分割的交易。當 A→B 與 B→A 兩筆轉帳同時發生，就是最典型的 AB-BA。
//
// ⚠️ 依本課程規範，本檔是「刻意示範死結」的檔案，main() 必須保持純粹，
//    因此以下實務範例【以註解呈現、不編入執行檔】，避免污染死結示範本身。
//
//   struct Account {
//       std::mutex m;
//       long long balance;   // 單位：分，避免浮點誤差
//       int       id;
//   };
//
//   // ❌ 危險版本：取鎖順序取決於呼叫端傳入的參數順序
//   void transfer_bad(Account& from, Account& to, long long amount) {
//       std::lock_guard<std::mutex> l1(from.m);   // 執行緒1: from=A, to=B → 先 A
//       std::lock_guard<std::mutex> l2(to.m);     // 執行緒2: from=B, to=A → 先 B
//       from.balance -= amount;                   //  → 兩者順序相反，形成循環等待
//       to.balance   += amount;
//   }
//
//   // ✅ 修法一（C++17）：std::scoped_lock 一次取得兩把，內建死結避免演算法
//   void transfer_ok(Account& from, Account& to, long long amount) {
//       if (&from == &to) return;                 // 自己轉給自己：不可重複鎖同一把
//       std::scoped_lock lk(from.m, to.m);        // 全有或全無，不會 hold-and-wait
//       if (from.balance < amount) return;        // 餘額檢查必須在鎖【之內】做，
//       from.balance -= amount;                   // 否則就是 check-then-act 的
//       to.balance   += amount;                   // race condition（見 1.4-1）
//   }
//
//   // ✅ 修法二（C++11 / 或想明確表達鎖排序時）：用帳號 ID 建立全序
//   void transfer_ordered(Account& a, Account& b, long long amount) {
//       if (a.id == b.id) return;
//       Account& first  = (a.id < b.id) ? a : b;  // 永遠先鎖 ID 小的那個
//       Account& second = (a.id < b.id) ? b : a;
//       std::lock_guard<std::mutex> l1(first.m);
//       std::lock_guard<std::mutex> l2(second.m);
//       if (a.balance < amount) return;
//       a.balance -= amount;
//       b.balance += amount;
//   }
//
// 為什麼修法二有效：無論呼叫端寫 transfer(A,B) 還是 transfer(B,A)，
//   實際取鎖順序永遠是「ID 小 → ID 大」。所有執行緒的等待方向一致，
//   有向圖不可能成環，Coffman 的第 ④ 條件被打破 → 結構上不可能死結。
//   排序鍵用 ID 比用位址好：ID 穩定、可跨行程、可寫進日誌重現問題。
// ───────────────────────────────────────────────────────────────────────────

#include <chrono>
#include <iostream>
#include <thread>
#include <mutex>

std::mutex mutexA;
std::mutex mutexB;

void thread1() {
    std::cout << "執行緒1: 嘗試獲取 mutexA..." << std::endl;
    mutexA.lock();
    std::cout << "執行緒1: 已獲取 mutexA" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "執行緒1: 嘗試獲取 mutexB..." << std::endl;
    mutexB.lock();  // 等待執行緒2釋放 mutexB
    std::cout << "執行緒1: 已獲取 mutexB" << std::endl;
    
    mutexB.unlock();
    mutexA.unlock();
}

void thread2() {
    std::cout << "執行緒2: 嘗試獲取 mutexB..." << std::endl;
    mutexB.lock();
    std::cout << "執行緒2: 已獲取 mutexB" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "執行緒2: 嘗試獲取 mutexA..." << std::endl;
    mutexA.lock();  // 等待執行緒1釋放 mutexA
    std::cout << "執行緒2: 已獲取 mutexA" << std::endl;
    
    mutexA.unlock();
    mutexB.unlock();
}

int main() {
#ifdef DEMONSTRATE_UB
    std::thread t1(thread1);
    std::thread t2(thread2);

    t1.join();
    t2.join();

    // 💀 一旦兩個執行緒真的交錯，就到不了這裡（實測每次都到不了）；
    //    但若排程剛好讓 thread1 先整段跑完，這行是【有可能】被印出來的。
    std::cout << "完成" << std::endl;
#else
    std::cout << "已略過極可能死結的示範（預設關閉）。" << std::endl;
    std::cout << "要重現請加上 -DDEMONSTRATE_UB 重新編譯，"
                 "並用 timeout 觀察它不會自行結束。" << std::endl;
#endif
    return 0;
}

// 編譯（預設：安全路徑，不會死結，正常結束）:
//   g++ -std=c++17 -Wall -Wextra -pthread 課程\ 1.4：多執行緒的挑戰2.cpp -o deadlock
//
// 編譯（開啟死結示範，極可能永遠停住，務必用 timeout 保護）:
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB 課程\ 1.4：多執行緒的挑戰2.cpp -o deadlock_demo
//   timeout 5 ./deadlock_demo ; echo "exit=$?"
//
// 偵測鎖順序反轉（在還沒真的死結前就先示警，推薦）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread -DDEMONSTRATE_UB 課程\ 1.4：多執行緒的挑戰2.cpp -o deadlock_tsan

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 1.4：多執行緒的挑戰2.cpp" -o challenge2
//       （示範 AB-BA 死鎖，預設不會觸發；要重現請加 -DDEMONSTRATE_UB 並自備 timeout）

// 【非決定性說明（每次執行都不同）】

// === 預期輸出 ===
//
// 【路徑一：預設編譯（未定義 DEMONSTRATE_UB）】—— 本機實測，exit=0，正常結束
// 已略過極可能死結的示範（預設關閉）。
// 要重現請加上 -DDEMONSTRATE_UB 重新編譯，並用 timeout 觀察它不會自行結束。
//
// 【路徑二：加上 -DDEMONSTRATE_UB】—— 本機實測（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）
// ⚠️ 這條路徑【不會自行結束】。以下輸出是 `timeout 5` 砍掉它之前印出來的，
//    exit=124 就是 timeout 的逾時退出碼（不是程式自己回傳的）。
//    ⚠️ 但這是【時序相依】的死結，不是標準保證：
//       若排程讓 thread1 整段先跑完，程式會印出 "完成" 並以 exit=0 結束。
//       連續三次實測都死結（exit=124），代表 sleep_for(100ms) 把交錯機率
//       拉得很高，但機率高 ≠ 保證。
//
// 執行緒1: 嘗試獲取 mutexA...
// 執行緒1: 已獲取 mutexA
// 執行緒2: 嘗試獲取 mutexB...
// 執行緒2: 已獲取 mutexB
// 執行緒1: 嘗試獲取 mutexB...
// 執行緒2: 嘗試獲取 mutexA...
// exit=124                    ← 逾時被砍；"完成" 這行沒有被印出來
//
//   → 最後兩行「嘗試獲取」之後就再也沒有下文：
//     thread1 拿著 A 等 B、thread2 拿著 B 等 A，形成循環等待。
//     此時兩條執行緒都睡在 futex 上，CPU 使用率接近 0%（死結的典型特徵；
//     若是 CPU 100% 但沒進展，那就是活鎖而非死結）。
//
//   1. 三次執行的訊息【順序都不一樣】（有時 thread1 先報到、有時 thread2 先），
//      因為兩條執行緒誰先被排程並不固定。
//   2. 某次執行還印出了絞在一起的畫面：
//          執行緒1: 已獲取 mutexA執行緒2: 嘗試獲取 mutexB...
//          執行緒2: 已獲取 mutexB
//      這是因為 `std::cout << ... << std::endl` 是多次獨立的 << 呼叫，
//      兩條執行緒可以在中間插隊。標準保證並行使用 std::cout【不會有 data race】
//      （不是 UB），但不保證多次 << 之間不被插入 ——
//      這是一個「沒有 data race 卻有 race condition」的實例，
//      正好對照 1.4-1 檔頭的定義差異。
