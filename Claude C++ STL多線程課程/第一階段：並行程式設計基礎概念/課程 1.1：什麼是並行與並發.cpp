// =============================================================================
//  課程 1.1：什麼是並行與並發（Concurrency vs Parallelism）
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<thread>（std::thread / this_thread / hardware_concurrency）、
//           <chrono>（steady_clock 計時）、<iostream>、<string>
//   標準版本：C++11（std::thread 與 <chrono> 皆為 C++11 引入）
//
//     std::thread t(func, args...);                    // 建立並【立即啟動】
//     void t.join();                                   // 等待結束
//     std::thread::id std::this_thread::get_id();      // 目前執行緒的 id
//     unsigned std::thread::hardware_concurrency();    // 核心數【提示】，可能為 0
//
//   複雜度：建立一條執行緒在 Linux 上約 20–80 微秒（clone(2) + 堆疊配置）；
//           本檔完整教材（含圖表與四種組合的完整討論）見下方 markdown 區塊。
//
// 【詳細解釋 Explanation】
//   下方的 markdown 講義已完整說明概念與生活比喻。這裡補上三個
//   「講義講了定義、但實際寫程式才會踩到」的關鍵點。
//
// 【1. 一句話分清楚：並發是「結構」，並行是「執行」】
//   Rob Pike 的經典說法最精準：
//       "Concurrency is about dealing with lots of things at once.
//        Parallelism is about doing lots of things at once."
//   * 並發（concurrency）是【程式的結構】——你把程式寫成多個可獨立推進的任務。
//     這是【你寫程式碼時】的決定，與硬體無關。
//   * 並行（parallelism）是【執行時的現象】——多個任務在同一時刻真的同時跑。
//     這是【執行期由硬體與 OS 決定】的結果，你的程式碼管不到。
//   關鍵推論：同一份並發程式碼，在單核心上就是並發不並行（時間分片），
//   在多核心上就變成並發且並行。【你寫的是並發，你得到的可能是並行】。
//   所以「並行一定並發，並發不一定並行」。
//
// 【2. 本檔的實驗其實證明了什麼】
//   方式一（循序）約 900ms，方式二（並發）約 300ms，加速約 3 倍。
//   ⚠️ 但要小心解讀：這 3 倍【不是因為 CPU 平行運算】。
//   performTask 的主體是 sleep_for(100ms)，那是【阻塞等待】，不吃 CPU。
//   三條執行緒同時「睡覺」，所以總時間 ≈ 單一任務的時間。
//   這其實是【I/O 密集型】任務的加速模型 —— 即使在【單核心】機器上，
//   本檔一樣會得到接近 3 倍的加速，因為瓶頸是等待而不是運算。
//   真正的 CPU 平行加速（需要多核心）見課程 1.2 第 1 部分的質數計算。
//   把這兩者混為一談是初學者最常見的誤解。
//
// 【3. 為什麼輸出會交錯，而且每次都不一樣】
//   三條執行緒同時對 std::cout 輸出。C++11 起標準保證
//   （[iostream.objects]）對標準串流的並行使用【不會造成 data race】，
//   但它【不保證原子性】—— 一次 << 鏈中的多個片段之間，
//   其他執行緒完全可以插進來。所以你會看到：
//       [任務A] 執行第 1 次迭代（執行緒 ID: [任務B] 執行第 1 次...
//   這【不是 bug，也不是未定義行為】，而是缺少輸出鎖的必然結果。
//   要保證整行不被打斷，必須自己加 mutex，或先組成單一字串再一次輸出。
//   本檔下方的實務範例會示範正確做法。
//
// 【概念補充 Concept Deep Dive】
//   * std::thread::id 的性質：它是一個不透明型別，只保證可比較（==、<）
//     與可輸出。印出來的【數值是實作定義的】——libstdc++ 印的是
//     pthread_t 的值，MSVC 印的是 Windows thread id，兩者格式完全不同。
//     不可把它當成穩定的識別碼寫進日誌格式或拿來做雜湊假設。
//     ⚠️ 更要注意：執行緒結束後，它的 id 可以被【重複使用】給新執行緒。
//
//   * 執行緒【一建立就開始跑】：std::thread 沒有「先建立、之後再 start()」
//     的兩段式 API（Java 的 Thread 有）。建構子返回時，新執行緒可能
//     已經跑完了。所以本檔中 threadA 在 threadB 還沒建立時就已在執行 ——
//     這是輸出交錯的來源之一。
//
//   * 為什麼用 steady_clock 而非 system_clock：system_clock 是牆上時鐘，
//     可能被 NTP 校時往回跳，量時間間隔可能得到負數。
//     steady_clock 保證單調遞增，是量測經過時間的唯一正確選擇。
//
//   * hardware_concurrency() 只是【提示】：標準允許回傳 0（資訊不可得）。
//     ⚠️ 且在容器（Docker/K8s）中它回報【宿主機】的核心數，
//     不反映 cgroup 的 CPU 配額 —— 只分到 2 核的容器照樣回報 64。
//
//   * 邏輯核心 ≠ 實體核心：本機回報 16 是【邏輯】核心數。
//     若啟用 SMT/Hyper-Threading，實體核心可能只有 8。
//     兩個邏輯核心共用執行單元，純運算任務的加速遠低於 2 倍。
//
// 【注意事項 Pay Attention】
//   1. 本檔的加速來自「同時等待」而非「同時運算」——這是 I/O 密集模型，
//      單核心也有效。別把它當成 CPU 平行的證據。
//   2. 多執行緒輸出到 std::cout 會【交錯】。無 data race，但無原子性。
//      每次執行的交錯順序都不同。
//   3. 執行緒 ID 的數值是【實作定義】的，且結束後可能被重複使用。
//   4. hardware_concurrency() 可能回傳 0，且在容器中不反映 cgroup 限制。
//   5. std::thread 建構後【立即】開始執行，沒有兩段式 start()。
//   6. 所有毫秒數、交錯順序、執行緒 ID【每次執行都不同】。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】並行 vs 並發
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 並發（concurrency）與並行（parallelism）的差別是什麼？
//     答：並發是【程式的結構】—— 把程式組織成多個可獨立推進的任務，
//         是寫程式碼時的設計決定，與硬體無關。並行是【執行時的現象】——
//         多個任務在同一時刻真的同時執行，需要多核心硬體支援。
//         同一份並發程式碼，在單核心上是並發不並行（時間分片），
//         在多核心上才變成並發且並行。所以並行一定並發，並發不一定並行。
//     追問：那在單核心機器上寫多執行緒還有意義嗎？
//         → 非常有意義。單核心無法提升 CPU 運算吞吐量，但（a）能讓 UI
//           保持響應；（b）I/O 密集任務在等待時會讓出 CPU，
//           照樣能得到接近執行緒數的加速 —— 本檔的實驗正是這種情況。
//
// 🔥 Q2. 本檔中並發版本比循序版本快約 3 倍，這是多核心平行運算的功勞嗎？
//     答：不是。performTask 的主體是 sleep_for(100ms)，那是阻塞等待、
//         不消耗 CPU。三條執行緒同時「睡覺」，總時間才接近單一任務的時間。
//         這是【I/O 密集型】的加速模型，即使在單核心機器上也會得到
//         幾乎相同的 3 倍加速。真正的 CPU 平行加速要看課程 1.2 的質數計算，
//         那才需要多核心，而且加速比會受 Amdahl 定律與負載不均限制。
//     追問：怎麼判斷一個任務是 CPU 密集還是 I/O 密集？
//         → 看它在等待還是在算。跑滿一顆核心的是 CPU 密集（加速上限 = 核心數）；
//           大部分時間在等網路/磁碟/鎖的是 I/O 密集（可以開遠多於核心數的執行緒）。
//
// ⚠️ 陷阱. 多條執行緒同時 std::cout << 輸出，會不會造成 undefined behavior？
//     答：不會。C++11 起標準明文保證（[iostream.objects]）對標準串流物件的
//         並行使用【不會造成 data race】。但它【不保證原子性】——
//         一次 << 鏈中的多個片段之間，其他執行緒可以插進來，
//         所以輸出會交錯甚至同一行被撕裂。這是「格式錯亂」，不是 UB。
//     為什麼會錯：把「交錯」誤判成「data race / UB」，或反過來以為
//         「標準保證了安全」就等於「輸出不會亂」。兩者是不同層次的保證：
//         * 執行緒安全（無 data race）→ 標準給你了；
//         * 輸出原子性（整行不被打斷）→ 標準【沒有】給你，要自己加鎖。
//         正確解法是用 mutex 保護整個輸出敘述，或先 ostringstream 組好
//         再一次 <<。（注意：後者仍不保證絕對原子，但把窗口縮到最小。）
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第一階段：並行程式設計基礎概念

## 課程 1.1：什麼是並行與並發

---

### 引言

在我們開始撰寫任何多執行緒程式碼之前，必須先釐清兩個經常被混淆的核心概念：**並發（Concurrency）** 與 **並行（Parallelism）**。這兩個術語在中文技術文獻中常被混用，但它們描述的是本質上不同的概念。理解這個區別，是掌握多執行緒程式設計的第一步。

---

### 一、從日常生活理解並發與並行

在深入技術定義之前，讓我們先用日常生活的例子來建立直覺。

#### 情境一：一位廚師準備多道菜（並發）

想像一位廚師需要同時準備三道菜：湯、主菜、甜點。這位廚師只有一雙手，他的工作方式是：

1. 先把湯放到爐子上煮
2. 趁湯在煮的時候，開始切主菜的食材
3. 切到一半，回去攪拌一下湯
4. 繼續處理主菜，把它放進烤箱
5. 趁主菜在烤的時候，開始準備甜點
6. 來回切換，直到三道菜都完成

這位廚師在「同時處理」三道菜，但在任何一個瞬間，他實際上只在做一件事。這就是**並發（Concurrency）**——在一段時間內處理多個任務，但不一定是同時執行。

#### 情境二：三位廚師各自準備一道菜（並行）

現在想像有三位廚師，每人負責一道菜：

- 廚師 A 專門煮湯
- 廚師 B 專門做主菜
- 廚師 C 專門做甜點

三位廚師在同一時刻各自工作，真正地同時執行三個任務。這就是**並行（Parallelism）**——在同一時刻同時執行多個任務。

---

### 二、技術定義

#### 並發（Concurrency）

**並發是一種程式的結構化方式，它將程式組織成多個可以獨立執行的任務。**

關鍵特點：
- 強調的是程式的**結構**與**設計**
- 多個任務在**邏輯上**同時進行
- 實際執行時可能是交錯進行（在單核心 CPU 上）
- 任務之間可能有相互依賴或需要協調

用更精確的技術語言來說：並發是指程式具有處理多個任務的**能力**，這些任務的執行時間在邏輯上是重疊的。

#### 並行（Parallelism）

**並行是一種執行方式，指多個任務在同一時刻真正地同時執行。**

關鍵特點：
- 強調的是**執行**與**效能**
- 多個任務在**物理上**同時進行
- 需要多核心 CPU 或多處理器的硬體支援
- 目的通常是提升運算效能

用更精確的技術語言來說：並行是指多個運算在同一時間點**實際上**同時發生。

---

### 三、兩者的關係

並發與並行的關係可以用以下四種組合來理解：

```
┌─────────────────────────────────────────────────────────────┐
│                      並發 vs 並行                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 非並發、非並行：                                          │
│     [任務A完成] → [任務B完成] → [任務C完成]                    │
│     一次只處理一個任務，完成後才開始下一個                       │
│                                                             │
│  2. 並發、非並行（單核心）：                                   │
│     [A1][B1][A2][C1][B2][A3][C2][B3]...                      │
│     多個任務交錯執行，但任一時刻只有一個在執行                   │
│                                                             │
│  3. 並行、非並發：                                            │
│     核心1: [任務A──────────────]                              │
│     核心2: [任務B──────────────]                              │
│     多個獨立任務同時執行，但彼此無關聯                          │
│                                                             │
│  4. 並發且並行（多核心）：                                     │
│     核心1: [A1][A2][B3][A4][C2]...                           │
│     核心2: [B1][C1][B2][A3][B4]...                           │
│     多個相關任務在多核心上同時且交錯執行                        │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

**重要結論**：
- 並發是一種設計概念，與硬體無關
- 並行是一種執行現象，需要硬體支援
- 並發程式在單核心上會以時間分片方式執行
- 並發程式在多核心上可以真正並行執行
- 並行一定是並發的，但並發不一定是並行的

---

### 四、C++ 程式碼示範

讓我們用實際的程式碼來觀察這些概念。

#### 範例程式：觀察並發執行

```cpp
// 檔案名稱：lesson_1_1_concurrency_demo.cpp
// 課程：1.1 - 什麼是並行與並發
// 說明：示範並發執行的基本概念

#include <iostream>
#include <thread>
#include <chrono>

// 模擬一個需要時間的任務
void performTask(const std::string& taskName, int iterations) {
    for (int i = 1; i <= iterations; ++i) {
        // 輸出目前的進度
        std::cout << "[" << taskName << "] 執行第 " << i 
                  << " 次迭代（執行緒 ID: " 
                  << std::this_thread::get_id() << "）" << std::endl;
        
        // 模擬工作需要一些時間
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "[" << taskName << "] 完成！" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    並發執行示範程式" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "主執行緒 ID: " << std::this_thread::get_id() << std::endl;
    std::cout << "硬體支援的執行緒數量: " 
              << std::thread::hardware_concurrency() << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 記錄開始時間
    auto startTime = std::chrono::steady_clock::now();

    // 方式一：循序執行（非並發）
    std::cout << "\n【方式一】循序執行：" << std::endl;
    auto seqStart = std::chrono::steady_clock::now();
    
    performTask("任務A", 3);
    performTask("任務B", 3);
    performTask("任務C", 3);
    
    auto seqEnd = std::chrono::steady_clock::now();
    auto seqDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        seqEnd - seqStart).count();
    std::cout << "循序執行總時間: " << seqDuration << " 毫秒" << std::endl;

    std::cout << "\n----------------------------------------" << std::endl;

    // 方式二：並發執行
    std::cout << "\n【方式二】並發執行：" << std::endl;
    auto concStart = std::chrono::steady_clock::now();
    
    // 建立三個執行緒，每個執行緒執行一個任務
    std::thread threadA(performTask, "任務A", 3);
    std::thread threadB(performTask, "任務B", 3);
    std::thread threadC(performTask, "任務C", 3);
    
    // 等待所有執行緒完成
    threadA.join();
    threadB.join();
    threadC.join();
    
    auto concEnd = std::chrono::steady_clock::now();
    auto concDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        concEnd - concStart).count();
    std::cout << "並發執行總時間: " << concDuration << " 毫秒" << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "效能比較：" << std::endl;
    std::cout << "循序執行: " << seqDuration << " 毫秒" << std::endl;
    std::cout << "並發執行: " << concDuration << " 毫秒" << std::endl;
    std::cout << "加速比: " << static_cast<double>(seqDuration) / concDuration 
              << " 倍" << std::endl;
    std::cout << "（注意：這個加速來自「同時等待」而非「同時運算」——"
                 "performTask 的主體是 sleep，屬於 I/O 密集模型，"
                 "單核心機器也會有幾乎相同的加速）" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "\n=== LeetCode 1114. Print in Order ===" << std::endl;
    demoPrintInOrder();

    std::cout << "\n=== 日常實務：並發拉取設定 + 不撕裂的日誌 ===" << std::endl;
    demoConcurrentConfigFetch();

    // startTime 在程式開頭記錄，這裡用來回報整支程式的總耗時
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();
    std::cout << "\n========================================" << std::endl;
    std::cout << "全部示範結束，總耗時: " << totalMs << " 毫秒" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
```

#### 編譯與執行

在你的終端機中執行以下命令：

```bash
# 編譯（需要連結 pthread 函式庫）
g++ -std=c++17 -pthread -o lesson_1_1 lesson_1_1_concurrency_demo.cpp

# 執行
./lesson_1_1
```

#### 預期輸出

```
========================================
    並發執行示範程式
========================================
主執行緒 ID: 140234567891520
硬體支援的執行緒數量: 8
----------------------------------------

【方式一】循序執行：
[任務A] 執行第 1 次迭代（執行緒 ID: 140234567891520）
[任務A] 執行第 2 次迭代（執行緒 ID: 140234567891520）
[任務A] 執行第 3 次迭代（執行緒 ID: 140234567891520）
[任務A] 完成！
[任務B] 執行第 1 次迭代（執行緒 ID: 140234567891520）
[任務B] 執行第 2 次迭代（執行緒 ID: 140234567891520）
[任務B] 執行第 3 次迭代（執行緒 ID: 140234567891520）
[任務B] 完成！
[任務C] 執行第 1 次迭代（執行緒 ID: 140234567891520）
[任務C] 執行第 2 次迭代（執行緒 ID: 140234567891520）
[任務C] 執行第 3 次迭代（執行緒 ID: 140234567891520）
[任務C] 完成！
循序執行總時間: 903 毫秒

----------------------------------------

【方式二】並發執行：
[任務A] 執行第 1 次迭代（執行緒 ID: 140234567891521）
[任務B] 執行第 1 次迭代（執行緒 ID: 140234567891522）
[任務C] 執行第 1 次迭代（執行緒 ID: 140234567891523）
[任務A] 執行第 2 次迭代（執行緒 ID: 140234567891521）
[任務B] 執行第 2 次迭代（執行緒 ID: 140234567891522）
[任務C] 執行第 2 次迭代（執行緒 ID: 140234567891523）
[任務A] 執行第 3 次迭代（執行緒 ID: 140234567891521）
[任務B] 執行第 3 次迭代（執行緒 ID: 140234567891522）
[任務C] 執行第 3 次迭代（執行緒 ID: 140234567891523）
[任務A] 完成！
[任務B] 完成！
[任務C] 完成！
並發執行總時間: 302 毫秒

========================================
效能比較：
循序執行: 903 毫秒
並發執行: 302 毫秒
加速比: 2.99 倍
========================================
```

---

### 五、程式碼逐行解析

讓我們詳細分析這個程式的每個部分：

#### 標頭檔引入

```cpp
#include <iostream>     // 標準輸入輸出
#include <thread>       // C++11 執行緒支援
#include <chrono>       // C++11 時間函式庫
```

`<thread>` 是 C++11 引入的標準執行緒函式庫，提供了 `std::thread` 類別。
`<chrono>` 提供了高精度的時間測量功能。

#### 任務函式

```cpp
void performTask(const std::string& taskName, int iterations) {
    for (int i = 1; i <= iterations; ++i) {
        std::cout << "[" << taskName << "] 執行第 " << i 
                  << " 次迭代（執行緒 ID: " 
                  << std::this_thread::get_id() << "）" << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "[" << taskName << "] 完成！" << std::endl;
}
```

- `std::this_thread::get_id()` 返回當前執行緒的唯一識別碼
- `std::this_thread::sleep_for()` 讓當前執行緒休眠指定時間
- `std::chrono::milliseconds(100)` 表示 100 毫秒的時間間隔

#### 取得硬體資訊

```cpp
std::cout << "硬體支援的執行緒數量: " 
          << std::thread::hardware_concurrency() << std::endl;
```

`std::thread::hardware_concurrency()` 返回系統支援的並行執行緒數量，通常等於 CPU 核心數（包含超執行緒）。這個值可以幫助我們決定應該建立多少執行緒。

#### 建立與管理執行緒

```cpp
std::thread threadA(performTask, "任務A", 3);
std::thread threadB(performTask, "任務B", 3);
std::thread threadC(performTask, "任務C", 3);

threadA.join();
threadB.join();
threadC.join();
```

- `std::thread` 的建構子接受一個可呼叫物件（函式、Lambda、函式物件等）和其參數
- 執行緒在建構時就開始執行
- `join()` 會阻塞當前執行緒，直到被 join 的執行緒執行完畢

---

### 六、觀察重點

從輸出結果中，我們可以觀察到幾個重要現象：

#### 1. 執行緒 ID 的差異

循序執行時，所有任務的執行緒 ID 相同（都是主執行緒）。並發執行時，每個任務有不同的執行緒 ID，表示它們在不同的執行緒中運行。

#### 2. 執行順序的交錯

在並發執行的輸出中，你會看到任務 A、B、C 的輸出交錯出現。這說明這些任務正在「同時」進行，而不是一個完成後才開始下一個。

#### 3. 執行時間的差異

- 循序執行：約 900 毫秒（3 個任務 × 3 次迭代 × 100 毫秒）
- 並發執行：約 300 毫秒（3 次迭代 × 100 毫秒，因為任務同時進行）

這證明了並發/並行執行可以顯著提升效能。

#### 4. 輸出的不確定性

每次執行程式，並發部分的輸出順序可能略有不同。這是因為作業系統的執行緒排程是非確定性的，這也是多執行緒程式設計的挑戰之一。

---

### 七、單核心 vs 多核心

讓我們更深入理解在不同硬體上的行為差異：

```
單核心 CPU 上的並發執行：
┌────────────────────────────────────────────────┐
│ 時間軸 →                                        │
│                                                │
│ CPU: [A1][B1][C1][A2][B2][C2][A3][B3][C3]      │
│                                                │
│ 看起來「同時」進行，實際上是快速切換             │
│ 總時間 ≈ 循序執行（可能還有切換開銷）            │
└────────────────────────────────────────────────┘

多核心 CPU 上的並發執行（真正的並行）：
┌────────────────────────────────────────────────┐
│ 時間軸 →                                        │
│                                                │
│ 核心1: [A1][A2][A3]                             │
│ 核心2: [B1][B2][B3]                             │
│ 核心3: [C1][C2][C3]                             │
│                                                │
│ 真正同時執行，總時間 ≈ 單一任務的時間            │
└────────────────────────────────────────────────┘
```

---

### 八、常見術語對照

在學習多執行緒程式設計時，你會遇到許多相關術語，以下是它們的對照：

| 英文術語 | 中文翻譯 | 說明 |
|----------|----------|------|
| Concurrency | 並發 | 程式結構上同時處理多個任務的能力 |
| Parallelism | 並行 | 實際上同時執行多個任務 |
| Thread | 執行緒 | 程式執行的最小單位 |
| Process | 程序/進程 | 執行中的程式實例 |
| Multithreading | 多執行緒 | 使用多個執行緒的程式設計技術 |
| Core | 核心 | CPU 中可獨立執行指令的單元 |
| Context Switch | 上下文切換 | CPU 在不同執行緒間切換的過程 |
| Time Slicing | 時間分片 | 作業系統分配 CPU 時間給各執行緒的機制 |

---

### 九、本課重點回顧

1. **並發（Concurrency）** 是一種程式結構，強調處理多個任務的能力
2. **並行（Parallelism）** 是一種執行方式，強調同時執行多個任務
3. 並發程式在單核心上會以時間分片方式執行
4. 並發程式在多核心上可以真正並行執行，獲得效能提升
5. C++ 使用 `<thread>` 標頭檔提供多執行緒支援
6. `std::thread` 用於建立和管理執行緒
7. 多執行緒程式的執行順序具有不確定性

---

### 十、課後練習

1. **修改迭代次數**：將 iterations 改為 5 或 10，觀察時間變化
2. **增加任務數量**：新增任務 D、E、F，觀察並發執行的效果
3. **觀察你的硬體**：記錄 `hardware_concurrency()` 的值，思考如果任務數量超過這個值會怎樣
4. **移除 sleep**：把 `sleep_for` 註解掉，觀察輸出會有什麼變化

---

### 下一課預告

在 **課程 1.2：為什麼需要多執行緒** 中，我們將探討：
- 多執行緒程式設計的實際應用場景
- 效能提升的具體案例
- 響應性改善的重要性
- 多執行緒的適用時機判斷

---

準備好繼續下一課了嗎？


這是一個很好的問題！讓我從作業系統和硬體層面來解釋 Context Switch 的機制。

## 簡短回答

**這是由作業系統自動處理的，程式設計師不需要手動管理。**

---

## 詳細解釋

### Context Switch 的運作機制

```
時間軸:
────────────────────────────────────────────────────────────

CPU0 執行 Thread A          OS 介入           CPU1 繼續執行 Thread A
    │                          │                      │
    ▼                          ▼                      ▼
┌─────────┐              ┌──────────┐            ┌─────────┐
│ 執行中  │  ──觸發──>   │ 保存狀態 │  ──排程──> │ 恢復狀態│
│ Thread A│              │ 到 PCB   │            │ 從 PCB  │
└─────────┘              └──────────┘            └─────────┘
```

### 1. 什麼被保存？（Thread Context）

當 Context Switch 發生時，OS 會保存該 thread 的完整執行狀態：

```
┌─────────────────────────────────────────────────┐
│           Thread Control Block (TCB)            │
│         或 Process Control Block (PCB)          │
├─────────────────────────────────────────────────┤
│                                                 │
│  ┌─────────────────────────────────────────┐    │
│  │  通用暫存器 (General Purpose Registers) │    │
│  │  • RAX, RBX, RCX, RDX, ...              │    │
│  │  • R8-R15 (x86-64)                      │    │
│  └─────────────────────────────────────────┘    │
│                                                 │
│  ┌─────────────────────────────────────────┐    │
│  │  程式計數器 (Program Counter / RIP)     │    │
│  │  • 下一條要執行的指令位址               │    │
│  └─────────────────────────────────────────┘    │
│                                                 │
│  ┌─────────────────────────────────────────┐    │
│  │  堆疊指標 (Stack Pointer / RSP)         │    │
│  │  • 目前堆疊的位置                       │    │
│  └─────────────────────────────────────────┘    │
│                                                 │
│  ┌─────────────────────────────────────────┐    │
│  │  狀態暫存器 (RFLAGS)                    │    │
│  │  • CPU 狀態旗標                         │    │
│  └─────────────────────────────────────────┘    │
│                                                 │
│  ┌─────────────────────────────────────────┐    │
│  │  浮點/向量暫存器                        │    │
│  │  • FPU 狀態, SSE/AVX 暫存器             │    │
│  └─────────────────────────────────────────┘    │
│                                                 │
└─────────────────────────────────────────────────┘
```

### 2. Context Switch 的完整流程

```
步驟 1: 觸發 (在 CPU0 上)
─────────────────────────
• Timer interrupt (時間片用完)
• I/O 等待
• 主動 yield
• 更高優先級 thread 就緒
        │
        ▼
步驟 2: 保存狀態 (OS Kernel)
─────────────────────────
• 進入 kernel mode
• 將 CPU0 所有暫存器值 → 寫入 Thread A 的 TCB
• TCB 存放在記憶體中 (所有 CPU 都能存取)
        │
        ▼
步驟 3: 排程決策 (Scheduler)
─────────────────────────
• 選擇下一個要執行的 thread
• 決定在哪個 CPU 上執行
        │
        ▼
步驟 4: 恢復狀態 (在 CPU1 上)
─────────────────────────
• 從 Thread A 的 TCB 讀取保存的狀態
• 將狀態載入到 CPU1 的暫存器
• 切換到 user mode
• 從 Program Counter 指向的位置繼續執行
```

### 3. 記憶體是共享的

這是關鍵點：

```
┌─────────────────────────────────────────────────────────┐
│                     主記憶體 (RAM)                       │
│                                                         │
│   ┌─────────────┐  ┌─────────────┐  ┌─────────────┐     │
│   │  Thread A   │  │  Thread A   │  │  Thread A   │     │
│   │   的 TCB    │  │   的 Stack  │  │   的 Heap   │     │
│   └─────────────┘  └─────────────┘  └─────────────┘     │
│          ▲                ▲                ▲            │
└──────────│────────────────│────────────────│────────────┘
           │                │                │
     ┌─────┴─────┐    ┌─────┴─────┐    ┌─────┴─────┐
     │   CPU0    │    │   CPU1    │    │   CPU2    │
     │  可存取   │    │  可存取   │    │  可存取   │
     └───────────┘    └───────────┘    └───────────┘
```

**重點：** TCB、Stack、Heap 都在共享記憶體中，任何 CPU 都能存取。

---

## 程式設計師需要關心什麼？

雖然 Context Switch 本身是自動的，但在多執行緒程式設計中，你需要處理：

### 你不需要管的（OS 自動處理）
- 暫存器的保存/恢復
- Program Counter 的追蹤
- Stack 的切換
- 執行緒在哪個 CPU 上執行

### 你需要管的（程式設計師責任）
```cpp
// 多個 thread 存取共享資料時，需要同步機制
std::mutex mtx;
int shared_data = 0;

void thread_function() {
    std::lock_guard<std::mutex> lock(mtx);  // 你需要處理這個
    shared_data++;                           // 保護共享資料
}
```

---

## 簡單比喻

想像一個接力賽跑：

```
情境：跑者 A (Thread) 需要休息，跑者 B (另一個 CPU) 接手

┌──────────────────────────────────────────────────────────┐
│                                                          │
│   跑者 A 在 CPU0 跑          裁判 (OS) 介入               │
│        │                          │                      │
│        ▼                          ▼                      │
│   ┌─────────┐              ┌──────────────┐              │
│   │ • 目前位置 (PC)        │ 記錄到計分板  │              │
│   │ • 已跑距離             │ (TCB in RAM) │              │
│   │ • 體力狀態 (暫存器)    │              │              │
│   └─────────┘              └──────────────┘              │
│                                    │                     │
│                                    ▼                     │
│                            ┌──────────────┐              │
│                            │ CPU1 空閒了  │              │
│                            │ 讀取計分板   │              │
│                            │ 從記錄點繼續 │              │
│                            └──────────────┘              │
│                                                          │
│   計分板 (TCB) 在公共區域，任何人都能讀取                 │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

---

## 總結

| 問題 | 答案 |
|------|------|
| 誰負責保存/恢復狀態？ | **作業系統 (OS Kernel)** |
| 狀態存在哪裡？ | **記憶體中的 TCB/PCB** |
| CPU1 怎麼知道 CPU0 的狀態？ | **從共享記憶體的 TCB 讀取** |
| 程式設計師需要處理嗎？ | **不需要處理 Context Switch 本身** |
| 程式設計師需要處理什麼？ | **共享資料的同步 (mutex, atomic 等)** |

這就是為什麼你寫多執行緒程式時，只需要關心 **資料同步**，而不需要關心 thread 會在哪個 CPU 上執行。OS 會處理所有底層的 Context Switch 細節！

有任何問題想深入了解嗎？
*/

// 檔案名稱：lesson_1_1_concurrency_demo.cpp
// 課程：1.1 - 什麼是並行與並發
// 說明：示範並發執行的基本概念

#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// 模擬一個需要時間的任務
void performTask(const std::string& taskName, int iterations) {
    for (int i = 1; i <= iterations; ++i) {
        // 輸出目前的進度
        std::cout << "[" << taskName << "] 執行第 " << i 
                  << " 次迭代（執行緒 ID: " 
                  << std::this_thread::get_id() << "）" << std::endl;
        
        // 模擬工作需要一些時間
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "[" << taskName << "] 完成！" << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1114. Print in Order
//   題目：三條執行緒分別呼叫 first()、second()、third()，呼叫順序【任意】，
//         但必須保證輸出永遠是 "firstsecondthird"。
//   為什麼用到本主題：這題是「並發 ≠ 並行」最好的教學案例。
//         三條執行緒是【並發】的（結構上獨立、可任意交錯），
//         題目卻要求一個【確定的順序】。這正好戳破初學者最常見的錯覺：
//         「執行緒是照我建立的順序跑的」。實際上建立順序完全不保證執行順序，
//         你必須用同步原語【主動建立】順序關係。
//         本檔上方的實驗也會看到：threadA/B/C 依序建立，輸出卻是亂的。
//   解法：用 condition_variable + 一個 step 計數器。
//         每條執行緒等到「輪到自己」才動作，做完把 step 推進並喚醒其他人。
//   ⚠️ 為什麼 wait 一定要帶述詞（predicate）：
//         condition_variable 允許【虛假喚醒】（spurious wakeup）——
//         沒有人 notify 也可能醒來。述詞版的 wait 會在醒來後重新檢查條件，
//         不成立就繼續睡。寫成無述詞的 wait(lock) 就是 bug。
// -----------------------------------------------------------------------------
class Foo {
public:
    Foo() = default;

    void first(const std::function<void()>& printFirst) {
        std::unique_lock<std::mutex> lock(mtx_);
        printFirst();          // first() 永遠第一個執行，不需等待
        step_ = 2;
        cv_.notify_all();
    }

    void second(const std::function<void()>& printSecond) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return step_ == 2; });   // 述詞版，擋掉虛假喚醒
        printSecond();
        step_ = 3;
        cv_.notify_all();
    }

    void third(const std::function<void()>& printThird) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return step_ == 3; });
        printThird();
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int step_ = 1;
};

// 刻意用「與期望相反」的順序建立執行緒，證明結果與建立順序無關
void demoPrintInOrder() {
    Foo foo;
    std::string out;
    std::mutex outMtx;

    auto emit = [&](const char* s) {
        std::lock_guard<std::mutex> lock(outMtx);
        out += s;
    };

    // 注意建立順序是 third → second → first（故意顛倒）
    std::thread t3([&] { foo.third([&] { emit("third"); }); });
    std::thread t2([&] { foo.second([&] { emit("second"); }); });
    std::thread t1([&] { foo.first([&] { emit("first"); }); });

    t3.join();
    t2.join();
    t1.join();

    std::cout << "  執行緒建立順序: third → second → first（故意顛倒）" << std::endl;
    std::cout << "  實際輸出結果  : " << out << std::endl;
    std::cout << "  ← 建立順序完全不影響結果，順序是靠 condition_variable 建立的"
              << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】並發拉取多個設定來源，並保證日誌不被撕裂
//   情境：微服務啟動時要同時向 3 個來源拉取設定（設定中心、feature flag 服務、
//         憑證服務）。這些請求彼此獨立、都是網路 I/O，很適合並發執行 ——
//         總耗時從「三者相加」變成「最慢的那一個」。
//   為何同時示範日誌鎖：多條執行緒同時寫 log 是實務上一定會遇到的事。
//         std::cout 保證無 data race，但【不保證整行原子性】，
//         不加鎖的話日誌會像本檔上方實驗那樣被撕裂 ——
//         在正式環境中，撕裂的日誌等同於不可用的日誌。
//   正確做法（本範例採用）：先用 ostringstream 在執行緒本地把整行組好，
//         再在 mutex 保護下一次輸出。既縮短持鎖時間，又保證整行完整。
// -----------------------------------------------------------------------------
class SafeLogger {
public:
    void log(const std::string& tag, const std::string& msg) {
        std::ostringstream oss;                       // 先在執行緒本地組好整行
        oss << "    [" << tag << "] " << msg << '\n';
        std::string line = oss.str();

        std::lock_guard<std::mutex> lock(mtx_);       // 只在真正輸出時持鎖
        std::cout << line << std::flush;
    }

private:
    std::mutex mtx_;
};

void demoConcurrentConfigFetch() {
    SafeLogger logger;
    struct Source { const char* name; int latencyMs; };
    const std::vector<Source> sources = {
        {"config-center", 120},
        {"feature-flags",  80},
        {"cert-service",  150},
    };

    // ---- 循序版 ----
    auto t0 = std::chrono::steady_clock::now();
    for (const auto& s : sources) {
        std::this_thread::sleep_for(std::chrono::milliseconds(s.latencyMs));
    }
    auto seqMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::steady_clock::now() - t0).count();

    // ---- 並發版 ----
    t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> fetchers;
    for (const auto& s : sources) {
        fetchers.emplace_back([&logger, s]() {
            logger.log(s.name, "開始拉取設定...");
            std::this_thread::sleep_for(std::chrono::milliseconds(s.latencyMs));
            logger.log(s.name, "完成（耗時 " + std::to_string(s.latencyMs) + " ms）");
        });
    }
    for (auto& t : fetchers) t.join();
    auto conMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::steady_clock::now() - t0).count();

    std::cout << "  循序拉取: " << seqMs << " ms（120+80+150 相加）" << std::endl;
    std::cout << "  並發拉取: " << conMs << " ms（≈ 最慢的 cert-service 150ms）"
              << std::endl;
    std::cout << "  ← 每一行日誌都完整不撕裂，因為用 mutex 保護了整行輸出"
              << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    並發執行示範程式" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "主執行緒 ID: " << std::this_thread::get_id() << std::endl;
    std::cout << "硬體支援的執行緒數量: " 
              << std::thread::hardware_concurrency() << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 記錄開始時間
    auto startTime = std::chrono::steady_clock::now();

    // 方式一：循序執行（非並發）
    std::cout << "\n【方式一】循序執行：" << std::endl;
    auto seqStart = std::chrono::steady_clock::now();
    
    performTask("任務A", 3);
    performTask("任務B", 3);
    performTask("任務C", 3);
    
    auto seqEnd = std::chrono::steady_clock::now();
    auto seqDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        seqEnd - seqStart).count();
    std::cout << "循序執行總時間: " << seqDuration << " 毫秒" << std::endl;

    std::cout << "\n----------------------------------------" << std::endl;

    // 方式二：並發執行
    std::cout << "\n【方式二】並發執行：" << std::endl;
    auto concStart = std::chrono::steady_clock::now();
    
    // 建立三個執行緒，每個執行緒執行一個任務
    std::thread threadA(performTask, "任務A", 3);
    std::thread threadB(performTask, "任務B", 3);
    std::thread threadC(performTask, "任務C", 3);
    
    // 等待所有執行緒完成
    threadA.join();
    threadB.join();
    threadC.join();
    
    auto concEnd = std::chrono::steady_clock::now();
    auto concDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        concEnd - concStart).count();
    std::cout << "並發執行總時間: " << concDuration << " 毫秒" << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "效能比較：" << std::endl;
    std::cout << "循序執行: " << seqDuration << " 毫秒" << std::endl;
    std::cout << "並發執行: " << concDuration << " 毫秒" << std::endl;
    std::cout << "加速比: " << static_cast<double>(seqDuration) / concDuration 
              << " 倍" << std::endl;
    std::cout << "（注意：這個加速來自「同時等待」而非「同時運算」——"
                 "performTask 的主體是 sleep，屬於 I/O 密集模型，"
                 "單核心機器也會有幾乎相同的加速）" << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "\n=== LeetCode 1114. Print in Order ===" << std::endl;
    demoPrintInOrder();

    std::cout << "\n=== 日常實務：並發拉取設定 + 不撕裂的日誌 ===" << std::endl;
    demoConcurrentConfigFetch();

    // startTime 在程式開頭記錄，這裡用來回報整支程式的總耗時
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();
    std::cout << "\n========================================" << std::endl;
    std::cout << "全部示範結束，總耗時: " << totalMs << " 毫秒" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 1.1：什麼是並行與並發.cpp" -o concurrency11
//   本檔只用到 C++11 起就有的功能，以 -std=c++17 -pedantic-errors 驗證通過（0 警告）。

// 註:
//   ⚠️ 非決定性內容，【每次執行都不同】：
//   * 所有執行緒 ID（實作定義的數值，且執行緒結束後可能被重複使用）；
//   * 【方式二】中三個任務的輸出【交錯順序】——甚至可能出現同一行被撕裂
//   （例如 "[任務A] 執行第 1 次迭代（執行緒 ID: [任務B] 執行第 1..."），
//   這不是 bug，而是 std::cout 無原子性保證的必然結果；
//   * 所有毫秒數（總耗時、各段耗時）。
//   穩定不變的是【結構】：循序約 900ms、並發約 300ms、加速約 3 倍；
//   LeetCode 1114 的輸出【永遠】是 firstsecondthird（那正是同步的意義）。
//   ⚠️ 毫秒數會有 1–3 ms 的抖動（實測 900/901、300/301、1701/1703）：
//      sleep_for 只保證「【至少】睡這麼久」，實際受排程延遲影響，絕不會更短。
//      看到 901 而不是 900 是正常的，不是計算錯誤。
//   以下為本機（16 邏輯核心）某一次的實際執行結果。

// === 預期輸出 ===
// ========================================
//     並發執行示範程式
// ========================================
// 主執行緒 ID: 124939285014464
// 硬體支援的執行緒數量: 16
// ----------------------------------------
//
// 【方式一】循序執行：
// [任務A] 執行第 1 次迭代（執行緒 ID: 124939285014464）
// [任務A] 執行第 2 次迭代（執行緒 ID: 124939285014464）
// [任務A] 執行第 3 次迭代（執行緒 ID: 124939285014464）
// [任務A] 完成！
// [任務B] 執行第 1 次迭代（執行緒 ID: 124939285014464）
// [任務B] 執行第 2 次迭代（執行緒 ID: 124939285014464）
// [任務B] 執行第 3 次迭代（執行緒 ID: 124939285014464）
// [任務B] 完成！
// [任務C] 執行第 1 次迭代（執行緒 ID: 124939285014464）
// [任務C] 執行第 2 次迭代（執行緒 ID: 124939285014464）
// [任務C] 執行第 3 次迭代（執行緒 ID: 124939285014464）
// [任務C] 完成！
// 循序執行總時間: 901 毫秒
//
// ----------------------------------------
//
// 【方式二】並發執行：
// [任務A] 執行第 1 次迭代（執行緒 ID: 124939277432512）
// [任務B] 執行第 1 次迭代（執行緒 ID: 124939269039808）
// [任務C] 執行第 1 次迭代（執行緒 ID: 124939260647104）
// [任務B] 執行第 2 次迭代（執行緒 ID: 124939269039808）
// [任務A] 執行第 2 次迭代（執行緒 ID: 124939277432512）
// [任務C] 執行第 2 次迭代（執行緒 ID: 124939260647104）
// [任務B] 執行第 3 次迭代（執行緒 ID: 124939269039808）
// [任務A] 執行第 3 次迭代（執行緒 ID: 124939277432512）
// [任務C] 執行第 3 次迭代（執行緒 ID: 124939260647104）
// [任務B] 完成！
// [任務A] 完成！
// [任務C] 完成！
// 並發執行總時間: 301 毫秒
//
// ========================================
// 效能比較：
// 循序執行: 901 毫秒
// 並發執行: 301 毫秒
// 加速比: 2.99336 倍
// （注意：這個加速來自「同時等待」而非「同時運算」——performTask 的主體是 sleep，屬於 I/O 密集模型，單核心機器也會有幾乎相同的加速）
// ========================================
//
// === LeetCode 1114. Print in Order ===
//   執行緒建立順序: third → second → first（故意顛倒）
//   實際輸出結果  : firstsecondthird
//   ← 建立順序完全不影響結果，順序是靠 condition_variable 建立的
//
// === 日常實務：並發拉取設定 + 不撕裂的日誌 ===
//     [config-center] 開始拉取設定...
//     [feature-flags] 開始拉取設定...
//     [cert-service] 開始拉取設定...
//     [feature-flags] 完成（耗時 80 ms）
//     [config-center] 完成（耗時 120 ms）
//     [cert-service] 完成（耗時 150 ms）
//   循序拉取: 350 ms（120+80+150 相加）
//   並發拉取: 150 ms（≈ 最慢的 cert-service 150ms）
//   ← 每一行日誌都完整不撕裂，因為用 mutex 保護了整行輸出
//
// ========================================
// 全部示範結束，總耗時: 1704 毫秒
// ========================================
